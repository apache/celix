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


#define RSA_SHM_CONFIGURATION_TYPE "celix.remote.admin.shm"

#define RSA_SHM_SERVER_NAME_KEY "rsaShmServerName"

//The memory pool size should be greater than 6536
#define RSA_SHM_MEMORY_POOL_SIZE_KEY "rsaShmPoolSize"

#define RSA_SHM_MEMORY_POOL_SIZE_DEFAULT (1024*256)

#define RSA_SHM_MSG_TIMEOUT_KEY "rsaShmMsgTimeout"

#define RSA_SHM_MSG_TIMEOUT_DEFAULT_IN_S 30

/**
 * @brief A property of RsaShm bundle which indicate the maximum concurrent invocations of the same service
 *
 */
#define RSA_SHM_MAX_CONCURRENT_INVOCATIONS_KEY "rsaShmCctIvNum"

/**
 * @brief The default value of the maximum concurrent invocations, If property RSA_SHM_MAX_CONCURRENT_INVOCATIONS_KEY does not exist, the default value is used
 *
 */
#define RSA_SHM_MAX_CONCURRENT_INVOCATIONS_DEFAULT 32

#define RSA_SHM_MAX_INVOKED_SVC_FAILURES 15

#define RSA_SHM_MAX_SVC_BREAKED_TIME_IN_S 60

#define ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT 512

#define RSA_SHM_RPC_TYPE_DEFAULT "rsa_json_rpc"

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
