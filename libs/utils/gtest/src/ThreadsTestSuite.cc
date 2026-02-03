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

#include <gtest/gtest.h>

#include <csignal>

#include "celix_compiler.h"
#include "celix_utils.h"
#include "celix_threads.h"

class ThreadsTestSuite : public ::testing::Test {
public:
#ifdef __APPLE__
    const double ALLOWED_ERROR_MARGIN = 0.2;
#else
    const double ALLOWED_ERROR_MARGIN= 0.1;
#endif
};

//----------------------TEST THREAD FUNCTION DECLARATIONS----------------------
static void * thread_test_func_create(void *);
static void * thread_test_func_exit(void *);
static void * thread_test_func_detach(void *);
static void * thread_test_func_self(void *);
static void * thread_test_func_lock(void *);
static void * thread_test_func_cond_wait(void *);
static void * thread_test_func_cond_broadcast(void *);
static int thread_test_func_recur_lock(celix_thread_mutex_t*, int);
static void * thread_test_func_kill(void*);

static void * thread_test_func_once(void*);
static void thread_test_func_once_init();
static int thread_test_func_once_init_times_called = 0;

struct func_param{
    int i, i2;
    celix_thread_mutex_t mu, mu2;
    celix_thread_cond_t cond, cond2;
    celix_thread_once_t once_control;
    celix_thread_rwlock_t rwlock;
};

//----------------------CELIX THREADS TESTS----------------------

TEST_F(ThreadsTestSuite, CreateTest) {
    celix_thread_t thread;
    int ret;
    char* testStr = nullptr;

    ret = celixThread_create(&thread, nullptr, &thread_test_func_create,
                             &testStr);
    EXPECT_EQ(CELIX_SUCCESS, ret);
    celixThread_join(thread, nullptr);

    EXPECT_STREQ("SUCCESS", testStr);

    free(testStr);
}

TEST_F(ThreadsTestSuite, ExitTest) {
    int ret, *status;
    celix_thread_t thread;

    ret = celixThread_create(&thread, nullptr, &thread_test_func_exit, nullptr);
    EXPECT_EQ(CELIX_SUCCESS, ret);
    celixThread_join(thread, (void**) &status);
    EXPECT_EQ(666, *status);
    free(status);
}

TEST_F(ThreadsTestSuite, DetachTest) {
    int ret;
    celix_thread_t thread;

    celixThread_create(&thread, nullptr, thread_test_func_detach, nullptr);
    ret = celixThread_detach(thread);
    EXPECT_EQ(CELIX_SUCCESS, ret);
}

TEST_F(ThreadsTestSuite, SelfTest) {
    celix_thread_t thread;
    celix_thread_t thread2;

    celixThread_create(&thread, nullptr, thread_test_func_self, &thread2);
    celixThread_join(thread, nullptr);
    EXPECT_TRUE(celixThread_equals(thread, thread2));
}

TEST_F(ThreadsTestSuite, InitializedTest) {
    celix_thread_t thread{};
    EXPECT_FALSE(celixThread_initialized(thread));

    celixThread_create(&thread, nullptr, thread_test_func_detach, nullptr);
    EXPECT_TRUE(celixThread_initialized(thread));
    celixThread_join(thread, nullptr);
}

TEST_F(ThreadsTestSuite, OnceTest) {
    int *status;
    celix_thread_t thread;
    celix_thread_t thread2;
    struct func_param* params = (struct func_param*) calloc(1,sizeof(struct func_param));

    celixThread_create(&thread, nullptr, &thread_test_func_once, params);
    celixThread_join(thread, (void**) &status);

    celixThread_create(&thread2, nullptr, &thread_test_func_once, params);
    celixThread_join(thread2, (void**) &status);

    free(params);

    EXPECT_EQ(1, thread_test_func_once_init_times_called);
}

//----------------------CELIX THREADS KILL TESTS----------------------

