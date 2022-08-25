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

#ifndef _SHM_CACHE_H_
#define _SHM_CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <celix_errno.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct shm_cache shm_cache_t;

/**
 * @brief Create shared memory cache
 *
 * @param[in] shmRdOnly Only read shared memory
 * @param[out] shmCache The shared memory cache instance
 * @return @see celix_errno.h
 */
celix_status_t shmCache_create(bool shmRdOnly, shm_cache_t **shmCache);

/**
 * @brief Destroy shared memory cache
 *
 * @param[in] shmCache The shared memory cache instance
 */
void shmCache_destroy(shm_cache_t *shmCache);

/**
 * @brief It will be called when shared memory is closed
 *
 * @param[in] handle Callback handle
 * @param[in] shmCache The shared memory cache instance
 * @param[in] shmId Closed shared memory id
 */
typedef void (*shmCache_shmPeerClosedCB)(void *handle, shm_cache_t *shmCache, int shmId);

/**
 * @brief Set the shared memory closed callback
 *
 * @param shmCache The shared memory cache instance
 * @param shmPeerClosedCB The shared memory closed callback,it will be called when shared memory is closed
 * @param closedCBHandle Callback handle
 */
void shmCache_setShmPeerClosedCB(shm_cache_t *shmCache, shmCache_shmPeerClosedCB shmPeerClosedCB, void *closedCBHandle);

/**
 * @brief Get shared memory address from shared memory cache.
 *
 * @param shmCache The shared memory cache instance
 * @param shmId Shared memory id
 * @param memoryOffset shared memory offset
 * @return Shared memory address/NULL
 */
void * shmCache_getMemoryPtr(shm_cache_t *shmCache, int shmId, ssize_t memoryOffset);

/**
 * @brief Give back shared memory to shared memory cache
 *
 * @param shmCache The shared memory cache instance
 * @param ptr Shared memory address
 */
void shmCache_releaseMemoryPtr(shm_cache_t *shmCache, void *ptr);

#ifdef __cplusplus
}
#endif

#endif //_SHM_CACHE_H_