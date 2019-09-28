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

#include <string.h>
#include <stdio.h>

#include <celix_api.h>
#include <civetweb.h>

#include "http_admin/api.h"

typedef struct http_example_activator_data {
    celix_bundle_context_t *ctx;

    celix_websocket_service_t sockSvc;
    long sockSvcId;
} http_example_activator_data_t;


static void websocket_ready_handler(struct mg_connection *conn, void *handle) {
    //http_example_activator_data_t *act = handle;

    const char *msg = "Hello World from Websocket";
    mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
}


static celix_status_t httpExample_activator_start(http_example_activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, WEBSOCKET_ADMIN_URI, "/hello/socket");
    data->sockSvc.handle = data;
    data->sockSvc.ready = websocket_ready_handler;
    data->sockSvcId = celix_bundleContext_registerService(ctx, &data->sockSvc, WEBSOCKET_ADMIN_SERVICE_NAME, props);

    return CELIX_SUCCESS;
}

static celix_status_t httpExample_activator_stop(http_example_activator_data_t *data, celix_bundle_context_t *ctx) {

    celix_bundleContext_unregisterService(ctx, data->sockSvcId);

    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(http_example_activator_data_t, httpExample_activator_start, httpExample_activator_stop)