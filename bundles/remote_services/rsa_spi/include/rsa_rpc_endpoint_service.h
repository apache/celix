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

#ifndef _RSA_RPC_ENDPOINT_SERVICE_H_
#define _RSA_RPC_ENDPOINT_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <celix_properties.h>
#include <celix_errno.h>
#include <sys/uio.h>

#define RSA_RPC_ENDPOINT_SERVICE_NAME "rsa_rpc_endpoint_service"
#define RSA_RPC_ENDPOINT_SERVICE_VERSION "1.0.0"
#define RSA_RPC_ENDPOINT_SERVICE_USE_RANGE "[1.0.0,2)"

typedef struct rsa_rpc_endpoint_service {
    void *handle;
    celix_status_t (*handleRequest)(void *handle, celix_properties_t **metadata, const struct iovec *request, struct iovec *response);
}rsa_rpc_endpoint_service_t;


#ifdef __cplusplus
}
#endif

#endif /* _RSA_RPC_ENDPOINT_SERVICE_H_ */
