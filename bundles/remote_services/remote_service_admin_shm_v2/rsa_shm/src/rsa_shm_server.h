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

#ifndef _RSA_SHM_SERVER_H_
#define _RSA_SHM_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <shm_pool.h>
#include <celix_log_helper.h>
#include <sys/uio.h>

typedef struct rsa_shm_server rsa_shm_server_t;

typedef celix_status_t (*rsaShmServer_receiveMsgCB)(void *handle, rsa_shm_server_t *server,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response);


celix_status_t rsaShmServer_create(celix_bundle_context_t *ctx, const char *name, celix_log_helper_t *loghelper,
        rsaShmServer_receiveMsgCB receiveCB, void *revHandle, rsa_shm_server_t **shmServerOut);

void rsaShmServer_destroy(rsa_shm_server_t *server);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_SHM_SERVER_H_ */
