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

#include <stdlib.h>

#include "celix_api.h"
#include "http_admin/api.h"

#include "civetweb.h"

struct activator {
    celix_http_service_t httpSvc;
    celix_http_service_t httpSvc2;
    celix_http_service_t httpSvc3;
    long httpSvcId;
    long httpSvcId2;
    long httpSvcId3;

    celix_websocket_service_t sockSvc;
    long sockSvcId;
};

//Local function prototypes
int alias_test_put(void *handle, struct mg_connection *connection, const char *path, const char *data, size_t length);
int websocket_data_echo(struct mg_connection *connection, int op_code, char *data, size_t length, void *handle);

celix_status_t bnd_start(struct activator *act, celix_bundle_context_t *ctx) {
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, HTTP_ADMIN_URI, "/alias");
    act->httpSvc.handle = act;
    act->httpSvc.doPut = alias_test_put;
    act->httpSvcId = celix_bundleContext_registerService(ctx, &act->httpSvc, HTTP_ADMIN_SERVICE_NAME, props);

    celix_properties_t *props2 = celix_properties_create();
    celix_properties_set(props2, HTTP_ADMIN_URI, "/foo/bar");
    act->httpSvc2.handle = act;
    act->httpSvcId2 = celix_bundleContext_registerService(ctx, &act->httpSvc2, HTTP_ADMIN_SERVICE_NAME, props2);

    celix_properties_t *props3 = celix_properties_create();
    celix_properties_set(props3, HTTP_ADMIN_URI, "/");
    act->httpSvc3.handle = act;
    act->httpSvcId3 = celix_bundleContext_registerService(ctx, &act->httpSvc3, HTTP_ADMIN_SERVICE_NAME, props3);

    celix_properties_t *props4 = celix_properties_create();
    celix_properties_set(props4, WEBSOCKET_ADMIN_URI, "/");
    act->sockSvc.handle = act;
    act->sockSvc.data = websocket_data_echo;
    act->sockSvcId = celix_bundleContext_registerService(ctx, &act->sockSvc, WEBSOCKET_ADMIN_SERVICE_NAME, props4);

    return CELIX_SUCCESS;
}

celix_status_t bnd_stop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->httpSvcId);
    celix_bundleContext_unregisterService(ctx, act->httpSvcId2);
    celix_bundleContext_unregisterService(ctx, act->httpSvcId3);
    celix_bundleContext_unregisterService(ctx, act->sockSvcId);

    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bnd_start, bnd_stop);

int alias_test_put(void *handle __attribute__((unused)), struct mg_connection *connection, const char *path __attribute__((unused)), const char *data, size_t length) {
    //If data received, echo the data for the test case
    if(length > 0 && data != NULL) {
        const char *mime_type = mg_get_header(connection, "Content-Type");
        mg_printf(connection,
                  "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n%s", mime_type, data);

    }

    return 200;
}

int websocket_data_echo(struct mg_connection *connection, int op_code __attribute__((unused)), char *data, size_t length, void *handle __attribute__((unused))) {
    mg_websocket_write(connection, MG_WEBSOCKET_OPCODE_PONG, data, length);

    return 0; //Close socket after echoing.
}
