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


#include "celix_bundle_activator.h"
#include "celix_bundle.h"
#include "shell.h"
#include "celix_utils_api.h" //for open_memstream for APPLE,BSD,ANDROID

#include <civetweb.h>
#include <string.h>
#include <stdio.h>


typedef struct activator_data {
    celix_bundle_context_t *ctx;
    char *root;
    struct mg_context *mgCtx;
} activator_data_t;

struct use_shell_arg {
    char *command;
    struct mg_connection *conn;
};

static void useShell(void *handle, void *svc) {
    shell_service_t *shell = svc;
    struct use_shell_arg *arg = handle;
    char *buf = NULL;
    size_t size;
    FILE *out = open_memstream(&buf, &size);
    shell->executeCommand(shell->shell, (char*)arg->command, out, out);
    fclose(out);
    mg_websocket_write(arg->conn, MG_WEBSOCKET_OPCODE_TEXT, buf, size);
};

static int websocket_data_handler(struct mg_connection *conn, int bits, char *data, size_t data_len, void *handle) {
    activator_data_t *act = handle;
    struct use_shell_arg arg;
    arg.conn = conn;

    //NOTE data is a not null terminated string..
    arg.command = calloc(data_len+1, sizeof(char));
    memcpy(arg.command, data, data_len);
    arg.command[data_len] = '\0';


    bool called = celix_bundleContext_useService(act->ctx, OSGI_SHELL_SERVICE_NAME, &arg, useShell);
    if (!called) {
        fprintf(stderr, "No shell available!\n");
    }

    free(arg.command);
    return 1; //keep open
}


static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    data->ctx = ctx;

    celix_bundle_t *bnd = celix_bundleContext_getBundle(ctx);
    data->root = celix_bundle_getEntry(bnd, "resources");

    const char *options[] = {
            "document_root", data->root,
            "listening_ports", "8081",
            "websocket_timeout_ms", "3600000",
            NULL
    };

    if (data->root != NULL) {
        data->mgCtx = mg_start(NULL, data, options);
        mg_set_websocket_handler(data->mgCtx, "/shellsocket", NULL, NULL, websocket_data_handler, NULL, data);
    }

    if (data->mgCtx != NULL) {
        fprintf(stdout, "Started civetweb at port %s\n", mg_get_option(data->mgCtx, "listening_ports"));
    } else {
        fprintf(stderr, "Error starting civetweb bundle\n");
    }

    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    if (data->mgCtx != NULL) {
        printf("Stopping civetweb\n");
        mg_stop(data->mgCtx);
        data->mgCtx = NULL;
    }
    free(data->root);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)