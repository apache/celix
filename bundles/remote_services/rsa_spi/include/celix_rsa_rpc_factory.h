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

#ifndef CELIX_RSA_RPC_SERVICE_H_
#define CELIX_RSA_RPC_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <endpoint_description.h>
#include <celix_properties.h>
#include <celix_errno.h>
#include <sys/uio.h>

/**
 * @brief The RPC type indicates which protocol is used to (de)serialize the raw data.
 */
#define CELIX_RSA_RPC_TYPE_KEY "celix.remote.admin.rpc_type"

#define CELIX_RSA_RPC_FACTORY_NAME "celix_rsa_rpc_factory"
#define CELIX_RSA_RPC_FACTORY_VERSION "1.0.0"
#define CELIX_RSA_RPC_FACTORY_USE_RANGE "[1.0.0,2.0.0)"

/**
 * @brief The function type that can be used to send request to remote service endpoint.
 */
typedef celix_status_t (*celix_rsa_send_request_fp)(void *handle, const endpoint_description_t *endpointDescription,
                                                    celix_properties_t *metadata, const struct iovec *request, struct iovec *response);

/**
 * @brief The service use to create remote service endpoint and remote service proxy
 * @note It can be implemented by RPC bundles.
 */
typedef struct celix_rsa_rpc_factory {
    void *handle;/// The Service handle
     /**
      * @brief Create remote service endpoint proxy
      *
      * @param[in] handle Service handle
      * @param[in] endpointDesc The endpoint description of remote service
      * @param[in] sendRequest The function that can be used to send request to remote service endpoint
      * @param[in] sendRequestHandle The handle that will be passed to sendReqFn function when it is called.
      * @param[out] proxyId The identifier of remote service proxy
      * @return @see celix_errno.h
      */
    celix_status_t (*createProxy)(void *handle, const endpoint_description_t *endpointDesc, celix_rsa_send_request_fp sendRequest, void *sendRequestHandle, long *proxyId);
    /**
     * @brief Destroy remote service endpoint proxy
     *
     * @param[in] handle Service handle
     * @param[in] proxyId The identifier of remote service proxy
     */
    void (*destroyProxy)(void *handle, long proxyId);
    /**
     * @brief Create remote service endpoint
     *
     * @param[in] handle Service handle
     * @param[in] endpointDesc The endpoint description of remote service
     * @param[out] endpointId The identifier of the created service endpoint
     * @return @see celix_errno.h
     */
    celix_status_t (*createEndpoint)(void *handle, const endpoint_description_t *endpointDesc, long *endpointId);
    /**
     * @brief Destroy remote service endpoint
     *
     * @param[in] handle Service handle
     * @param[in] endpointId The identifier of the endpoint that is created by createEndpoint function.
     */
    void (*destroyEndpoint)(void *handle, long endpointId);
    /**
     * @brief Handle the request that from remote service proxy.
     * @param handle Service handle
     * @param endpointId The identifier of the endpoint that is created by createEndpoint function.
     * @param metadata The metadata, can be NULL.
     * @param request The request from remote service proxy
     * @param response  The response from remote service endpoint. The caller should use free function to free response memory
     * @return @see celix_errno.h
     */
    celix_status_t (*handleRequest)(void *handle, long endpointId, celix_properties_t *metadata, const struct iovec *request, struct iovec *response);
} celix_rsa_rpc_factory_t;


#ifdef __cplusplus
}
#endif

#endif /* CELIX_RSA_RPC_SERVICE_H_ */
