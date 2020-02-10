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
#include <celix_shell.h>
#include <civetweb.h>

#include "http_admin/api.h"

typedef struct shell_wui_activator_data {
    celix_bundle_context_t *ctx;

    celix_websocket_service_t sockSvc;
    long sockSvcId;
} shell_wui_activator_data_t;

struct use_shell_arg {
    char *command;
    struct mg_connection *conn;
};

static void useShell(void *handle, void *svc) {
    celix_shell_t *shell = svc;
    struct use_shell_arg *arg = handle;
    char *buf = NULL;
    size_t size;
    FILE *out = open_memstream(&buf, &size);
    shell->executeCommand(shell->handle, arg->command, out, out);
    fclose(out);
    mg_websocket_write(arg->conn, MG_WEBSOCKET_OPCODE_TEXT, buf, size);
};

static int websocket_data_handler(struct mg_connection *conn, int bits, char *data, size_t data_len, void *handle) {
    shell_wui_activator_data_t *act = handle;
    struct use_shell_arg arg;
    arg.conn = conn;

    //NOTE data is a not null terminated string..
    arg.command = calloc(data_len+1, sizeof(char));
    memcpy(arg.command, data, data_len);
    arg.command[data_len] = '\0';


    bool called = celix_bundleContext_useService(act->ctx, CELIX_SHELL_SERVICE_NAME, &arg, useShell);
    if (!called) {
        const char *msg = "No shell available!";
        mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_TEXT, msg , strlen(msg));
    }

    free(arg.command);
    return 1; //keep open
}


static celix_status_t shellWui_activator_start(shell_wui_activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, WEBSOCKET_ADMIN_URI, "/shell/socket");
    data->sockSvc.handle = data;
    data->sockSvc.data = websocket_data_handler;
    data->sockSvcId = celix_bundleContext_registerService(ctx, &data->sockSvc, WEBSOCKET_ADMIN_SERVICE_NAME, props);

    return CELIX_SUCCESS;
}

static celix_status_t shellWui_activator_stop(shell_wui_activator_data_t *data, celix_bundle_context_t *ctx) {

    celix_bundleContext_unregisterService(ctx, data->sockSvcId);

    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(shell_wui_activator_data_t, shellWui_activator_start, shellWui_activator_stop)