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

#ifndef _RSA_SHM_CONSTANTS_H_
#define _RSA_SHM_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RSA Configuration type for shared memory
 */
#define RSA_SHM_CONFIGURATION_TYPE          "celix.remote.admin.shm"
/**
 * @brief The unix domain socket server name, which is used to create an abstract socket. The abstract socket is used to transfer shared memory address information.
 */
#define RSA_SHM_SERVER_NAME_KEY             "celix.remote.admin.shm.server_name"
/**
 * @brief The serializer type of RSA_SHM_CONFIGURATION_TYPE. The value should be associated with RSA_RPC_TYPE_KEY of rsa_rpc_factory_t.
 */
#define RSA_SHM_RPC_TYPE_KEY                "celix.remote.admin.shm.rpc_type"

/**
 * @brief A property of RsaShm bundle that indicates the shared memory pool size.
 *  Its value should be greater than or equal to 8192, because the memory pool ctrl block(control_t) size is 6536 bytes.
 *
 */
#define RSA_SHM_MEMORY_POOL_SIZE_KEY "CELIX_RSA_SHM_POOL_SIZE"
/**
 * @brief Shared memory pool default size
 *
 */
#define RSA_SHM_MEMORY_POOL_SIZE_DEFAULT (1024*256)

/**
 * @brief A property of RsaShm bundle that indicates the timeout of remote service invocation.
 *
 */
#define RSA_SHM_MSG_TIMEOUT_KEY "CELIX_RSA_SHM_MSG_TIMEOUT"
/**
 * @brief The default timeout of remote service invocation.
 *
 */
#define RSA_SHM_MSG_TIMEOUT_DEFAULT_IN_S 30

/**
 * @brief A property of RsaShm bundle that indicates the maximum concurrent invocations of the same service.
 * If there are more concurrent invocations than its value,  service invocation will fail.
 *
 */
#define RSA_SHM_MAX_CONCURRENT_INVOCATIONS_KEY "CELIX_RSA_SHM_MAX_CONCURRENT_INVOCATIONS_NUM"

/**
 * @brief The default value of the maximum concurrent invocations, If property RSA_SHM_MAX_CONCURRENT_INVOCATIONS_KEY does not exist, the default value is used
 *
 */
#define RSA_SHM_MAX_CONCURRENT_INVOCATIONS_DEFAULT 32

/**
 * @brief The maximum failures of service invocation.
 * If there are more invocation failures than this value, the service invocation will fail for the next 'RSA_SHM_MAX_SVC_BREAKED_TIME_IN_S' seconds
 *
 */
#define RSA_SHM_MAX_INVOKED_SVC_FAILURES 15
/**
 * @brief The breaked time of service invocation. After this time, the service invocation will return to normal.
 *
 */
#define RSA_SHM_MAX_SVC_BREAKED_TIME_IN_S 60

/**
 * @brief Estimated remote service response default size
 *
 */
#define ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT 512

/**
 * @brief Default RPC type used by shared memory RSA
 *
 */
#define RSA_SHM_RPC_TYPE_DEFAULT "celix.remote.admin.rpc_type.json"

/**
 * @brief The maximum len of shm server name.
 * @note The corresponding client name will be "{serverName}_cl.%ld",
 * and we will use it to create an abstract socket. Therefore, we should set the maximum len of the server name to (sizeof(((struct sockaddr_un*)0)->sun_path)-1-4-19).
 */
#define MAX_RSA_SHM_SERVER_NAME_SIZE 84

#ifdef __cplusplus
}
#endif

#endif /* _RSA_SHM_CONSTANTS_H_ */
