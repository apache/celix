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

#ifndef _RSA_SHM_CLIENT_H_
#define _RSA_SHM_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <celix_log_helper.h>
#include <celix_api.h>
#include <sys/uio.h>



typedef struct rsa_shm_client_manager rsa_shm_client_manager_t;

celix_status_t rsaShmClientManager_create(celix_bundle_context_t *ctx,
        celix_log_helper_t *loghelper, rsa_shm_client_manager_t **clientManagerOut);

void rsaShmClientManager_destory(rsa_shm_client_manager_t *clientManager);

celix_status_t rsaShmClientManager_createOrAttachClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId);

void rsaShmClientManager_destroyOrDetachClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId);

celix_status_t rsaShmClientManager_sendMsgTo(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_SHM_CLIENT_H_ */