TEST_F(ThreadsTestSuite, KillTest){
    //setup signal handler to ignore SIGUSR1
    struct sigaction sigact;
    struct sigaction sigactold;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = [](int) {/*nop*/};;
    sigaction(SIGUSR1, &sigact, &sigactold);


    int * ret;
    celix_thread_t thread;
    celixThread_create(&thread, nullptr, thread_test_func_kill, nullptr);
    sleep(1);

    celixThread_kill(thread, SIGUSR1);
    celixThread_join(thread, (void**)&ret);

    EXPECT_EQ(-1, *ret); // -1 returned from usleep (interrupted by signal)
    free(ret);

    sigaction(SIGUSR1, &sigactold, nullptr);
}

//----------------------CELIX THREADS MUTEX TESTS----------------------

TEST_F(ThreadsTestSuite, CreateMutexTest) {
    celix_thread_mutex_t mu;
    EXPECT_EQ(CELIX_SUCCESS, celixThreadMutex_create(&mu, nullptr));
    EXPECT_EQ(CELIX_SUCCESS, celixThreadMutex_destroy(&mu));
}


TEST_F(ThreadsTestSuite, LockTest) {
    celix_thread_t thread;
    struct func_param * params = (struct func_param*) calloc(1,
                                                             sizeof(struct func_param));

    celixThreadMutex_create(&params->mu, nullptr);

    celixThreadMutex_lock(&params->mu);
    celixThread_create(&thread, nullptr, thread_test_func_lock, params);

    sleep(1);

    EXPECT_EQ(0, params->i);

    //possible race condition, not perfect test
    celixThreadMutex_unlock(&params->mu);

    sleep(1);

    celixThreadMutex_lock(&params->mu2);
    EXPECT_EQ(666, params->i);
    celixThreadMutex_unlock(&params->mu2);
    celixThread_join(thread, nullptr);
    free(params);
}

TEST_F(ThreadsTestSuite, AttrCreateTest) {
    celix_thread_mutexattr_t mu_attr;
    EXPECT_EQ(CELIX_SUCCESS, celixThreadMutexAttr_create(&mu_attr));
    EXPECT_EQ(CELIX_SUCCESS, celixThreadMutexAttr_destroy(&mu_attr));
}

TEST_F(ThreadsTestSuite, AttrSettypeTest) {
    celix_thread_mutex_t mu;
    celix_thread_mutexattr_t mu_attr;
    celixThreadMutexAttr_create(&mu_attr);

    //test recursive mutex
    celixThreadMutexAttr_settype(&mu_attr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&mu, &mu_attr);
    //if program doesnt deadlock: success! also check factorial of 10, for reasons unknown
    EXPECT_EQ(3628800, thread_test_func_recur_lock(&mu, 10));
    celixThreadMutex_destroy(&mu);

    //test deadlock check mutex
    celixThreadMutexAttr_settype(&mu_attr, CELIX_THREAD_MUTEX_ERRORCHECK);
    celixThreadMutex_create(&mu, &mu_attr);
    //get deadlock error
    celixThreadMutex_lock(&mu);
    EXPECT_TRUE(celixThreadMutex_lock(&mu) != CELIX_SUCCESS);
    //do not get deadlock error
    celixThreadMutex_unlock(&mu);
    EXPECT_EQ(CELIX_SUCCESS, celixThreadMutex_lock(&mu));
    //get unlock error
    celixThreadMutex_unlock(&mu);
    EXPECT_TRUE(celixThreadMutex_unlock(&mu) != CELIX_SUCCESS);

    celixThreadMutex_destroy(&mu);
    celixThreadMutexAttr_destroy(&mu_attr);
}

//----------------------CELIX THREAD CONDITIONS TESTS----------------------

TEST_F(ThreadsTestSuite, InitCondTest) {
    celix_thread_cond_t cond;
    EXPECT_EQ(CELIX_SUCCESS, celixThreadCondition_init(&cond, nullptr));
    EXPECT_EQ(CELIX_SUCCESS, celixThreadCondition_destroy(&cond));
}

