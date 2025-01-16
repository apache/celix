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
#include <memory.h>

#include "celix_bundle_activator.h"
#include "http_admin.h"
#include "websocket_admin.h"
#include "http_admin/api.h"
#include "http_admin_constants.h"
#include "civetweb.h"
#include "celix_file_utils.h"


typedef struct http_admin_activator {
    http_admin_manager_t *httpManager;
    websocket_admin_manager_t *sockManager;

    long httpAdminSvcId;
    long sockAdminSvcId;

    bool useWebsockets;

    long bundleTrackerId;
} http_admin_activator_t;

static int http_admin_start(http_admin_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundle_t *bundle = celix_bundleContext_getBundle(ctx);
    char* storeRoot = celix_bundle_getDataFile(bundle, "");
    if (storeRoot == NULL) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot get bundle store root for the http admin bundle.");
        return CELIX_BUNDLE_EXCEPTION;
    }

    celix_autofree char* httpRoot = NULL;
    int rc = asprintf(&httpRoot, "%s/root", storeRoot);
    if (rc < 0) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create http root directory for the http admin bundle.");
        free(storeRoot);
        return CELIX_ENOMEM;
    }
    free(storeRoot);

    celix_status_t status = celix_utils_createDirectory(httpRoot, false, NULL);
    if (status != CELIX_SUCCESS) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot create http root directory for the http admin bundle.");
        return status;
    }
    celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_DEBUG, "Using http root directory %s", httpRoot);

    bool prop_use_websockets = celix_bundleContext_getPropertyAsBool(ctx, HTTP_ADMIN_USE_WEBSOCKETS_KEY, HTTP_ADMIN_USE_WEBSOCKETS_DFT);
    long listPort = celix_bundleContext_getPropertyAsLong(ctx,    HTTP_ADMIN_LISTENING_PORTS_KEY, HTTP_ADMIN_LISTENING_PORTS_DFT);
    long websocketTimeoutMs = celix_bundleContext_getPropertyAsLong(ctx, HTTP_ADMIN_WEBSOCKET_TIMEOUT_MS_KEY, HTTP_ADMIN_WEBSOCKET_TIMEOUT_MS_DFT);
    long prop_port_min = celix_bundleContext_getPropertyAsLong(ctx, HTTP_ADMIN_PORT_RANGE_MIN_KEY, HTTP_ADMIN_PORT_RANGE_MIN_DFT);
    long prop_port_max = celix_bundleContext_getPropertyAsLong(ctx, HTTP_ADMIN_PORT_RANGE_MAX_KEY, HTTP_ADMIN_PORT_RANGE_MAX_DFT);
    long num_threads = celix_bundleContext_getPropertyAsLong(ctx, HTTP_ADMIN_NUM_THREADS_KEY, HTTP_ADMIN_NUM_THREADS_DFT);

    char prop_port[64];
    snprintf(prop_port, 64, "%li", listPort);
    char prop_timeout[64];
    snprintf(prop_timeout, 64, "%li", websocketTimeoutMs);
    char prop_num_threads[64];
    snprintf(prop_num_threads, 64, "%li", num_threads);


    act->useWebsockets = prop_use_websockets;

    const char *svr_opts[] = {
            "document_root", httpRoot,
            "listening_ports", prop_port,
            "websocket_timeout_ms", prop_timeout,
            "websocket_root", httpRoot,
            "num_threads", prop_num_threads,
            NULL
    };

    //Try the 'LISTENING_PORT' property first, if failing continue with the port range functionality
    act->httpManager = httpAdmin_create(ctx, httpRoot, svr_opts);

    for(long port = prop_port_min; act->httpManager == NULL && port <= prop_port_max; port++) {
        char *port_str;
        rc= asprintf(&port_str, "%li", port);
        if(rc < 0 || port_str == NULL) {
            celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot allocate memory for port string.");
            free(httpRoot);
            return CELIX_ENOMEM;
        }
        svr_opts[3] = port_str;
        act->httpManager = httpAdmin_create(ctx, httpRoot, svr_opts);
        free(port_str);
    }

    if (act->httpManager != NULL) {
        {
            celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
            opts.callbackHandle = act->httpManager;
            opts.addWithProperties = http_admin_addHttpService;
            opts.removeWithProperties = http_admin_removeHttpService;
            opts.filter.serviceName = HTTP_ADMIN_SERVICE_NAME;
            act->httpAdminSvcId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
        }
        {
            celix_bundle_tracking_options_t opts = CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS;
            opts.callbackHandle = act->httpManager;
            opts.onStarted = http_admin_startBundle;
            opts.onStopped = http_admin_stopBundle;
            act->bundleTrackerId = celix_bundleContext_trackBundlesWithOptions(ctx, &opts);
        }

        //Websockets are dependent from the http admin, which starts the server.
        if(act->useWebsockets) {
            //Retrieve some data from the http admin and reuse it for the websocket admin
            struct mg_context *svr_ctx = httpAdmin_getServerContext(act->httpManager);
            act->sockManager = websocketAdmin_create(ctx, svr_ctx);
            if (act->sockManager != NULL) {
                celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
                opts.callbackHandle = act->sockManager;
                opts.addWithProperties = websocket_admin_addWebsocketService;
                opts.removeWithProperties = websocket_admin_removeWebsocketService;
                opts.filter.serviceName = WEBSOCKET_ADMIN_SERVICE_NAME;
                act->sockAdminSvcId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
            }
        }
    }

    return CELIX_SUCCESS;
}

static int http_admin_stop(http_admin_activator_t *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_stopTracker(ctx, act->httpAdminSvcId);
    celix_bundleContext_stopTracker(ctx, act->sockAdminSvcId);
    celix_bundleContext_stopTracker(ctx, act->bundleTrackerId);
    httpAdmin_destroy(act->httpManager);

    if(act->useWebsockets && act->sockManager != NULL) {
        websocketAdmin_destroy(act->sockManager);
        act->useWebsockets = false;
    }

    return CELIX_SUCCESS;
}


CELIX_GEN_BUNDLE_ACTIVATOR(http_admin_activator_t, http_admin_start, http_admin_stop);
