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

#ifndef _RSA_REQUEST_SENDER_SERVICE_H_
#define _RSA_REQUEST_SENDER_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <endpoint_description.h>
#include <celix_properties.h>
#include <celix_errno.h>
#include <sys/uio.h>

#define CELIX_RSA_REQUEST_SENDER_SERVICE_NAME "rsa_request_sender_service"
#define CELIX_RSA_REQUEST_SENDER_SERVICE_VERSION "1.0.0"
#define CELIX_RSA_REQUEST_SENDER_SERVICE_USE_RANGE "[1.0.0,2)"

/**
 * @brief The service send RPC request
 * @note It can be implemented by RSA bundles, and called by RPC bundles.
 *
 */
typedef struct rsa_request_sender_service {
    void *handle;/// The Service handle
    /**
     * @brief Send the request that from remote service proxy.
     * @note  It will be a remote, synchronized and blocking call.
     * @param[in] handle Service handle
     * @param[in] endpointDesciption The endpoint desciption of remote service
     * @param[in,out] metadata The metadata, can be NULL.
     * @param[in] request The request for the remote service endpoint
     * @param[out] response The response received from remote service endpoint. The caller should use free function to free response memory
     * @return @see celix_errno.h
     */
    celix_status_t (*sendRequest)(void *handle, const endpoint_description_t *endpointDesciption, celix_properties_t *metadata, const struct iovec *request, struct iovec *response);
}rsa_request_sender_service_t;


#ifdef __cplusplus
}
#endif

#endif /* _RSA_REQUEST_SENDER_SERVICE_H_ */