//test wait and signal
TEST_F(ThreadsTestSuite, WaitCondTest) {
    celix_thread_t thread;
    struct func_param * param = (struct func_param*) calloc(1,
                                                            sizeof(struct func_param));
    celixThreadMutex_create(&param->mu, nullptr);
    celixThreadCondition_init(&param->cond, nullptr);

    celixThreadMutex_lock(&param->mu);

    sleep(2);

    celixThread_create(&thread, nullptr, thread_test_func_cond_wait, param);
    EXPECT_EQ(0, param->i);

    celixThreadCondition_wait(&param->cond, &param->mu);
    EXPECT_EQ(666, param->i);
    celixThreadMutex_unlock(&param->mu);

    celixThread_join(thread, nullptr);
    free(param);
}

//test wait and broadcast on multiple threads
TEST_F(ThreadsTestSuite, CondBroadcastTest) {
    celix_thread_t thread;
    celix_thread_t thread2;
    struct func_param * param = (struct func_param*) calloc(1,sizeof(struct func_param));
    celixThreadMutex_create(&param->mu, nullptr);
    celixThreadMutex_create(&param->mu2, nullptr);
    celixThreadCondition_init(&param->cond, nullptr);

    celixThread_create(&thread, nullptr, thread_test_func_cond_broadcast, param);
    celixThread_create(&thread2, nullptr, thread_test_func_cond_broadcast, param);

    sleep(1);
    celixThreadMutex_lock(&param->mu);
    EXPECT_EQ(0, param->i);
    celixThreadMutex_unlock(&param->mu);

    celixThreadMutex_lock(&param->mu);
    celixThreadCondition_broadcast(&param->cond);
    celixThreadMutex_unlock(&param->mu);
    sleep(1);
    celixThreadMutex_lock(&param->mu);
    EXPECT_EQ(2, param->i);
    celixThreadMutex_unlock(&param->mu);

    celixThread_join(thread, nullptr);
    celixThread_join(thread2, nullptr);
    free(param);
}

TEST_F(ThreadsTestSuite, CondTimedWaitTest) {
    celix_thread_mutex_t mutex;
    celix_thread_cond_t cond;

    auto status = celixThreadMutex_create(&mutex, nullptr);
    ASSERT_EQ(status, CELIX_SUCCESS);
    status = celixThreadCondition_init(&cond, nullptr);
    ASSERT_EQ(status, CELIX_SUCCESS);

    //Test with nullptr abstime
    status = celixThreadCondition_waitUntil(&cond, &mutex, nullptr);
    ASSERT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //Test with valid abstime
    auto start = celixThreadCondition_getTime();
    auto targetEnd = celixThreadCondition_getDelayedTime(0.01);
    celixThreadMutex_lock(&mutex);
    status = celixThreadCondition_waitUntil(&cond, &mutex, &targetEnd);
    ASSERT_EQ(status, ETIMEDOUT);
    celixThreadMutex_unlock(&mutex);
    auto end = celixThreadCondition_getTime();
    EXPECT_NEAR(celix_difftime(&start, &end), 0.01, ALLOWED_ERROR_MARGIN);

    start = celixThreadCondition_getTime();
    celixThreadMutex_lock(&mutex);
    status = celixThreadCondition_waitUntil(&cond, &mutex, &start);
    ASSERT_EQ(status, ETIMEDOUT);
    celixThreadMutex_unlock(&mutex);
    end = celixThreadCondition_getTime();
    EXPECT_NEAR(celix_difftime(&start, &end), 0.0, ALLOWED_ERROR_MARGIN);
}

//----------------------CELIX READ-WRITE LOCK TESTS----------------------

