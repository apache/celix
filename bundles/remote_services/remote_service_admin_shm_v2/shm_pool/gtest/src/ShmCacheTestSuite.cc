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
#include <shm_pool.h>
#include <gtest/gtest.h>
#include <string.h>

class ShmCacheTestSuite : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
      if (shmPool == nullptr) {
          celix_status_t status = shmPool_create(8192, &shmPool);
          EXPECT_EQ(CELIX_SUCCESS, status);
          EXPECT_TRUE(shmPool != nullptr);
          shmId = shmPool_getShmId(shmPool);
          EXPECT_TRUE(shmId > 0);
      }
    }

    static void TearDownTestSuite() {
        shmPool_destroy(shmPool);
        shmPool = nullptr;
    }
    static shm_pool_t* shmPool;
    static int shmId;
};

shm_pool_t* ShmCacheTestSuite::shmPool = nullptr;
int ShmCacheTestSuite::shmId = -1;

static void shmPeerClosedCallback(void *handle, shm_cache_t *shmCache, int shmId) {
    (void)handle;
    EXPECT_TRUE(shmCache != nullptr);
    EXPECT_TRUE(shmId > 0);
}

TEST_F(ShmCacheTestSuite, CreateDestroyShmCache) {
    shm_cache_t *shmCache = nullptr;
    celix_status_t status = shmCache_create(false, &shmCache);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(shmCache != nullptr);
    shmCache_setShmPeerClosedCB(shmCache, shmPeerClosedCallback, nullptr);
    shmCache_destroy(shmCache);
}

TEST_F(ShmCacheTestSuite, CreationFailedDueToInvalidArgument) {
    celix_status_t status = shmCache_create(false, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(ShmCacheTestSuite, SetClosedCallbackForNullShmCache) {
    shmCache_setShmPeerClosedCB(nullptr, shmPeerClosedCallback, nullptr);
}

TEST_F(ShmCacheTestSuite, DestroyForNullShmCache) {
    shmCache_destroy(nullptr);
}

TEST_F(ShmCacheTestSuite, GetMemoryPtr) {
    shm_cache_t *shmCache = nullptr;
    celix_status_t status = shmCache_create(false, &shmCache);
    EXPECT_EQ(CELIX_SUCCESS, status);

    void *mem1 = shmPool_malloc(shmPool, 128);
    EXPECT_TRUE(mem1 != nullptr);
    ssize_t memOffset1 = shmPool_getMemoryOffset(shmPool, mem1);
    EXPECT_LT(0, memOffset1);
    strcpy((char *)mem1, (const char*)"hello");
    void *mem2 = shmPool_malloc(shmPool, 128);
    EXPECT_TRUE(mem2 != nullptr);
    ssize_t memOffset2 = shmPool_getMemoryOffset(shmPool, mem2);
    EXPECT_LT(0, memOffset2);
    strcpy((char *)mem2, (const char*)"shm cache");

    void *addr1 = shmCache_getMemoryPtr(shmCache, shmId, memOffset1);
    EXPECT_TRUE(addr1 != nullptr);
    EXPECT_STREQ((const char*)"hello", (const char*)addr1);
    shmCache_releaseMemoryPtr(shmCache, addr1);
    void *addr2 = shmCache_getMemoryPtr(shmCache, shmId, memOffset2);
    EXPECT_TRUE(addr2 != nullptr);
    EXPECT_STREQ((const char*)"shm cache", (const char*)addr2);
    shmCache_releaseMemoryPtr(shmCache, addr2);

    shmPool_free(shmPool, mem2);
    shmPool_free(shmPool, mem1);

    shmCache_destroy(shmCache);
}

TEST_F(ShmCacheTestSuite, GetMemoryPtrFailedDueToInvalidArgument) {
    shm_cache_t *shmCache = nullptr;
    celix_status_t status = shmCache_create(false, &shmCache);
    EXPECT_EQ(CELIX_SUCCESS, status);

    void *mem = shmPool_malloc(shmPool, 128);
    EXPECT_TRUE(mem != nullptr);
    ssize_t memOffset = shmPool_getMemoryOffset(shmPool, mem);
    EXPECT_LT(0, memOffset);

    void *addr = shmCache_getMemoryPtr(shmCache, 1, memOffset);
    EXPECT_TRUE(addr == nullptr);
    addr = shmCache_getMemoryPtr(shmCache, -1, memOffset);
    EXPECT_TRUE(addr == nullptr);
    addr = shmCache_getMemoryPtr(shmCache, shmId, -1);
    EXPECT_TRUE(addr == nullptr);
    addr = shmCache_getMemoryPtr(nullptr, shmId, memOffset);
    EXPECT_TRUE(addr == nullptr);

    shmPool_free(shmPool, mem);
    shmCache_destroy(shmCache);
}

TEST_F(ShmCacheTestSuite, EvictInactiveShmCacheBlock) {
    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(8192, &shmPool);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(shmPool != nullptr);
    int shmId = shmPool_getShmId(shmPool);
    EXPECT_TRUE(shmId > 0);

    shm_cache_t *shmCache = nullptr;
    status = shmCache_create(false, &shmCache);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(shmCache != nullptr);
    shmCache_setShmPeerClosedCB(shmCache, shmPeerClosedCallback, nullptr);

    void *mem = shmPool_malloc(shmPool, 128);
    EXPECT_TRUE(mem != nullptr);
    ssize_t memOffset = shmPool_getMemoryOffset(shmPool, mem);
    EXPECT_LT(0, memOffset);

    void *addr = shmCache_getMemoryPtr(shmCache, shmId, memOffset);
    EXPECT_TRUE(addr != nullptr);
    shmCache_releaseMemoryPtr(shmCache, addr);

    shmPool_free(shmPool, mem);
    shmPool_destroy(shmPool);
    sleep(5);// TODO mock monitonic time

    shmCache_destroy(shmCache);
}



