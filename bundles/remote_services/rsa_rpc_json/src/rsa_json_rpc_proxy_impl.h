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

#ifndef _RSA_JSON_RPC_PROXY_IMPL_H_
#define _RSA_JSON_RPC_PROXY_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "remote_interceptors_handler.h"
#include "celix_rsa_rpc_factory.h"
#include "endpoint_description.h"
#include "celix_log_helper.h"
#include "celix_types.h"
#include "celix_errno.h"
#include <stdio.h>

typedef struct rsa_json_rpc_proxy_factory rsa_json_rpc_proxy_factory_t;

celix_status_t rsaJsonRpcProxy_factoryCreate(celix_bundle_context_t* ctx, celix_log_helper_t* logHelper,
                                             FILE* logFile, remote_interceptors_handler_t* interceptorsHandler,
                                             const endpoint_description_t* endpointDesc,
                                             celix_rsa_send_request_fp sendRequest, void* sendRequestHandle,
                                             unsigned int serialProtoId, rsa_json_rpc_proxy_factory_t** proxyFactoryOut);

void rsaJsonRpcProxy_factoryDestroy(rsa_json_rpc_proxy_factory_t *proxyFactory);

long rsaJsonRpcProxy_factorySvcId(rsa_json_rpc_proxy_factory_t *proxyFactory);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_JSON_RPC_PROXY_IMPL_H_ */
