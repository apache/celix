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

#include "shm_pool.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "sys_shm_ei.h"
#include "celix_errno.h"
#include <gtest/gtest.h>

class ShmPoolTestSuite : public ::testing::Test {
public:
    ShmPoolTestSuite() {

    }

    ~ShmPoolTestSuite() override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadCondition_init(nullptr, 0, 0);
        celix_ei_expect_shmget(nullptr, 0, 0);
        celix_ei_expect_shmat(nullptr, 0, nullptr);
    }
};

TEST_F(ShmPoolTestSuite, CreateDestroyShmPool) {
    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(shmPool != nullptr);
    EXPECT_LE(0, shmPool_getShmId(shmPool));
    shmPool_destroy(shmPool);
}

TEST_F(ShmPoolTestSuite, CreationFailedDueToInvalidArgument) {
    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(1, &shmPool);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    EXPECT_TRUE(shmPool == nullptr);

    status = shmPool_create(8192, NULL);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(ShmPoolTestSuite, CreateShmPoolFailed1) {
    shm_pool_t *shmPool = nullptr;
    celix_ei_expect_malloc((void *)&shmPool_create, 0, nullptr);
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(ShmPoolTestSuite, CreateShmPoolFailed2) {
    shm_pool_t *shmPool = nullptr;
    celix_ei_expect_celixThreadMutex_create((void *)&shmPool_create, 0, CELIX_ENOMEM);
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(ShmPoolTestSuite, CreateShmPoolFailed3) {
    shm_pool_t *shmPool = nullptr;
    celix_ei_expect_shmget((void *)&shmPool_create, 0, -1);
    errno = EACCES;
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno), status);
    errno = 0;
}

TEST_F(ShmPoolTestSuite, CreateShmPoolFailed4) {
    shm_pool_t *shmPool = nullptr;
    celix_ei_expect_shmat((void *)&shmPool_create, 0, nullptr);
    errno = ENOMEM;
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno), status);
    errno = 0;
}

TEST_F(ShmPoolTestSuite, CreateShmPoolFailed5) {
    shm_pool_t *shmPool = nullptr;
    celix_ei_expect_celixThreadCondition_init((void *)&shmPool_create, 0, CELIX_ENOMEM);
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(ShmPoolTestSuite, CreateShmPoolFailed6) {
    shm_pool_t *shmPool = nullptr;
    celix_ei_expect_celixThread_create((void *)&shmPool_create, 0, CELIX_BUNDLE_EXCEPTION);
    celix_status_t status = shmPool_create(10240, &shmPool);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(ShmPoolTestSuite, DestroyForNullPool) {
    shmPool_destroy(nullptr);
}

TEST_F(ShmPoolTestSuite, GetShmIdForNullPool) {
    EXPECT_EQ(-1, shmPool_getShmId(nullptr));
}

TEST_F(ShmPoolTestSuite, MallocFreeMemory) {
    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(8192, &shmPool);
    EXPECT_EQ(CELIX_SUCCESS, status);
    void *addr = shmPool_malloc(shmPool, 128);
    EXPECT_TRUE(addr != NULL);
    EXPECT_LT(0, shmPool_getMemoryOffset(shmPool, addr));
    shmPool_free(shmPool, addr);
    shmPool_destroy(shmPool);
}

TEST_F(ShmPoolTestSuite, MallocMemoryFailedDueToRequestingSizeTooLarge) {
    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(8192, &shmPool);
    EXPECT_EQ(CELIX_SUCCESS, status);
    void *addr = shmPool_malloc(shmPool, 10240);
    EXPECT_TRUE(addr == NULL);
    shmPool_destroy(shmPool);
}

TEST_F(ShmPoolTestSuite, MallocMemoryForNullPool) {
    void *addr = shmPool_malloc(nullptr, 128);
    EXPECT_TRUE(addr == NULL);
}

TEST_F(ShmPoolTestSuite, FreeMemoryForNullPool) {
    shmPool_free(nullptr, nullptr);
}

TEST_F(ShmPoolTestSuite, GetMemoryOffsetForNullPtrOrNullPool) {
    EXPECT_EQ(-1, shmPool_getMemoryOffset(nullptr, nullptr));

    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(8192, &shmPool);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(-1, shmPool_getMemoryOffset(shmPool, nullptr));
    shmPool_destroy(shmPool);
}

TEST_F(ShmPoolTestSuite, FreeNullPtr) {
    shm_pool_t *shmPool = nullptr;
    celix_status_t status = shmPool_create(8192, &shmPool);
    EXPECT_EQ(CELIX_SUCCESS, status);
    shmPool_free(shmPool, nullptr);
    shmPool_destroy(shmPool);
}

