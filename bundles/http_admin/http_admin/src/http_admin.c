/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * http_admin.c
 *
 *  \date       May 24, 2019
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <memory.h>
#include <limits.h>

#include "http_admin.h"
#include "http_admin/api.h"
#include "service_tree.h"

#include "civetweb.h"

#include "celix_api.h"
#include "celix_utils_api.h"


struct http_admin_manager {
    bundle_context_pt context;

    struct mg_context *mgCtx;

    celix_http_info_service_t infoSvc;
    long infoSvcId;

    service_tree_t http_svc_tree;
    celix_thread_mutex_t admin_lock;
};

//Local function prototypes
int http_request_handle(struct mg_connection *connection);


http_admin_manager_t *httpAdmin_create(bundle_context_pt context, const char **svr_opts) {
    celix_status_t status;
    struct mg_callbacks callbacks;

    http_admin_manager_t *admin = (http_admin_manager_t *) calloc(1, sizeof(http_admin_manager_t));

    if (admin == NULL) {
        return NULL;
    }

    admin->context = context;
    status = celixThreadMutex_create(&admin->admin_lock, NULL);

    if (status == CELIX_SUCCESS) {
        //Use only begin_request callback
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.begin_request = http_request_handle;

        admin->mgCtx = mg_start(&callbacks, admin, svr_opts);
        status = (admin->mgCtx == NULL ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS);
    }

    if (status == CELIX_SUCCESS) {
        //When webserver started successful, set port info in HTTP admin service
        const char *ports = mg_get_option(admin->mgCtx, "listening_ports");
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, HTTP_ADMIN_INFO_PORT, ports);
        admin->infoSvcId = celix_bundleContext_registerService(context, &admin->infoSvc,
                                                                  HTTP_ADMIN_INFO_SERVICE_NAME, properties);
        fprintf(stdout, "Started HTTP webserver at port %s\n", ports);
    }

    if (status != CELIX_SUCCESS) {
        if (admin->mgCtx != NULL) {
            mg_stop(admin->mgCtx);
        }
        celixThreadMutex_destroy(&admin->admin_lock);

        free(admin);
        admin = NULL;
    }
    return admin;
}

void httpAdmin_destroy(http_admin_manager_t *admin) {
    celixThreadMutex_lock(&(admin->admin_lock));

    if(admin->mgCtx != NULL) {
        mg_stop(admin->mgCtx);
    }

    celix_bundleContext_unregisterService(admin->context, admin->infoSvcId);

    destroyServiceTree(&admin->http_svc_tree);

    celixThreadMutex_unlock(&(admin->admin_lock));
    celixThreadMutex_destroy(&(admin->admin_lock));

    free(admin);
}

struct mg_context *httpAdmin_getServerContext(const http_admin_manager_t *admin) {
    return admin->mgCtx;
}

void http_admin_addHttpService(void *handle, void *svc, const celix_properties_t *props) {
    http_admin_manager_t *admin = (http_admin_manager_t *) handle;
    celix_http_service_t *httpSvc = (celix_http_service_t *) svc;

    const char *uri = celix_properties_get(props, HTTP_ADMIN_URI, NULL);

    if(uri != NULL) {
        celixThreadMutex_lock(&(admin->admin_lock));
        if(!addServiceNode(&admin->http_svc_tree, uri, httpSvc)) {
            printf("HTTP service with URI %s already exists!\n", uri);
        }
        celixThreadMutex_unlock(&(admin->admin_lock));
    }
}

void http_admin_removeHttpService(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
    http_admin_manager_t *admin = (http_admin_manager_t *) handle;

    const char *uri = celix_properties_get(props, HTTP_ADMIN_URI, NULL);

    if(uri != NULL) {
        celixThreadMutex_lock(&(admin->admin_lock));
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->http_svc_tree, uri);

        if(node != NULL){
            destroyServiceNode(node, &admin->http_svc_tree.tree_node_count, &admin->http_svc_tree.tree_svc_count);
        } else {
            printf("Couldn't remove HTTP service with URI: %s, it doesn't exist\n", uri);
        }

        celixThreadMutex_unlock(&(admin->admin_lock));
    }
}

