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

#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include <gtest/gtest.h>
#include <stdint.h>

class CelixUtilsCleanupTestSuite : public ::testing::Test {
};

TEST_F(CelixUtilsCleanupTestSuite, AutoFreeTest) {
    celix_autofree char* p = NULL;
    celix_autofree char* p2 = NULL;
    celix_autofree char* alwaysNull = NULL;

    p = (char*)malloc(10);
    p2 =(char*)malloc(42);

    p[0] = 1;
    p2[0] = 1;
    if (true) {
        celix_autofree uint8_t *buf = (uint8_t*)malloc(128);
        celix_autofree char* alwaysNullAgain = NULL;
        buf[0] = 1;
        ASSERT_EQ(nullptr, alwaysNullAgain);
    }

    if (true) {
        celix_autofree uint8_t* buf2 = (uint8_t*)malloc(256);
        buf2[255] = 42;
    }
    ASSERT_EQ(nullptr, alwaysNull);
}

static void* mutexLockedThread(void* data)
{
    celix_thread_mutex_t* mutex = (celix_thread_mutex_t*) data;
    EXPECT_EQ(EBUSY, celixThreadMutex_tryLock(mutex));
    return NULL;
}

static void* mutexUnlockedThread(void* data)
{
    celix_thread_mutex_t* mutex = (celix_thread_mutex_t*) data;
    EXPECT_EQ(0, celixThreadMutex_tryLock(mutex));
    return NULL;
}

TEST_F(CelixUtilsCleanupTestSuite, MutexLockerTest) {
    celix_thread_mutex_t mutex = CELIX_THREAD_MUTEX_INITIALIZER;
    celix_thread_t thread = celix_thread_default;
    if (true) {
        celix_autoptr(celix_mutex_locker_t) val = celixThreadMutexLocker_new(&mutex);
        ASSERT_NE(nullptr, val);

        // Verify that the mutex is actually locked
        celixThread_create(&thread, NULL, mutexLockedThread, &mutex);
        celixThread_join(thread, nullptr);
    }

    // Verify that the mutex is unlocked again
    celixThread_create(&thread, NULL, mutexUnlockedThread, &mutex);
    celixThread_join(thread, nullptr);
}
