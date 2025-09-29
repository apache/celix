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
#include "endpoint_description.h"
#include "celix_rsa_rpc_factory.h"
#include "celix_cleanup.h"
#include "celix_log_helper.h"
#include "celix_types.h"
#include "celix_errno.h"

typedef struct rsa_json_rpc rsa_json_rpc_t;

celix_status_t rsaJsonRpc_create(celix_bundle_context_t* ctx, celix_log_helper_t *logHelper, rsa_json_rpc_t **jsonRpcOut);

void rsaJsonRpc_destroy(rsa_json_rpc_t *jsonRpc);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(rsa_json_rpc_t, rsaJsonRpc_destroy)

celix_status_t rsaJsonRpc_createProxy(void* handle, const endpoint_description_t* endpointDesc,
                                      celix_rsa_send_request_fp sendRequest, void* sendRequestHandle, long* proxyId);

void rsaJsonRpc_destroyProxy(void *handle, long proxyId);

celix_status_t rsaJsonRpc_createEndpoint(void *handle, const endpoint_description_t *endpointDesc,
        long *endpointId);

void rsaJsonRpc_destroyEndpoint(void *handle, long endpointId);

celix_status_t celix_rsaJsonRpc_handleRequest(void *handle, long endpointId, celix_properties_t *metadata, const struct iovec *request, struct iovec *response);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_JSON_RPC_IMPL_H_ */
