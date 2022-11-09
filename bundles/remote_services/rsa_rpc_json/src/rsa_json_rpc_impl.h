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

#ifndef _RSA_JSON_RPC_IMPL_H_
#define _RSA_JSON_RPC_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <endpoint_description.h>
#include <celix_log_helper.h>
#include <celix_api.h>

typedef struct rsa_json_rpc rsa_json_rpc_t;

celix_status_t rsaJsonRpc_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper, rsa_json_rpc_t **jsonRpcOut);

void rsaJsonRpc_destroy(rsa_json_rpc_t *jsonRpc);

celix_status_t rsaJsonRpc_createProxy(void *handle, const endpoint_description_t *endpointDesc,
        long requestSenderSvcId, long *proxySvcId);

void rsaJsonRpc_destroyProxy(void *handle, long proxySvcId);

celix_status_t rsaJsonRpc_createEndpoint(void *handle, const endpoint_description_t *endpointDesc,
        long *requestHandlerSvcId);

void rsaJsonRpc_destroyEndpoint(void *handle, long requestHandlerSvcId);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_JSON_RPC_IMPL_H_ */
