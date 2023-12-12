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

#ifndef _RSA_RPC_SERVICE_H_
#define _RSA_RPC_SERVICE_H_

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
#define RSA_RPC_TYPE_KEY "celix.remote.admin.rpc_type"

/**
 * @brief The prefix for RPC type value, and the RPC type value belongs to `service.exported.configs`.
 * A whole RPC type value as follows: celix.remote.admin.rpc_type.json.
 */
#define RSA_RPC_TYPE_PREFIX "celix.remote.admin.rpc_type."

#define RSA_RPC_FACTORY_NAME "rsa_rpc_factory"
#define RSA_RPC_FACTORY_VERSION "1.0.0"
#define RSA_RPC_FACTORY_USE_RANGE "[1.0.0,2.0.0)"

/**
 * @brief The service use to create remote service endpoint and remote service proxy
 * @note It can be implemented by RPC bundles.
 */
typedef struct rsa_rpc_factory {
    void *handle;/// The Service handle
     /**
      * @brief Create remote service endpoint proxy
      *
      * @param[in] handle Service handle
      * @param[in] endpointDesc The endpoint desciption of remote service
      * @param[in] requestSenderSvcId The service id of request sender service. It specifies the service that sends the endpoint proxy request.
      * @param[out] proxySvcId The service id of remote service proxy
      * @return @see celix_errno.h
      */
    celix_status_t (*createProxy)(void *handle, const endpoint_description_t *endpointDesc, long requestSenderSvcId, long *proxySvcId);
    /**
     * @brief Destroy remote service endpoint proxy
     *
     * @param[in] handle Service handle
     * @param[in] proxySvcId The service id of remote service proxy
     */
    void (*destroyProxy)(void *handle, long proxySvcId);
    /**
     * @brief Create remote service endpoint
     *
     * @param[in] handle Service handle
     * @param[in] endpointDesc The endpoint desciption of remote service
     * @param[out] requestHandlerSvcId The service id of request handler service.
     * It specifies the service that handle the endpoint proxy request. And it is registered by remote service endpoint.
     * @return @see celix_errno.h
     */
    celix_status_t (*createEndpoint)(void *handle, const endpoint_description_t *endpointDesc, long *requestHandlerSvcId);
    /**
     * @brief Destroy remote service endpoint
     *
     * @param[in] handle Service handle
     * @param[in] requestHandlerSvcId The service id of request handler service registered by the remote service endpoint.
     */
    void (*destroyEndpoint)(void *handle, long requestHandlerSvcId);
}rsa_rpc_factory_t;


#ifdef __cplusplus
}
#endif

#endif /* _RSA_RPC_SERVICE_H_ */
