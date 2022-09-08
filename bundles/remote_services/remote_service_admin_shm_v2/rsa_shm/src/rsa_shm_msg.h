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

#ifndef _RSA_SHM_MSG_H_
#define _RSA_SHM_MSG_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <stddef.h>
#include <sys/types.h>

typedef enum {
    REQUESTING = 0,
    REPLYING = 1,
    REPLIED = 2,
    ABEND = 3,//abnormal end
}rsa_shm_msg_state;

typedef struct rsa_shm_msg_control {
    size_t size;//The size of ‘struct rsa_shm_msg_control‘.It is used to extend 'struct rsa_shm_msg_control' in the future.
    rsa_shm_msg_state msgState;
    pthread_mutex_t lock;
    pthread_cond_t signal;
    size_t actualReplyedSize;
}rsa_shm_msg_control_t;

typedef struct rsa_shm_msg {
    size_t size;//The size of ‘struct rsa_shm_msg‘.It is used to extend 'struct rsa_shm_msg' in the future.
    int shmId;
    ssize_t ctrlDataOffset;
    size_t ctrlDataSize;
    ssize_t msgBodyOffset;//Message body includes metadata, request and reserve space
    size_t msgBodyTotalSize;//equal metadataSize + requestSize + reserve space size
    size_t metadataSize;
    size_t requestSize;
}rsa_shm_msg_t;

#ifdef __cplusplus
}
#endif

#endif /* _RSA_SHM_MSG_H_ */
