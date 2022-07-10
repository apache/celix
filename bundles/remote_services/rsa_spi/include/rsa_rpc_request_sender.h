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

#ifndef _RSA_RPC_REQUEST_SENDER_H_
#define _RSA_RPC_REQUEST_SENDER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <endpoint_description.h>
#include <celix_properties.h>
#include <celix_errno.h>
#include <sys/uio.h>

typedef celix_status_t (*send_request_fn)(void *sendFnHandle, const endpoint_description_t *endpointDesciption,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response);

typedef struct rsa_rpc_request_sender rsa_rpc_request_sender_t;

celix_status_t rsaRpcRequestSender_create(send_request_fn sendFn, void *sendFnHandle,
        rsa_rpc_request_sender_t **requestSenderOut);

void rsaRpcRequestSender_close(rsa_rpc_request_sender_t *requestSender);

void rsaRpcRequestSender_addRef(rsa_rpc_request_sender_t *requestSender);

bool rsaRpcRequestSender_release(rsa_rpc_request_sender_t *requestSender);

celix_status_t rsaRpcRequestSender_send(rsa_rpc_request_sender_t *requestSender,
        const endpoint_description_t *endpointDesciption, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response);



#ifdef __cplusplus
}
#endif

#endif /* _RSA_RPC_REQUEST_SENDER_H_ */
