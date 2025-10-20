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
 * @note he service provider is RPC bundle(e.g., Celix::rsa_json_rpc), and the service consumer is RSA bundle(e.g., Celix::rsa_shm).
 *
 * In current design, remote service registrations are life cycle bound to the the Topology Manager that created them.(It can also reference the OSGi specification.(https://docs.osgi.org/specification/osgi.cmpn/7.0.0/service.remoteserviceadmin.html#i1736585)) And the endpoint/proxy creation and destruction are life cycle bound to the remote service registrations. Therefore, the createEndpoint/destroyEndpoint functions are called when remote service is exported/unexported, and the createProxy/destroyProxy functions are called when remote service is imported/unimported. The sequence of functions invoked is as follows:
 *
 * When local service is registered, or remote_service_admin_service_t is registered, or celix_rsa_rpc_factory_t is registered:
 * Topology Manager bundle -exportService-> Remote Service Admin bundle -createEndpoint-> RSA RPC bundle
 *
 * When local service is unregistered, or remote_service_admin_service_t is unregistered, or celix_rsa_rpc_factory_t is unregistered:
 * Topology Manager bundle -exportRegistration_close-> Remote Service Admin bundle -destroyEndpoint-> RSA RPC bundle
 *
 * When remote service endpoint description is discovered, or remote_service_admin_service_t is registered, or celix_rsa_rpc_factory_t is registered:
 * Topology Manager bundle -importService-> Remote Service Admin bundle -createProxy-> RSA RPC bundle
 *
 * When remote service is to be removed, or remote_service_admin_service_t is unregistered, or celix_rsa_rpc_factory_t is unregistered:
 * Topology Manager bundle -importRegistration_close-> Remote Service Admin bundle -destroyProxy-> RSA RPC bundle
 *
 * @warning The createProxy/destroyProxy functions and createEndpoint/destroyEndpoint functions must be called in pairs, and the celix_rsa_rpc_factory_t provider must not proactively destroy proxies/endpoints.
 */
typedef struct celix_rsa_rpc_factory {
    void *handle;/// The Service handle
     /**
      * @brief Create remote service endpoint proxy
      *
      * @param[in] handle Service handle
      * @param[in] endpointDesc The endpoint description of remote service
      * @param[in] sendRequest The function that can be used to send request to remote service endpoint. To celix_rsa_rpc_factory_t provider, it should not use the sendRequest function after destroyProxy is called, to celix_rsa_rpc_factory_t consumer, it should ensure that the sendRequest function can be used until destroyProxy is called.
      * @param[in] sendRequestHandle The handle that will be passed to sendReqFn function when it is called.
      * @param[out] proxyId The identifier of remote service proxy
      * @return @see celix_errno.h
      *
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
     * @param[in] handle Service handle
     * @param[in] endpointId The identifier of the endpoint that is created by createEndpoint function.
     * @param[in] metadata The metadata, can be NULL.
     * @param[in] request The request from remote service proxy
     * @param[out] response  The response from remote service endpoint. The caller should use free function to free response memory
     * @return @see celix_errno.h
     */
    celix_status_t (*handleRequest)(void *handle, long endpointId, celix_properties_t *metadata, const struct iovec *request, struct iovec *response);
} celix_rsa_rpc_factory_t;


#ifdef __cplusplus
}
#endif

#endif /* CELIX_RSA_RPC_SERVICE_H_ */
