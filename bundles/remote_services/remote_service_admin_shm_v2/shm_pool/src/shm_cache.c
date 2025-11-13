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

#include <shm_cache.h>
#include <shm_pool_private.h>
#include <celix_errno.h>
#include <celix_array_list.h>
#include <celix_long_hash_map.h>
#include <celix_threads.h>
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"
#include <stdlib.h>
#include <assert.h>
#include <sys/shm.h>

typedef struct shm_cache_block {
    int shmId;
    void *shmStartAddr;
    struct shm_pool_shared_info *sharedInfo;
    uint64_t lastHeartbeatCnt;
    unsigned int refCnt;
    size_t maxOffset;
}shm_cache_block_t;

struct shm_cache{
    bool shmRdOnly;
    celix_thread_mutex_t mutex;// projects below
    celix_long_hash_map_t *shmCacheBlocks;
    celix_thread_t shmWatcherThread;
    bool watcherActive;
    celix_thread_cond_t watcherStopped;
    shmCache_shmPeerClosedCB shmPeerClosedCB;
    void *closedCbHandle;
};

static void * shmCache_WatcherThread(void *data);

celix_status_t shmCache_create(bool shmRdOnly, shm_cache_t **shmCache) {
    celix_status_t status = CELIX_SUCCESS;
    if (shmCache == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autofree shm_cache_t *cache = (shm_cache_t *)malloc(sizeof(shm_cache_t));
    if (cache == NULL) {
        return CELIX_ENOMEM;
    }

    cache->shmRdOnly = shmRdOnly;
    cache->shmPeerClosedCB = NULL;
    cache->closedCbHandle = NULL;

    status = celixThreadMutex_create(&cache->mutex, NULL);
    if(status != CELIX_SUCCESS) {
        celix_err_pushf("Shm cache: Error creating cache mutux. %d.\n", status);
        return status;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &cache->mutex;
    celix_autoptr(celix_long_hash_map_t) shmCacheBlocks = cache->shmCacheBlocks = celix_longHashMap_create();
    assert(cache->shmCacheBlocks != NULL);

    status = celixThreadCondition_init(&cache->watcherStopped, NULL);
    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Shm cache: Error creating stoped condition for cache watcher. %d.\n", status);
        return status;
    }
    celix_autoptr(celix_thread_cond_t) watcherStopped = &cache->watcherStopped;
    cache->watcherActive = true;
    status = celixThread_create(&cache->shmWatcherThread, NULL,
            shmCache_WatcherThread, cache);
    if (status != CELIX_SUCCESS) {
        celix_err_pushf("Shm cache: Error creating cache watcher. %d.\n", status);
        return status;
    }

    celix_steal_ptr(watcherStopped);
    celix_steal_ptr(shmCacheBlocks);
    celix_steal_ptr(mutex);
    *shmCache = celix_steal_ptr(cache);

    return CELIX_SUCCESS;
}

void shmCache_setShmPeerClosedCB(shm_cache_t *shmCache, shmCache_shmPeerClosedCB shmPeerClosedCB, void *closedCBHandle) {
    if (shmCache != NULL && shmPeerClosedCB != NULL) {
        celixThreadMutex_lock(&shmCache->mutex);
        shmCache->shmPeerClosedCB = shmPeerClosedCB;
        shmCache->closedCbHandle = closedCBHandle;
        celixThreadMutex_unlock(&shmCache->mutex);
    }
    return ;
}

static shm_cache_block_t * shmCache_createBlock(shm_cache_t *shmCache, int shmId) {
    shm_cache_block_t *shmBlock = NULL;
    void *shmStartAddr = NULL;
    if (shmCache->shmRdOnly) {
        shmStartAddr = shmat(shmId, NULL, SHM_RDONLY);
    } else {
        shmStartAddr = shmat(shmId, NULL, 0);
    }
    if (shmStartAddr != (void*)-1) {
        shmBlock = (shm_cache_block_t *)malloc(sizeof(shm_cache_block_t));
        assert(shmBlock != NULL);
        shmBlock->shmId = shmId;
        shmBlock->shmStartAddr = shmStartAddr;
        shmBlock->sharedInfo = (struct shm_pool_shared_info *)shmStartAddr;
        shmBlock->lastHeartbeatCnt = 0;
        shmBlock->refCnt = 1;
        shmBlock->maxOffset = 0;
    } else {
        celix_err_pushf("Shm cache: Error attaching shared memory for shmid %d. %d.\n", shmId, errno);
    }
    return shmBlock;
}

static void shmCache_destroyBlock(shm_cache_t *shmCache, shm_cache_block_t *shmBlock) {
    (void)shmCache;//unused
    shmdt(shmBlock->shmStartAddr);
    free(shmBlock);
    return ;
}

void * shmCache_getMemoryPtr(shm_cache_t *shmCache, int shmId, ssize_t memoryOffset) {
    void *ptr = NULL;
    if (shmCache != NULL && shmId >= 0 && memoryOffset > 0) {
        celixThreadMutex_lock(&shmCache->mutex);
        shm_cache_block_t *shmBlock = celix_longHashMap_get(shmCache->shmCacheBlocks, shmId);
        if (shmBlock != NULL) {
            shmBlock->refCnt++;
        } else {
            shmBlock = shmCache_createBlock(shmCache, shmId);
            if (shmBlock != NULL) {
                celix_longHashMap_put(shmCache->shmCacheBlocks, shmId, shmBlock);
            }
        }

        if (shmBlock != NULL) {
            if (shmBlock->maxOffset < memoryOffset) {
                shmBlock->maxOffset = memoryOffset;
            }
            ptr = shmBlock->shmStartAddr + memoryOffset;
        }
        celixThreadMutex_unlock(&shmCache->mutex);
    }
    return ptr;
}

void shmCache_releaseMemoryPtr(shm_cache_t *shmCache, void *ptr) {
    if (shmCache != NULL && ptr != NULL) {
        celixThreadMutex_lock(&shmCache->mutex);
        CELIX_LONG_HASH_MAP_ITERATE(shmCache->shmCacheBlocks, iter) {
            shm_cache_block_t *shmBlock = (shm_cache_block_t *)iter.value.ptrValue;
            if (shmBlock->shmStartAddr <= ptr && ptr <= shmBlock->shmStartAddr + shmBlock->maxOffset ) {
                if (shmBlock->refCnt != 0) {
                    shmBlock->refCnt--;
                } else {
                    assert(0);//should never happen
                }
                break;
            }
        }
        celixThreadMutex_unlock(&shmCache->mutex);
    }
    return;
}

void shmCache_destroy(shm_cache_t *shmCache) {
    if (shmCache != NULL) {
        celixThreadMutex_lock(&shmCache->mutex);
        shmCache->watcherActive = false;
        celixThreadMutex_unlock(&shmCache->mutex);
        celixThreadCondition_signal(&shmCache->watcherStopped);
        celixThread_join(shmCache->shmWatcherThread, NULL);
        celixThreadCondition_destroy(&shmCache->watcherStopped);
        CELIX_LONG_HASH_MAP_ITERATE(shmCache->shmCacheBlocks, iter) {
            shm_cache_block_t *shmBlock = (shm_cache_block_t *)iter.value.ptrValue;
            assert(shmBlock->refCnt == 0);//should be 0, otherwise memory leak
            shmCache_destroyBlock(shmCache, shmBlock);
        }
        celix_longHashMap_destroy(shmCache->shmCacheBlocks);
        celixThreadMutex_destroy(&shmCache->mutex);
        free(shmCache);
    }
    return ;
}

static void * shmCache_WatcherThread(void *data) {
    shm_cache_t *shmCache = (shm_cache_t *)data;
    assert(shmCache !=  NULL);
    bool active = true;
    celix_array_list_t *preEvictedBlocks = celix_arrayList_create();
    celix_array_list_t *evictedBlocks = celix_arrayList_create();
    assert(preEvictedBlocks != NULL);
    while (active) {
        celixThreadMutex_lock(&shmCache->mutex);
        int waitRet = 0;
        while (shmCache->watcherActive && waitRet != ETIMEDOUT) {
            // pthread_cond_timedwait shall not return an error code of [EINTR]，refer https://man7.org/linux/man-pages/man3/pthread_cond_timedwait.3p.html
            waitRet = celixThreadCondition_timedwaitRelative(&shmCache->watcherStopped, &shmCache->mutex, 2 * SHM_HEART_BEAT_UPDATE_INTERVAL_IN_S, 0);
        }
        active = shmCache->watcherActive;
        if (waitRet != ETIMEDOUT) {
            celixThreadMutex_unlock(&shmCache->mutex);
            continue;
        }

        CELIX_LONG_HASH_MAP_ITERATE(shmCache->shmCacheBlocks, iter) {
            shm_cache_block_t *shmBlock = (shm_cache_block_t *)iter.value.ptrValue;
            if (shmBlock->sharedInfo->heartbeatCnt == shmBlock->lastHeartbeatCnt) {
                if (shmBlock->refCnt == 0) {
                    celix_arrayList_add(evictedBlocks, shmBlock);
                } else {
                    celix_arrayList_add(preEvictedBlocks, shmBlock);
                }
            } else {
                shmBlock->lastHeartbeatCnt = shmBlock->sharedInfo->heartbeatCnt;
            }
        }

        // Close shared memory cache blocks that them are inactive.
        size_t size = celix_arrayList_size(evictedBlocks);
        for (int i = 0; i < size; ++i) {
            shm_cache_block_t *shmBlock = celix_arrayList_get(evictedBlocks, i);
            celix_longHashMap_remove(shmCache->shmCacheBlocks, shmBlock->shmId);
            shmCache_destroyBlock(shmCache, shmBlock);
        }
        celix_arrayList_clear(evictedBlocks);

        size = celix_arrayList_size(preEvictedBlocks);
        for (int i = 0; i < size; ++i) {
            shm_cache_block_t *shmBlock = celix_arrayList_get(preEvictedBlocks, i);
            if (shmCache->shmPeerClosedCB != NULL) {
                shmCache->shmPeerClosedCB(shmCache->closedCbHandle, shmCache, shmBlock->shmId);
            }
        }
        celix_arrayList_clear(preEvictedBlocks);

        celixThreadMutex_unlock(&shmCache->mutex);
    }

    celix_arrayList_destroy(evictedBlocks);
    celix_arrayList_destroy(preEvictedBlocks);

    return NULL;
}