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

#include <shm_pool.h>
#include <shm_pool_private.h>
#include <celix_threads.h>
#include <tlsf.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>


struct shm_pool{
    celix_thread_mutex_t mutex;// projects below
    int shmId;
    void *shmStartAddr;
    struct shm_pool_shared_info *sharedInfo;
    celix_thread_t shmHeartbeatThread;
    bool heartbeatThreadActive;
    celix_thread_cond_t heartbeatThreadStoped;
    tlsf_t allocator;
};

static void *shmPool_heartbeatThread(void *data);

celix_status_t shmPool_create(size_t size, shm_pool_t **pool) {
    celix_status_t status = CELIX_SUCCESS;
    size_t normalizedSharedInfoSize = (sizeof(struct shm_pool_shared_info) % sizeof(void *) == 0) ?
            sizeof(struct shm_pool_shared_info) : (sizeof(struct shm_pool_shared_info)+sizeof(void *))/sizeof(void *) * sizeof(void *);
    if (size <= tlsf_size() + normalizedSharedInfoSize || pool == NULL) {
        fprintf(stderr,"Shm pool: Shm size should be greater than %zu.\n", tlsf_size());
        status = CELIX_ILLEGAL_ARGUMENT;
        goto shm_size_invalid;
    }

    shm_pool_t *shmPool = (shm_pool_t *)malloc(sizeof(*shmPool));
    assert(shmPool != NULL);

    status = celixThreadMutex_create(&shmPool->mutex, NULL);
    if(status != CELIX_SUCCESS) {
        goto shm_pool_mutex_err;
    }
    /* Specify the IPC_PRIVATE constant as the key value to the `shmget` when creating the
     * IPC object, which always results in the creation of a new IPC object that is guaranteed to have a unique key.
     * And other process can use 'shmat' to attach relevant shared memory.
     */
    shmPool->shmId = shmget(IPC_PRIVATE, size, SHM_R | SHM_W);
    if (shmPool->shmId  == -1) {
        fprintf(stderr,"Shm pool: Error getting shm. %d.\n",errno);
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
        goto err_getting_shm;
    }
    shmPool->shmStartAddr = shmat(shmPool->shmId, NULL, 0);
    if (shmPool->shmStartAddr == NULL) {
        fprintf(stderr,"Shm pool: Error sttaching shm attach, %d.\n",errno);
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
        goto err_attaching_shm;
    }

    shmPool->sharedInfo = (struct shm_pool_shared_info *)shmPool->shmStartAddr;
    shmPool->sharedInfo->heartbeatCnt = 1;
    shmPool->sharedInfo->size = sizeof(struct shm_pool_shared_info);

    void *poolMem = shmPool->shmStartAddr + normalizedSharedInfoSize;
    shmPool->allocator = tlsf_create_with_pool(poolMem, size - normalizedSharedInfoSize);
    if (shmPool->allocator == NULL) {
        fprintf(stderr,"Shm pool: Error creating shm pool allocator.\n");
        status = CELIX_ILLEGAL_STATE;
        goto allocator_err;
    }

    status = celixThreadCondition_init(&shmPool->heartbeatThreadStoped, NULL);
    if (status != CELIX_SUCCESS) {
        fprintf(stderr,"Shm pool: Error creating stoped condition for heartbeat thread. %d.\n", status);
        goto stoped_cond_err;
    }
    shmPool->heartbeatThreadActive = true;
    status = celixThread_create(&shmPool->shmHeartbeatThread, NULL,
            shmPool_heartbeatThread, shmPool);
    if (status != CELIX_SUCCESS) {
        fprintf(stderr,"Shm pool: Error creating heartbeat thread. %d.\n", status);
        goto heartbeat_thread_err;
    }

    (void)shmctl(shmPool->shmId, IPC_RMID, NULL);
    *pool = shmPool;

    return CELIX_SUCCESS;

heartbeat_thread_err:
    (void)celixThreadCondition_destroy(&shmPool->heartbeatThreadStoped);
stoped_cond_err:
    tlsf_destroy(shmPool->allocator);
allocator_err:
    (void)shmdt(shmPool->shmStartAddr);
err_attaching_shm:
    (void)shmctl(shmPool->shmId, IPC_RMID, NULL);
err_getting_shm:
    (void)celixThreadMutex_destroy(&shmPool->mutex);
shm_pool_mutex_err:
    free(shmPool);
shm_size_invalid:
    return status;
}

int shmPool_getShmId(shm_pool_t *pool) {
    if (pool != NULL) {
        return pool->shmId;
    }
    return -1;
}

void shmPool_destroy(shm_pool_t *pool) {
    if (pool != NULL) {
        celixThreadMutex_lock(&pool->mutex);
        pool->heartbeatThreadActive = false;
        celixThreadMutex_unlock(&pool->mutex);
        celixThreadCondition_signal(&pool->heartbeatThreadStoped);
        celixThread_join(pool->shmHeartbeatThread, NULL);
        (void)celixThreadCondition_destroy(&pool->heartbeatThreadStoped);
        tlsf_destroy(pool->allocator);
        (void)shmdt(pool->shmStartAddr);
        celixThreadMutex_destroy(&pool->mutex);
        free(pool);
    }
    return ;
}

void *shmPool_malloc(shm_pool_t *pool, size_t size) {
    if (pool != NULL) {
        void *addr = NULL;
        celixThreadMutex_lock(&pool->mutex);
        addr = tlsf_malloc(pool->allocator, size);
        celixThreadMutex_unlock(&pool->mutex);
        return addr;
    }
    return NULL;
}

void shmPool_free(shm_pool_t *pool, void *ptr) {
    if (pool != NULL && ptr != NULL) {
        celixThreadMutex_lock(&pool->mutex);
        tlsf_free(pool->allocator, ptr);
        celixThreadMutex_unlock(&pool->mutex);
    }
    return ;
}

ssize_t shmPool_getMemoryOffset(shm_pool_t *pool, void *ptr) {
    if (pool != NULL && ptr != NULL) {
        return ptr - pool->shmStartAddr;
    }
    return -1;
}

static void *shmPool_heartbeatThread(void *data){
    shm_pool_t *pool = (shm_pool_t *)data;
    assert(pool != NULL);
    bool active = true;
    while (active) {
        celixThreadMutex_lock(&pool->mutex);
        int waitRet = 0;
        while (pool->heartbeatThreadActive && waitRet != ETIMEDOUT) {
            waitRet = celixThreadCondition_timedwaitRelative(&pool->heartbeatThreadStoped, &pool->mutex, SHM_HEART_BEAT_UPDATE_INTERVAL_IN_S, 0);
            assert(waitRet == 0 || waitRet == ETIMEDOUT);// pthread_cond_timedwait shall not return an error code of [EINTR]
        }
        pool->sharedInfo->heartbeatCnt++;
        active = pool->heartbeatThreadActive ;
        celixThreadMutex_unlock(&pool->mutex);
    }
    return NULL;
}