TEST_F(ThreadsTestSuite, CreateRwLockTest) {
    celix_thread_rwlock_t alock;
    celix_status_t status;
    status = celixThreadRwlock_create(&alock, nullptr);
    if (status != CELIX_SUCCESS) {
        fprintf(stderr, "Found error '%s'\n", strerror(status));
    }
    EXPECT_EQ(CELIX_SUCCESS, status);
    status = celixThreadRwlock_destroy(&alock);
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(ThreadsTestSuite, ReadLockTest) {
    int status;
    celix_thread_rwlock_t lock;
    memset(&lock, 0x00, sizeof(celix_thread_rwlock_t));
    //struct func_param * param = (struct func_param*) calloc(1,sizeof(struct func_param));

    celixThreadRwlock_create(&lock, nullptr);

    status = celixThreadRwlock_readLock(&lock);
    EXPECT_EQ(0, status);
    status = celixThreadRwlock_readLock(&lock);
    EXPECT_EQ(0, status);
    status = celixThreadRwlock_unlock(&lock);
    EXPECT_EQ(0, status);
    status = celixThreadRwlock_unlock(&lock);
    EXPECT_EQ(0, status);

    celixThreadRwlock_destroy(&lock);
}

TEST_F(ThreadsTestSuite, WriteLockTest) {
    int status;
    celix_thread_rwlock_t lock;
    celixThreadRwlock_create(&lock, nullptr);

    status = celixThreadRwlock_writeLock(&lock);
    EXPECT_EQ(0, status);
    status = celixThreadRwlock_writeLock(&lock);
    //EDEADLK ErNo: Resource deadlock avoided
    EXPECT_EQ(EDEADLK, status);

    celixThreadRwlock_unlock(&lock);
}

TEST_F(ThreadsTestSuite, RwLockAttrTest) {
    celix_thread_rwlockattr_t attr;
    celixThreadRwlockAttr_create(&attr);
    celixThreadRwlockAttr_destroy(&attr);
}

static void * thread_test_func_create(void * arg) {
    char ** test_str = (char**) arg;
    *test_str = strdup("SUCCESS");

    return nullptr;
}

static void * thread_test_func_exit(void *) {
    int *pi = (int*) calloc(1, sizeof(int));
    *pi = 666;

    celixThread_exit(pi);
    return nullptr;
}

static void * thread_test_func_detach(void *) {
    return nullptr;
}

static void * thread_test_func_self(void * arg) {
    *((celix_thread*) arg) = celixThread_self();
    return nullptr;
}

static void * thread_test_func_once(void * arg) {
    struct func_param *param = (struct func_param *) arg;
    celixThread_once(&param->once_control, thread_test_func_once_init);
    return nullptr;
}

static void thread_test_func_once_init() {
    thread_test_func_once_init_times_called += 1;
}

static void * thread_test_func_lock(void *arg) {
    struct func_param *param = (struct func_param *) arg;

    celixThreadMutex_lock(&param->mu2);
    celixThreadMutex_lock(&param->mu);
    param->i = 666;
    celixThreadMutex_unlock(&param->mu);
    celixThreadMutex_unlock(&param->mu2);

    return nullptr;
}

static void * thread_test_func_cond_wait(void *arg) {
    struct func_param *param = (struct func_param *) arg;

    celixThreadMutex_lock(&param->mu);

    param->i = 666;

    celixThreadCondition_signal(&param->cond);
    celixThreadMutex_unlock(&param->mu);
    return nullptr;
}

static void * thread_test_func_cond_broadcast(void *arg) {
    struct func_param *param = (struct func_param *) arg;

    celixThreadMutex_lock(&param->mu);
    celixThreadCondition_wait(&param->cond, &param->mu);
    celixThreadMutex_unlock(&param->mu);
    celixThreadMutex_lock(&param->mu);
    param->i++;
    celixThreadMutex_unlock(&param->mu);
    return nullptr;
}

static int thread_test_func_recur_lock(celix_thread_mutex_t *mu, int i) {
    int temp;
    if (i == 1) {
        return 1;
    } else {
        celixThreadMutex_lock(mu);
        temp = thread_test_func_recur_lock(mu, i - 1);
        temp *= i;
        celixThreadMutex_unlock(mu);
        return temp;
    }
}

static void * thread_test_func_kill(void* arg CELIX_UNUSED){
    int * ret = (int*) malloc(sizeof(*ret));
    //sleep for about a minute, or until a kill signal (USR1) is received
    *ret = usleep(60000000);
    return ret;
}
