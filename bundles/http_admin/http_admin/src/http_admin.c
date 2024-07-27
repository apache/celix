/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <ctype.h>
#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "http_admin.h"
#include "http_admin/api.h"
#include "service_tree.h"

#include "civetweb.h"

#include "celix_compiler.h"
#include "celix_threads.h"


struct http_admin_manager {
    celix_bundle_context_t *context;
    struct mg_context *mgCtx;
    char *root;

    celix_thread_rwlock_t admin_lock;  //protects below

    celix_http_info_service_t infoSvc;
    long infoSvcId;
    celix_array_list_t *aliasList;      //Array list of http_alias_t
    service_tree_t http_svc_tree;
};


typedef struct http_alias {
    char *url;
    char *alias_path;
    long bundle_id;
} http_alias_t;


//Local function prototypes
static int http_request_handle(struct mg_connection *connection);
static void httpAdmin_updateInfoSvc(http_admin_manager_t *admin);
static void createAliasesSymlink(const char *aliases, const char *admin_root, const char *bundle_root, long bundle_id, celix_array_list_t *alias_list);
static bool aliasList_containsAlias(celix_array_list_t *alias_list, const char *alias);


http_admin_manager_t *httpAdmin_create(celix_bundle_context_t *context, char *root, const char **svr_opts) {
    celix_status_t status;
    struct mg_callbacks callbacks;

    http_admin_manager_t *admin = (http_admin_manager_t *) calloc(1, sizeof(http_admin_manager_t));

    admin->context = context;
    admin->root = root;
    admin->infoSvcId = -1L;

    status = celixThreadRwlock_create(&admin->admin_lock, NULL);
    admin->aliasList = celix_arrayList_create();

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
        httpAdmin_updateInfoSvc(admin);
        fprintf(stdout, "HTTP Admin started at port %s\n", ports);
    }

    if (status != CELIX_SUCCESS) {
        if (admin->mgCtx != NULL) {
            mg_stop(admin->mgCtx);
        }
        celixThreadRwlock_destroy(&admin->admin_lock);

        celix_arrayList_destroy(admin->aliasList);
        free(admin);
        admin = NULL;
    }
    return admin;
}

void httpAdmin_destroy(http_admin_manager_t *admin) {

    if(admin->mgCtx != NULL) {
        mg_stop(admin->mgCtx);
    }

    celixThreadRwlock_writeLock(&(admin->admin_lock));
    celix_bundleContext_unregisterService(admin->context, admin->infoSvcId);
    destroyServiceTree(&admin->http_svc_tree);

    //Destroy alias map by removing symbolic links first.
    unsigned int size = celix_arrayList_size(admin->aliasList);
    for (int i = 0; i < size; ++i) {
        http_alias_t *alias = celix_arrayList_get(admin->aliasList, i);

        //Delete alias in cache directory
        if (remove(alias->alias_path) < 0) {
            fprintf(stdout, "remove of %s failed\n", alias->alias_path);
        }

        free(alias->url);
        free(alias->alias_path);
        free(alias);
    }
    celix_arrayList_destroy(admin->aliasList);

    celixThreadRwlock_unlock(&(admin->admin_lock));
    celixThreadRwlock_destroy(&(admin->admin_lock));

    free(admin->root);
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
        celix_auto(celix_rwlock_wlock_guard_t) lock = celixRwlockWlockGuard_init(&(admin->admin_lock));
        if(!addServiceNode(&admin->http_svc_tree, uri, httpSvc)) {
            printf("HTTP service with URI %s already exists!\n", uri);
        }
    }
}

void http_admin_removeHttpService(void *handle, void *svc CELIX_UNUSED, const celix_properties_t *props) {
    http_admin_manager_t *admin = (http_admin_manager_t *) handle;

    const char *uri = celix_properties_get(props, HTTP_ADMIN_URI, NULL);

    if(uri != NULL) {
        celix_auto(celix_rwlock_wlock_guard_t) lock = celixRwlockWlockGuard_init(&(admin->admin_lock));
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->http_svc_tree, uri);

        if(node != NULL){
            destroyServiceNode(&admin->http_svc_tree, node, &admin->http_svc_tree.tree_node_count, &admin->http_svc_tree.tree_svc_count);
        } else {
            printf("Couldn't remove HTTP service with URI: %s, it doesn't exist\n", uri);
        }
    }
}

