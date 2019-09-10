/**
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
/*
 * websocket_admin.c
 *
 *  \date       May 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <memory.h>

#include "websocket_admin.h"
#include "http_admin/api.h"
#include "http_admin.h"
#include "service_tree.h"

#include "civetweb.h"

#include "celix_api.h"
#include "celix_utils_api.h"
#include "hash_map.h"


struct websocket_admin_manager {
    bundle_context_pt context;

    struct mg_context *mg_ctx;

    service_tree_t sock_svc_tree;
    celix_thread_mutex_t admin_lock;

};

websocket_admin_manager_t *websocketAdmin_create(celix_bundle_context_t *context, struct mg_context *svr_ctx) {
    celix_status_t status;

    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) calloc(1, sizeof(websocket_admin_manager_t));

    if (admin == NULL) {
        return NULL;
    }

    admin->context = context;
    admin->mg_ctx = svr_ctx;
    status = celixThreadMutex_create(&admin->admin_lock, NULL);

    if(status != CELIX_SUCCESS) {
        //No need to destroy other things
        free(admin);
    }

    return admin;
}

void websocketAdmin_destroy(websocket_admin_manager_t *admin) {
    celixThreadMutex_lock(&(admin->admin_lock));

    //Destroy tree with services
    destroyServiceTree(&admin->sock_svc_tree);

    celixThreadMutex_unlock(&(admin->admin_lock));

    celixThreadMutex_destroy(&(admin->admin_lock));

    free(admin);
}

void websocket_admin_addWebsocketService(void *handle, void *svc, const celix_properties_t *props) {
    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) handle;
    celix_websocket_service_t *websockSvc = (celix_websocket_service_t *) svc;

    const char *uri = celix_properties_get(props, WEBSOCKET_ADMIN_URI, NULL);

    if(uri != NULL) {
        celixThreadMutex_lock(&(admin->admin_lock));
        if(addServiceNode(&admin->sock_svc_tree, uri, websockSvc)) {
            mg_set_websocket_handler(admin->mg_ctx, uri, websocket_connect_handler, websocket_ready_handler, websocket_data_handler, websocket_close_handler, admin);
        } else {
            printf("Websocket service with URI %s already exists!\n", uri);
        }
        celixThreadMutex_unlock(&(admin->admin_lock));
    }
}

void websocket_admin_removeWebsocketService(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) handle;

    const char *uri = celix_properties_get(props, WEBSOCKET_ADMIN_URI, NULL);

    if(uri != NULL) {
        celixThreadMutex_lock(&(admin->admin_lock));
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->sock_svc_tree, uri);

        if(node != NULL){
            destroyServiceNode(&admin->sock_svc_tree, node, &admin->sock_svc_tree.tree_node_count, &admin->sock_svc_tree.tree_svc_count);
        } else {
            printf("Couldn't remove websocket service with URI: %s, it doesn't exist\n", uri);
        }

        celixThreadMutex_unlock(&(admin->admin_lock));
    }
}

int websocket_connect_handler(const struct mg_connection *connection, void *handle) {
    int result = 1; //Default 1. Close socket by returning a non-zero value, something went wrong.
    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) handle;

    if(connection != NULL && handle != NULL) {
        const struct mg_request_info *ri = mg_get_request_info(connection);
        const char *req_uri = ri->request_uri;
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->sock_svc_tree, req_uri);

        if(node != NULL) {
            //Requested URI exists, now obtain the service and delegate the callback handle.
            celix_websocket_service_t *sockSvc = (celix_websocket_service_t *) node->svc_data->service;

            if(sockSvc->connect != NULL) {
                result = sockSvc->connect(connection, sockSvc->handle);
            }
            else {
                result = 0; //No connect callback attached, proceed without error.
            }
        }
    }

    return result;
}


void websocket_ready_handler(struct mg_connection *connection, void *handle) {
    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) handle;

    if(connection != NULL && handle != NULL) {
        const struct mg_request_info *ri = mg_get_request_info(connection);
        const char *req_uri = ri->request_uri;
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->sock_svc_tree, req_uri);

        if(node != NULL) {
            //Requested URI exists, now obtain the service and delegate the callback handle.
            celix_websocket_service_t *sockSvc = (celix_websocket_service_t *) node->svc_data->service;

            if(sockSvc->ready != NULL) {
                sockSvc->ready(connection, sockSvc->handle);
            }
        }
    }
}

int websocket_data_handler(struct mg_connection *connection, int op_code, char *data, size_t length, void *handle) {
    int result = 0; //Default 0. Close socket by returning 0, something went wrong.
    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) handle;

    if(connection != NULL && handle != NULL) {
        const struct mg_request_info *ri = mg_get_request_info(connection);
        const char *req_uri = ri->request_uri;
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->sock_svc_tree, req_uri);

        if(node != NULL) {
            //Requested URI exists, now obtain the service and delegate the callback handle.
            celix_websocket_service_t *sockSvc = (celix_websocket_service_t *) node->svc_data->service;

            if(sockSvc->data != NULL) {
                result = sockSvc->data(connection, op_code, data, length, sockSvc->handle);
            }
        }
    }

    return result;
}

void websocket_close_handler(const struct mg_connection *connection, void *handle) {
    websocket_admin_manager_t *admin = (websocket_admin_manager_t *) handle;

    if (connection != NULL && handle != NULL) {
        const struct mg_request_info *ri = mg_get_request_info(connection);
        const char *req_uri = ri->request_uri;
        service_tree_node_t *node = NULL;

        node = findServiceNodeInTree(&admin->sock_svc_tree, req_uri);

        if(node != NULL) {
            //Requested URI exists, now obtain the service and delegate the callback handle.
            celix_websocket_service_t *sockSvc = (celix_websocket_service_t *) node->svc_data->service;

            if (sockSvc->close != NULL) {
                sockSvc->close(connection, sockSvc->handle);
            }
        }
    }
}
