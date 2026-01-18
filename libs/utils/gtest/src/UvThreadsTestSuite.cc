//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.

#include "celix_uv_cleanup.h"

#include <atomic>
#include <gtest/gtest.h>

class UvThreadsTestSuite : public ::testing::Test {
};

static void uvThreadIncrement(void* data) {
    auto* counter = static_cast<std::atomic<int>*>(data);
    counter->fetch_add(1);
}

TEST_F(UvThreadsTestSuite, ThreadAutoCleanupTest) {
    std::atomic<int> counter{0};
    {
        celix_auto(uv_thread_t) thread;
        ASSERT_EQ(0, uv_thread_create(&thread, uvThreadIncrement, &counter));
    } //thread out of scope -> join
    EXPECT_EQ(1, counter.load());
}

TEST_F(UvThreadsTestSuite, MutexGuardTest) {
    uv_mutex_t mutex;
    ASSERT_EQ(0, uv_mutex_init(&mutex));
    celix_autoptr(uv_mutex_t) mutexCleanup = &mutex;

    {
        celix_auto(celix_uv_mutex_lock_guard_t) guard = celixUvMutexLockGuard_init(&mutex);
        EXPECT_NE(0, uv_mutex_trylock(&mutex));
    } //guard out of scope -> unlock

    ASSERT_EQ(0, uv_mutex_trylock(&mutex));
    uv_mutex_unlock(&mutex);
}

TEST_F(UvThreadsTestSuite, MutexStealTest) {
    uv_mutex_t mutex;
    ASSERT_EQ(0, uv_mutex_init(&mutex));
    celix_autoptr(uv_mutex_t) mutexCleanup = &mutex;
    celix_steal_ptr(mutexCleanup);
    uv_mutex_destroy(&mutex);
}

TEST_F(UvThreadsTestSuite, RwlockGuardTest) {
    uv_rwlock_t lock;
    ASSERT_EQ(0, uv_rwlock_init(&lock));
    celix_autoptr(uv_rwlock_t) lockCleanup = &lock;

#ifndef __APPLE__ //On MacOs uv_rwlock_tryrdlock and uv_rwlock_trywrlock on write locked lock results in SIGABRT
    {
        celix_auto(celix_uv_rwlock_wlock_guard_t) writeGuard = celixUvRwlockWlockGuard_init(&lock);
        EXPECT_NE(0, uv_rwlock_tryrdlock(&lock));
        EXPECT_NE(0, uv_rwlock_trywrlock(&lock));
    } //guard out of scope -> unlock write lock
#endif

    {
        celix_auto(celix_uv_rwlock_rlock_guard_t) readGuard = celixUvRwlockRlockGuard_init(&lock);

        EXPECT_EQ(0, uv_rwlock_tryrdlock(&lock));
        uv_rwlock_rdunlock(&lock);

        EXPECT_NE(0, uv_rwlock_trywrlock(&lock));
    } //guard out of scope -> unlock read lock

    ASSERT_EQ(0, uv_rwlock_trywrlock(&lock));
    uv_rwlock_wrunlock(&lock);
}

TEST_F(UvThreadsTestSuite, RwlockStealdTest) {
    uv_rwlock_t lock;
    ASSERT_EQ(0, uv_rwlock_init(&lock));
    celix_autoptr(uv_rwlock_t) lockCleanup = &lock;
    celix_steal_ptr(lockCleanup);
    uv_rwlock_destroy(&lock);
}

TEST_F(UvThreadsTestSuite, ConditionAutoCleanupTest) {
    uv_cond_t cond;
    ASSERT_EQ(0, uv_cond_init(&cond));
    celix_autoptr(uv_cond_t) condCleanup = &cond;
}

TEST_F(UvThreadsTestSuite, ConditionStealTest) {
    uv_cond_t cond;
    ASSERT_EQ(0, uv_cond_init(&cond));
    celix_autoptr(uv_cond_t) condCleanup = &cond;
    celix_steal_ptr(condCleanup);
    uv_cond_destroy(&cond);
}

TEST_F(UvThreadsTestSuite, LocalThreadStorageKeyAutoCleanupTest) {
    uv_key_t key;
    ASSERT_EQ(0, uv_key_create(&key));
    celix_autoptr(uv_key_t) keyCleanup = &key;
}

TEST_F(UvThreadsTestSuite, LocalThreadStorageKeyStealTest) {
    uv_key_t key;
    ASSERT_EQ(0, uv_key_create(&key));
    celix_autoptr(uv_key_t) keyCleanup = &key;
    celix_steal_ptr(keyCleanup);
    uv_key_delete(&key);
}
