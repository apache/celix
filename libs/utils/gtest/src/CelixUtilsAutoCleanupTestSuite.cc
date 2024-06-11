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

#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_threads.h"
#include "celix_unistd_cleanup.h"
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

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

TEST_F(CelixUtilsCleanupTestSuite, StealPtrTest) {
    celix_autofree char* p = NULL;
    p = (char*)malloc(10);
    free(celix_steal_ptr(p));
}

TEST_F(CelixUtilsCleanupTestSuite, ThradAttrTest) {
    celix_auto(celix_thread_attr_t) attr;
    EXPECT_EQ(0, pthread_attr_init(&attr));
}

TEST_F(CelixUtilsCleanupTestSuite, MutexTest) {
    celix_thread_mutex_t mutex;
    EXPECT_EQ(0, celixThreadMutex_create(&mutex, nullptr));
    celix_autoptr(celix_thread_mutex_t) mutex2 = &mutex;
}

static void* mutexLockedThread(void* data) {
    celix_thread_mutex_t* mutex = (celix_thread_mutex_t*) data;
    EXPECT_NE(0, celixThreadMutex_tryLock(mutex));
    return NULL;
}

static void* mutexUnlockedThread(void* data) {
    celix_thread_mutex_t* mutex = (celix_thread_mutex_t*) data;
    EXPECT_EQ(0, celixThreadMutex_tryLock(mutex));
    return NULL;
}

TEST_F(CelixUtilsCleanupTestSuite, MutexLockerTest) {
    celix_thread_mutex_t mutex;
    celix_thread_t thread = celix_thread_default;
    EXPECT_EQ(0, celixThreadMutex_create(&mutex, nullptr));
    if (true) {
        celix_auto(celix_mutex_lock_guard_t) val = celixMutexLockGuard_init(&mutex);
        ASSERT_NE(nullptr, val.mutex);

        // Verify that the mutex is actually locked
        celixThread_create(&thread, nullptr, mutexLockedThread, &mutex);
        celixThread_join(thread, nullptr);
    }

    // Verify that the mutex is unlocked again
    celixThread_create(&thread, nullptr, mutexUnlockedThread, &mutex);
    celixThread_join(thread, nullptr);
    celixThreadMutex_destroy(&mutex);
}

TEST_F(CelixUtilsCleanupTestSuite, MutexAttrTest) {
    celix_auto(celix_thread_mutexattr_t) attr;
    EXPECT_EQ(0, celixThreadMutexAttr_create(&attr));
}

/* Thread function to check that an rw lock given in @data cannot be writer locked */
static void* rwlockCannotTakeWriterLockThread(void* data) {
    celix_thread_rwlock_t* lock = (celix_thread_rwlock_t*)data;
    EXPECT_NE(0, celixThreadRwlock_tryWriteLock(lock));
    return NULL;
}

/* Thread function to check that an rw lock given in @data can be reader locked */
static void* rwlockCanTakeReaderLockThread(void* data) {
    celix_thread_rwlock_t* lock = (celix_thread_rwlock_t*)data;
    EXPECT_EQ(0, celixThreadRwlock_tryReadLock(lock));
    celixThreadRwlock_unlock(lock);
    return NULL;
}

TEST_F(CelixUtilsCleanupTestSuite, RwLockLockerTest) {
    celix_thread_rwlock_t lock;
    celix_thread_t thread = celix_thread_default;

    ASSERT_EQ(0, celixThreadRwlock_create(&lock, nullptr));
    if (true) {
        celix_auto(celix_rwlock_wlock_guard_t) val = celixRwlockWlockGuard_init(&lock);
        ASSERT_NE(nullptr, val.lock);

        /* Verify that we cannot take another writer lock as a writer lock is currently held */
        celixThread_create(&thread, nullptr, rwlockCannotTakeWriterLockThread, &lock);
        celixThread_join(thread, nullptr);

        /* Verify that we cannot take a reader lock as a writer lock is currently held */
        EXPECT_NE(0, celixThreadRwlock_tryReadLock(&lock));
    }
    if (true) {
        celix_auto(celix_rwlock_rlock_guard_t) val = celixRwlockRlockGuard_init(&lock);
        ASSERT_NE(nullptr, val.lock);

        /* Verify that we can take another reader lock from another thread */
        celixThread_create(&thread, nullptr, rwlockCanTakeReaderLockThread, &lock);
        celixThread_join(thread, nullptr);

        /* ... and also that recursive reader locking from the same thread works */
        EXPECT_EQ(0, celixThreadRwlock_tryReadLock(&lock));
        celixThreadRwlock_unlock(&lock);

        /* Verify that we cannot take a writer lock as a reader lock is currently held */
        celixThread_create(&thread, nullptr, rwlockCannotTakeWriterLockThread, &lock);
        celixThread_join(thread, nullptr);
    }

    /* Verify that we can take a writer lock again: this can only work if all of
     * the locks taken above have been correctly released. */
    EXPECT_EQ(0, celixThreadRwlock_tryWriteLock(&lock));
    celixThreadRwlock_unlock(&lock);
    celixThreadRwlock_destroy(&lock);
}

TEST_F(CelixUtilsCleanupTestSuite, RwLockAttrTest) {
    celix_auto(celix_thread_rwlockattr_t) attr;
    EXPECT_EQ(0, celixThreadRwlockAttr_create(&attr));
}

TEST_F(CelixUtilsCleanupTestSuite, CondTest) {
    celix_thread_cond_t cond;
    EXPECT_EQ(0, celixThreadCondition_init(&cond, nullptr));
    celix_autoptr(celix_thread_cond_t) cond2 = &cond;
}

TEST_F(CelixUtilsCleanupTestSuite, CondAttrTest) {
    celix_auto(celix_thread_condattr_t) attr;
    EXPECT_EQ(0, pthread_condattr_init(&attr));
}

TEST_F(CelixUtilsCleanupTestSuite, FileDescriptorTest) {
    celix_auto(celix_fd_t) fd = open("/dev/null", O_RDONLY);
    EXPECT_NE(-1, fd);
}

TEST_F(CelixUtilsCleanupTestSuite, StealFdTest) {
    celix_auto(celix_fd_t) fd = open("/dev/null", O_RDONLY);
    EXPECT_NE(-1, fd);
    celix_fd_t fd2 = celix_steal_fd(&fd);
    EXPECT_EQ(-1, fd);
    EXPECT_NE(-1, fd2);
    close(fd2);
}

TEST_F(CelixUtilsCleanupTestSuite, FileTest) {
    celix_autoptr(FILE) file = tmpfile();
}

TEST_F(CelixUtilsCleanupTestSuite, DirTest) {
    celix_autoptr(DIR) dir = opendir(".");
}
