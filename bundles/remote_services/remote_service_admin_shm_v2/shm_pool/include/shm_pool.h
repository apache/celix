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

#ifndef _SHM_POOL_H_
#define _SHM_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "celix_errno.h"
#include "celix_cleanup.h"
#include <stddef.h>
#include <sys/types.h>

typedef struct shm_pool shm_pool_t;


/**
 * @brief Create a shared memory pool
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] size Shared memory size, it should be greater than or equal to 8192
 * @param[out] pool The shared memory pool instance
 * @return @see celix_errno.h
 */
celix_status_t shmPool_create(size_t size, shm_pool_t **pool);

/**
 * @brief Get the shared memory id of shared memory pool
 *
 * @param[in] pool The shared memory pool instance
 * @return Shared memory id/-1
 */
int shmPool_getShmId(shm_pool_t *pool);

/**
 * @brief Destroy shared memory pool
 *
 * @param[in] pool The shared memory pool instance
 */
void shmPool_destroy(shm_pool_t *pool);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(shm_pool_t, shmPool_destroy)

/**
 * @brief Allocate memory from shared memory pool
 *
 * @param[in] pool The shared memory pool instance
 * @param[in] size Allocating memory size
 * @return Shared memory address/NULL
 */
void *shmPool_malloc(shm_pool_t *pool, size_t size);

/**
 * @brief Free shared memory
 *
 * @param[in] pool The shared memory pool instance
 * @param[in] ptr Shared memory address
 */
void shmPool_free(shm_pool_t *pool, void *ptr);

/**
 * @brief Get the memory offset in shared memory
 *
 * @param[in] pool The shared memory pool instance
 * @param[in] ptr Shared memory address
 * @return Shared memory offset
 */
ssize_t shmPool_getMemoryOffset(shm_pool_t *pool, void *ptr);

/**
 * @brief Scoped guard for shared memory pool allocation.
 */
typedef struct celix_shm_pool_alloc_guard {
    void* ptr;
    shm_pool_t* pool;
} celix_shm_pool_alloc_guard_t;

/**
 * @brief Initialize a shared memory pool allocation guard.
 * @param [in] addr Start address of the allocated memory.
 * @param [in] pool The shared memory pool.
 * @return An initialized guard.
 */
static CELIX_UNUSED inline celix_shm_pool_alloc_guard_t celix_shmPoolAllocGuard_init(void* addr, shm_pool_t* pool) {
    return (celix_shm_pool_alloc_guard_t) {
        .ptr = addr,
        .pool = pool
    };
}

/**
 * @brief Deinitialize a shared memory pool allocation guard.
 * The guard will free the allocated memory.
 * @param [in] guard The guard to deinitialize.
 */
static CELIX_UNUSED inline void celix_shmPoolAllocGuard_deinit(celix_shm_pool_alloc_guard_t* guard) {
    shmPool_free(guard->pool, guard->ptr);
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_shm_pool_alloc_guard_t, celix_shmPoolAllocGuard_deinit)

#ifdef __cplusplus
}
#endif

#endif //_SHM_POOL_H_