int http_request_handle(struct mg_connection *connection) {
    int ret_status = 400; //Default bad request

    if (connection != NULL) {
        const struct mg_request_info *ri = mg_get_request_info(connection);
        http_admin_manager_t *admin = (http_admin_manager_t *) ri->user_data;
        service_tree_node_t *node = NULL;

        if (mg_get_header(connection, "Upgrade") != NULL) {
            //Assume this is a websocket request...
            ret_status = 0; //... so return zero to let the civetweb server handle the request.
        }
        else {
            celix_auto(celix_rwlock_rlock_guard_t) lock = celixRwlockRlockGuard_init(&(admin->admin_lock));
            const char *req_uri = ri->request_uri;
            node = findServiceNodeInTree(&admin->http_svc_tree, req_uri);

            if (node != NULL) {
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
                            rcv_buf = malloc((size_t) content_size + 1);
                            rcv_buf[content_size] = '\0';
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
                            rcv_buf = malloc((size_t) content_size + 1);
                            rcv_buf[content_size] = '\0';
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
                            rcv_buf = malloc((size_t) content_size + 1);
                            rcv_buf[content_size] = '\0';
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
                            rcv_buf = malloc((size_t) content_size + 1);
                            rcv_buf[content_size] = '\0';
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

static void httpAdmin_updateInfoSvc(http_admin_manager_t *admin) {
    const char *ports = mg_get_option(admin->mgCtx, "listening_ports");

    char *resources_urls = NULL;
    size_t resources_urls_size;
    FILE *stream = open_memstream(&resources_urls, &resources_urls_size);

    unsigned int size = celix_arrayList_size(admin->aliasList);
    for (unsigned int i = 0; i < size; ++i) {
        http_alias_t *alias = celix_arrayList_get(admin->aliasList, i);
        bool isLast = (i == size-1);
        const char *separator = isLast ? "" : ",";
        fprintf(stream, "%s%s", alias->url, separator);
    }
    fclose(stream);

    celix_auto(celix_rwlock_wlock_guard_t) lock = celixRwlockWlockGuard_init(&(admin->admin_lock));
    celix_bundleContext_unregisterService(admin->context, admin->infoSvcId);

    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, HTTP_ADMIN_INFO_PORT, ports);
    if (resources_urls_size > 1) {
        celix_properties_set(properties, HTTP_ADMIN_INFO_RESOURCE_URLS, resources_urls);
    }
    admin->infoSvcId = celix_bundleContext_registerService(admin->context, &admin->infoSvc, HTTP_ADMIN_INFO_SERVICE_NAME, properties);

    free(resources_urls);
}


/**
 * Decode aliases from the given alias string and make a symbolic link for proper resource behavior.
 * Save the created aliases as pair with the corresponding bundle id in the given hash map.
 *
 * @param aliases                      String which contains the aliases to append to the alias map
 * @param admin_root                   Root path of bundle where the admin runs
 * @param bundle_root                  Bundle path of bundle where the resources remain
 * @param bundle_id                    Bundle ID to connect aliases to this bundle
 * @param alias_map                    Pointer to the alias map to which the created symlink is saved with the bundle id as key
 */
static void createAliasesSymlink(const char *aliases, const char *admin_root, const char *bundle_root, long bundle_id, celix_array_list_t *alias_list) {
    char *token = NULL;
    char *sub_token = NULL;
    char *save_ptr = NULL;
    char *sub_save_ptr = NULL;
    char *alias_cpy;

    if(aliases != NULL && alias_list != NULL && admin_root != NULL && bundle_root != NULL) {
        token = alias_cpy = strdup(aliases);
        token = strtok_r(token, ",", &save_ptr);

        while(token != NULL) {
            int i;
            char *alias_path;
            char *bnd_resource_path;
            char cwdBuf[1024] = {0};
            char *cwd = getcwd(cwdBuf, sizeof(cwdBuf));

            while(isspace(*token)) token++; //skip spaces at beginning
            for (i = (int)(strlen(token) - 1); (isspace(token[i])); i--); //skip spaces at end
            token[i+1] = '\0';

            sub_token = strtok_r(token, ";", &sub_save_ptr);
            if(sub_token == NULL) {
                token = strtok_r(NULL, ",", &save_ptr);
                continue; //Invalid alias to path, continue to next entry
            }

            asprintf(&alias_path, "%s/%s%s", cwd, admin_root, sub_token);
            if(aliasList_containsAlias(alias_list, alias_path)) {
                free(alias_path);
                continue; //Alias already existing, Continue to next entry
            }

            sub_token = strtok_r(NULL, ";", &sub_save_ptr);
            if(sub_token == NULL) {
                free(alias_path);
                token = strtok_r(NULL, ",", &save_ptr);
                continue; //Invalid alias to path, continue to next entry
            }

            asprintf(&bnd_resource_path, "%s/%s/%s", cwd, bundle_root, sub_token);

            if(symlink(bnd_resource_path, alias_path) == 0) {
                http_alias_t *alias = calloc(1, sizeof(*alias));
                alias->url = strndup(token, 1024 * 1024 * 10);
                alias->alias_path = alias_path;
                alias->bundle_id = bundle_id;
                celix_arrayList_add(alias_list, alias);
            } else {
                free(alias_path);
            }

            free(bnd_resource_path);
            token = strtok_r(NULL, ",", &save_ptr);
        }
        free(alias_cpy);
    }
}

/**
 * Check if the given alias is already part of the given alias list
 *
 * @param alias_list                   The list to search for aliases
 * @param alias                        The alias to search for
 * @return true if alias is already in the list, false if not.
 */
static bool aliasList_containsAlias(celix_array_list_t *alias_list, const char *alias) {
    unsigned int size = celix_arrayList_size(alias_list);
    for(unsigned int i = 0; i < size; i++) {
        http_alias_t *http_alias = celix_arrayList_get(alias_list, i);
        if(strcmp(http_alias->alias_path, alias) == 0) {
            return true;
        }
    }

    return false;
}

void http_admin_startBundle(void *data, const celix_bundle_t *bundle) {
    http_admin_manager_t *admin = data;
    const char* aliases = celix_bundle_getManifestValue(bundle, "X-Web-Resource");
    if (aliases == NULL) {
        celix_bundleContext_log(admin->context, CELIX_LOG_LEVEL_TRACE, "No aliases found for bundle %s",
                                celix_bundle_getSymbolicName(bundle));
        return;
    }
    char* bundleRoot = celix_bundle_getEntry(bundle, "");
    if (bundleRoot == NULL) {
        celix_bundleContext_log(admin->context, CELIX_LOG_LEVEL_ERROR, "No root for bundle %s",
                                celix_bundle_getSymbolicName(bundle));
        return;
    }
    long bndId = celix_bundle_getId(bundle);
    createAliasesSymlink(aliases, admin->root, bundleRoot, bndId, admin->aliasList);
    free(bundleRoot);
    httpAdmin_updateInfoSvc(admin);
}

void http_admin_stopBundle(void *data, const celix_bundle_t *bundle) {
    http_admin_manager_t *admin = data;
    long bundle_id = celix_bundle_getId(bundle);

    //Remove all aliases which are connected to this bundle
    unsigned int size = celix_arrayList_size(admin->aliasList);
    for (unsigned int i = (size - 1); i < size; i--) {
        http_alias_t *alias = celix_arrayList_get(admin->aliasList, i);
        if(alias->bundle_id == bundle_id) {
            remove(alias->alias_path); //Delete alias in cache directory
            free(alias->url);
            free(alias->alias_path);
            free(alias);
            celix_arrayList_removeAt(admin->aliasList, i);
        }
    }
    httpAdmin_updateInfoSvc(admin);
}

