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

#include "rsa_json_rpc_impl.h"
#include "celix_log_helper.h"
#include "celix_rsa_rpc_factory.h"
#include "celix_bundle_activator.h"
#include <assert.h>


typedef struct rsa_json_rpc_activator {
    celix_bundle_context_t *ctx;
    rsa_json_rpc_t *jsonRpc;
    celix_rsa_rpc_factory_t rpcFac;
    long rpcSvcId;
    celix_log_helper_t *logHelper;
}rsa_json_rpc_activator_t;

static celix_status_t rsaJsonRpc_start(rsa_json_rpc_activator_t* activator, celix_bundle_context_t* ctx) {
    celix_status_t status = CELIX_SUCCESS;
    assert(activator != NULL);
    assert(ctx != NULL);

    activator->ctx = ctx;
    activator->rpcSvcId = -1;
    celix_autoptr(celix_log_helper_t) logHelper = activator->logHelper = celix_logHelper_create(ctx, "rsa_json_rpc");
    if (activator->logHelper == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    status = rsaJsonRpc_create(ctx, activator->logHelper, &activator->jsonRpc);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(activator->logHelper, "Error creating json rpc. %d.", status);
        return status;
    }
    celix_autoptr(rsa_json_rpc_t) jsonRpc = activator->jsonRpc;
    celix_properties_t *props = celix_properties_create();
    if (props == NULL) {
        celix_logHelper_error(activator->logHelper, "Error creating properties for json rpc.");
        return CELIX_ENOMEM;
    }
    celix_properties_set(props, CELIX_RSA_RPC_TYPE_KEY, "celix.remote.admin.rpc_type.json");
    activator->rpcFac.handle = activator->jsonRpc;
    activator->rpcFac.createProxy = rsaJsonRpc_createProxy;
    activator->rpcFac.destroyProxy = rsaJsonRpc_destroyProxy;
    activator->rpcFac.createEndpoint = rsaJsonRpc_createEndpoint;
    activator->rpcFac.destroyEndpoint = rsaJsonRpc_destroyEndpoint;
    activator->rpcFac.handleRequest = celix_rsaJsonRpc_handleRequest;
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.serviceName = CELIX_RSA_RPC_FACTORY_NAME;
    opts.serviceVersion = CELIX_RSA_RPC_FACTORY_VERSION;
    opts.properties = props;
    opts.svc = &activator->rpcFac;
    activator->rpcSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    if (activator->rpcSvcId < 0) {
        celix_logHelper_error(activator->logHelper, "Error registering json rpc service.");
        return CELIX_BUNDLE_EXCEPTION;
    }
    celix_steal_ptr(jsonRpc);
    celix_steal_ptr(logHelper);
    return CELIX_SUCCESS;
}

static celix_status_t rsaJsonRpc_stop(rsa_json_rpc_activator_t *activator, celix_bundle_context_t* ctx) {
    assert(activator != NULL);
    assert(ctx != NULL);
    celix_bundleContext_unregisterServiceAsync(ctx, activator->rpcSvcId, NULL, NULL);
    celix_bundleContext_waitForEvents(ctx);//Ensure that no events use jsonRpc
    rsaJsonRpc_destroy(activator->jsonRpc);
    celix_bundleContext_waitForEvents(ctx);//Ensure that no events use logHelper
    celix_logHelper_destroy(activator->logHelper);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(rsa_json_rpc_activator_t, rsaJsonRpc_start, rsaJsonRpc_stop)
