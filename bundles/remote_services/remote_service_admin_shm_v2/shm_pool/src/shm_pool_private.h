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

#ifndef _SHM_POOL_PRIVATE_H_
#define _SHM_POOL_PRIVATE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

#define SHM_HEART_BEAT_UPDATE_INTERVAL_IN_S 1

struct shm_pool_shared_info {
    size_t size;//The size of ‘struct shm_pool_shared_info‘.It is used to extend 'struct shm_pool_shared_info' in the future.
    uint64_t heartbeatCnt;//Keep alive for shared memory
};

#ifdef __cplusplus
}
#endif


#endif /* _SHM_POOL_PRIVATE_H_ */