int http_request_handle(struct mg_connection *connection) {
    int ret_status = 400; //Default bad request

    if(connection != NULL) {
        const struct mg_request_info *ri = mg_get_request_info(connection);
        http_admin_manager_t *admin = (http_admin_manager_t *) ri->user_data;
        service_tree_node_t *node = NULL;

        if(mg_get_header(connection, "Upgrade") != NULL) {
            //Assume this is a websocket request...
            ret_status = 0; //... so return zero to let the civetweb server handle the request.
        }
        else {
            const char *req_uri = ri->request_uri;
            node = findServiceNodeInTree(&admin->http_svc_tree, req_uri);

            if(node != NULL) {
                //Requested URI with node exists, now obtain the http service and call the requested function.
                celix_http_service_t *httpSvc = (celix_http_service_t *) node->svc_data->service;

                if (strcmp("GET", ri->request_method) == 0) {
                    if (httpSvc->doGet != NULL) {
                        ret_status = httpSvc->doGet(httpSvc->handle, connection, req_uri);
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else if (strcmp("HEAD", ri->request_method) == 0) {
                    if (httpSvc->doHead != NULL) {
                        ret_status = httpSvc->doHead(httpSvc->handle, connection, req_uri);
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else if (strcmp("POST", ri->request_method) == 0) {
                    if (httpSvc->doPost != NULL) {
                        int bytes_read = 0;
                        bool no_data = false;
                        char *rcv_buf = NULL;

                        if (ri->content_length > 0) {
                            int content_size = (ri->content_length > INT_MAX ? INT_MAX : (int) ri->content_length);
                            rcv_buf = malloc((size_t) content_size);
                            bytes_read = mg_read(connection, rcv_buf, (size_t) content_size);
                        } else {
                            no_data = true;
                        }

                        if (bytes_read > 0 || no_data) {
                            ret_status = httpSvc->doPost(httpSvc->handle, connection, rcv_buf, (size_t) bytes_read);
                        } else {
                            mg_send_http_error(connection, 400, "%s", "Bad request");
                            ret_status = 400; //Bad Request, failed to read data
                        }

                        if (rcv_buf != NULL) {
                            free(rcv_buf);
                        }
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }

                } else if (strcmp("PUT", ri->request_method) == 0) {
                    if (httpSvc->doPut != NULL) {
                        int bytes_read = 0;
                        bool no_data = false;
                        char *rcv_buf = NULL;

                        if (ri->content_length > 0) {
                            int content_size = (ri->content_length > INT_MAX ? INT_MAX : (int) ri->content_length);
                            rcv_buf = malloc((size_t) content_size);
                            bytes_read = mg_read(connection, rcv_buf, (size_t) content_size);
                        } else {
                            no_data = true;
                        }

                        if (bytes_read > 0 || no_data) {
                            ret_status = httpSvc->doPut(httpSvc->handle, connection, req_uri, rcv_buf,
                                                        (size_t) bytes_read);
                        } else {
                            mg_send_http_error(connection, 400, "%s", "Bad request");
                            ret_status = 400; //Bad Request, failed to read data
                        }

                        if (rcv_buf != NULL) {
                            free(rcv_buf);
                        }
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else if (strcmp("DELETE", ri->request_method) == 0) {
                    if (httpSvc->doDelete != NULL) {
                        ret_status = httpSvc->doDelete(httpSvc->handle, connection, req_uri);
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else if (strcmp("TRACE", ri->request_method) == 0) {
                    if (httpSvc->doTrace != NULL) {
                        int bytes_read = 0;
                        bool no_data = false;
                        char *rcv_buf = NULL;

                        if (ri->content_length > 0) {
                            int content_size = (ri->content_length > INT_MAX ? INT_MAX : (int) ri->content_length);
                            rcv_buf = malloc((size_t) content_size);
                            bytes_read = mg_read(connection, rcv_buf, (size_t) content_size);
                        } else {
                            no_data = true;
                        }

                        if (bytes_read > 0 || no_data) {
                            ret_status = httpSvc->doTrace(httpSvc->handle, connection, rcv_buf, (size_t) bytes_read);
                        } else {
                            mg_send_http_error(connection, 400, "%s", "Bad request");
                            ret_status = 400; //Bad Request, failed to read data
                        }

                        if (rcv_buf != NULL) {
                            free(rcv_buf);
                        }
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else if (strcmp("OPTIONS", ri->request_method) == 0) {
                    if (httpSvc->doOptions != NULL) {
                        ret_status = httpSvc->doOptions(httpSvc->handle, connection);
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else if (strcmp("PATCH", ri->request_method) == 0) {
                    if (httpSvc->doPatch != NULL) {
                        int bytes_read = 0;
                        bool no_data = false;
                        char *rcv_buf = NULL;

                        if (ri->content_length > 0) {
                            int content_size = (ri->content_length > INT_MAX ? INT_MAX : (int) ri->content_length);
                            rcv_buf = malloc((size_t) content_size);
                            bytes_read = mg_read(connection, rcv_buf, (size_t) content_size);
                        } else {
                            no_data = true;
                        }

                        if (bytes_read > 0 || no_data) {
                            ret_status = httpSvc->doPatch(httpSvc->handle, connection, req_uri, rcv_buf,
                                                          (size_t) bytes_read);
                        } else {
                            mg_send_http_error(connection, 400, "%s", "Bad request");
                            ret_status = 400; //Bad Request, failed to read data
                        }

                        if (rcv_buf != NULL) {
                            free(rcv_buf);
                        }
                    } else {
                        ret_status = 0; //Let civetweb handle the request
                    }
                } else {
                    mg_send_http_error(connection, 501, "%s", "Not found");
                    ret_status = 501; //Not implemented...
                }
            } else {
                ret_status = 0; //Not found requested URI, let civetweb handle this situation
            }
        }
    } else {
        mg_send_http_error(connection, 400, "%s", "Bad request");
    }


    return ret_status;
}
