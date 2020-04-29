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
/**
 * array_list_test.cpp
 *
 *  \date       Sep 15, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "celix_threads.h"
#include "CppUTestExt/MockSupport_c.h"


static char* my_strdup(const char* s) {
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s);

    char *d = (char*) calloc(len + 1, sizeof(char));

    if (d == NULL) {
        return NULL;
    }

    strncpy(d, s, len);
    return d;
}

static int celix_thread_t_equals(const void * object, const void * compareTo){
    celix_thread_t * thread1 = (celix_thread_t*) object;
    celix_thread_t * thread2 = (celix_thread_t*) compareTo;

    return thread1->thread == thread2->thread &&
            thread1->threadInitialized == thread2->threadInitialized;
}

static const char * celix_thread_t_toString(const void * object){
    celix_thread_t * thread = (celix_thread_t*) object;
    char buff[512];
    snprintf(buff, 512, "thread: %lu, threadInitialized: %s", (unsigned long)thread->thread, (thread->threadInitialized ? "true" : "false"));

    return my_strdup(buff);
}

//----------------------TEST THREAD FUNCTION DECLARATIONS----------------------
static void * thread_test_func_create(void *);
static void * thread_test_func_exit(void *);
static void * thread_test_func_detach(void *);
static void * thread_test_func_self(void *);
static void * thread_test_func_once(void*);
static void thread_test_func_once_init(void);
static void * thread_test_func_lock(void *);
static void * thread_test_func_cond_wait(void *);
static void * thread_test_func_cond_broadcast(void *);
static int thread_test_func_recur_lock(celix_thread_mutex_t*, int);
static void * thread_test_func_kill(void*);
static void thread_test_func_kill_handler(int);
struct func_param{
    int i, i2;
    celix_thread_mutex_t mu, mu2;
    celix_thread_cond_t cond, cond2;
    celix_thread_once_t once_control;
    celix_thread_rwlock_t rwlock;
};
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(celix_thread) {
    celix_thread thread;

    void setup(void) {
    }

    void teardown(void) {
    }
};

TEST_GROUP(celix_thread_kill) {
    celix_thread thread;
    struct sigaction sigact, sigactold;

    void setup(void) {
        memset(&sigact, 0, sizeof(sigact));
        sigact.sa_handler = thread_test_func_kill_handler;
        sigaction(SIGUSR1, &sigact, &sigactold);

        mock_c()->installComparator("celix_thread_t", celix_thread_t_equals, celix_thread_t_toString);
    }

    void teardown(void) {
        sigaction(SIGUSR1, &sigactold, &sigact);

        mock_c()->removeAllComparatorsAndCopiers();
    }
};

TEST_GROUP(celix_thread_mutex) {
    celix_thread thread;
    celix_thread_mutex_t mu;

    void setup(void) {
    }

    void teardown(void) {
    }
};

TEST_GROUP(celix_thread_condition) {
    celix_thread thread;
    celix_thread_mutex_t mu;
    celix_thread_cond_t cond;

    void setup(void) {
    }

    void teardown(void) {
    }
};

TEST_GROUP(celix_thread_rwlock) {
    celix_thread thread;

    void setup(void) {
    }

    void teardown(void) {
    }
};

//----------------------CELIX THREADS TESTS----------------------

TEST(celix_thread, create) {
    int ret;
    char * test_str;

    ret = celixThread_create(&thread, NULL, &thread_test_func_create,
            &test_str);
    LONGS_EQUAL(CELIX_SUCCESS, ret);
    celixThread_join(thread, NULL);

    CHECK(test_str != NULL);
    STRCMP_EQUAL("SUCCESS", test_str);

    free(test_str);
}

TEST(celix_thread, exit) {
    int ret, *status;

    ret = celixThread_create(&thread, NULL, &thread_test_func_exit, NULL);
    LONGS_EQUAL(CELIX_SUCCESS, ret);
    celixThread_join(thread, (void**) &status);
    LONGS_EQUAL(666, *status);
    free(status);
}

//HORRIBLE TEST
TEST(celix_thread, detach) {
    int ret;

    celixThread_create(&thread, NULL, thread_test_func_detach, NULL);
    ret = celixThread_detach(thread);
    LONGS_EQUAL(CELIX_SUCCESS, ret);
}

TEST(celix_thread, self) {
    celix_thread thread2;

    celixThread_create(&thread, NULL, thread_test_func_self, &thread2);
    celixThread_join(thread, NULL);
    CHECK(celixThread_equals(thread, thread2));
}

TEST(celix_thread, initialized) {
    CHECK(!celixThread_initialized(thread));
    celixThread_create(&thread, NULL, thread_test_func_detach, NULL);
    CHECK(celixThread_initialized(thread));
    celixThread_detach(thread);
}

TEST(celix_thread, once) {
    int *status;
    celix_thread thread2;
    struct func_param * params = (struct func_param*) calloc(1,
                sizeof(struct func_param));

    mock().expectOneCall("thread_test_func_once_init");

    celixThread_create(&thread, NULL, &thread_test_func_once, params);
    celixThread_join(thread, (void**) &status);

    celixThread_create(&thread2, NULL, &thread_test_func_once, params);
    celixThread_join(thread2, (void**) &status);

    free(params);

    mock().checkExpectations();
    mock().clear();
}
//----------------------CELIX THREADS KILL TESTS----------------------
TEST(celix_thread_kill, kill){
    int * ret;
    celixThread_create(&thread, NULL, thread_test_func_kill, NULL);
    sleep(2);

    mock().expectOneCall("thread_test_func_kill_handler")
            .withParameter("signo", SIGUSR1)
            .withParameterOfType("celix_thread_t", "inThread", (const void*) &thread);

    celixThread_kill(thread, SIGUSR1);
    celixThread_join(thread, (void**)&ret);

    LONGS_EQUAL(-1, *ret);
    free(ret);

    mock().checkExpectations();
    mock().clear();
}

//----------------------CELIX THREADS MUTEX TESTS----------------------

TEST(celix_thread_mutex, create) {
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadMutex_create(&mu, NULL));
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadMutex_destroy(&mu));
}

//check normal lock behaviour
TEST(celix_thread_mutex, lock) {
    struct func_param * params = (struct func_param*) calloc(1,
            sizeof(struct func_param));

    celixThreadMutex_create(&params->mu, NULL);

    celixThreadMutex_lock(&params->mu);
    celixThread_create(&thread, NULL, thread_test_func_lock, params);

    sleep(2);

    LONGS_EQUAL(0, params->i);

    //possible race condition, not perfect test
    celixThreadMutex_unlock(&params->mu);

    sleep(2);

    celixThreadMutex_lock(&params->mu2);
    LONGS_EQUAL(666, params->i);
    celixThreadMutex_unlock(&params->mu2);
    celixThread_join(thread, NULL);
    free(params);
}

TEST(celix_thread_mutex, attrCreate) {
    celix_thread_mutexattr_t mu_attr;
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadMutexAttr_create(&mu_attr));
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadMutexAttr_destroy(&mu_attr));
}

TEST(celix_thread_mutex, attrSettype) {
    celix_thread_mutex_t mu;
    celix_thread_mutexattr_t mu_attr;
    celixThreadMutexAttr_create(&mu_attr);

    //test recursive mutex
    celixThreadMutexAttr_settype(&mu_attr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&mu, &mu_attr);
    //if program doesnt deadlock: success! also check factorial of 10, for reasons unknown
    LONGS_EQUAL(3628800, thread_test_func_recur_lock(&mu, 10));
    celixThreadMutex_destroy(&mu);

    //test deadlock check mutex
    celixThreadMutexAttr_settype(&mu_attr, CELIX_THREAD_MUTEX_ERRORCHECK);
    celixThreadMutex_create(&mu, &mu_attr);
    //get deadlock error
    celixThreadMutex_lock(&mu);
    CHECK(celixThreadMutex_lock(&mu) != CELIX_SUCCESS);
    //do not get deadlock error
    celixThreadMutex_unlock(&mu);
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadMutex_lock(&mu));
    //get unlock error
    celixThreadMutex_unlock(&mu);
    CHECK(celixThreadMutex_unlock(&mu) != CELIX_SUCCESS);

    celixThreadMutex_destroy(&mu);
    celixThreadMutexAttr_destroy(&mu_attr);
}

//----------------------CELIX THREAD CONDITIONS TESTS----------------------

TEST(celix_thread_condition, init) {
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadCondition_init(&cond, NULL));
    LONGS_EQUAL(CELIX_SUCCESS, celixThreadCondition_destroy(&cond));
}

//test wait and signal
TEST(celix_thread_condition, wait) {
    struct func_param * param = (struct func_param*) calloc(1,
            sizeof(struct func_param));
    celixThreadMutex_create(&param->mu, NULL);
    celixThreadCondition_init(&param->cond, NULL);

    celixThreadMutex_lock(&param->mu);

    sleep(2);

    celixThread_create(&thread, NULL, thread_test_func_cond_wait, param);
    LONGS_EQUAL(0, param->i);

    celixThreadCondition_wait(&param->cond, &param->mu);
    LONGS_EQUAL(666, param->i);
    celixThreadMutex_unlock(&param->mu);

    celixThread_join(thread, NULL);
    free(param);
}

//test wait and broadcast on multiple threads
TEST(celix_thread_condition, broadcast) {
    celix_thread_t thread2;
    struct func_param * param = (struct func_param*) calloc(1,sizeof(struct func_param));
    celixThreadMutex_create(&param->mu, NULL);
    celixThreadMutex_create(&param->mu2, NULL);
    celixThreadCondition_init(&param->cond, NULL);

    celixThread_create(&thread, NULL, thread_test_func_cond_broadcast, param);
    celixThread_create(&thread2, NULL, thread_test_func_cond_broadcast, param);

    sleep(1);
    celixThreadMutex_lock(&param->mu);
    LONGS_EQUAL(0, param->i);
    celixThreadMutex_unlock(&param->mu);

    celixThreadMutex_lock(&param->mu);
    celixThreadCondition_broadcast(&param->cond);
    celixThreadMutex_unlock(&param->mu);
    sleep(1);
    celixThreadMutex_lock(&param->mu);
    LONGS_EQUAL(2, param->i);
    celixThreadMutex_unlock(&param->mu);

    celixThread_join(thread, NULL);
    celixThread_join(thread2, NULL);
    free(param);
}

//----------------------CELIX READ-WRITE LOCK TESTS----------------------

TEST(celix_thread_rwlock, create){
    celix_thread_rwlock_t lock;
    celix_status_t status;
    status = celixThreadRwlock_create(&lock, NULL);
    LONGS_EQUAL(CELIX_SUCCESS, status);
    status = celixThreadRwlock_destroy(&lock);
    LONGS_EQUAL(CELIX_SUCCESS, status);
}

TEST(celix_thread_rwlock, readLock){
    int status;
    celix_thread_rwlock_t lock;
    memset(&lock, 0x00, sizeof(celix_thread_rwlock_t));
    //struct func_param * param = (struct func_param*) calloc(1,sizeof(struct func_param));

    celixThreadRwlock_create(&lock, NULL);

    status = celixThreadRwlock_readLock(&lock);
    LONGS_EQUAL(0, status);
    status = celixThreadRwlock_readLock(&lock);
    LONGS_EQUAL(0, status);
    status = celixThreadRwlock_unlock(&lock);
    LONGS_EQUAL(0, status);
    status = celixThreadRwlock_unlock(&lock);
    LONGS_EQUAL(0, status);

    celixThreadRwlock_destroy(&lock);
}

TEST(celix_thread_rwlock, writeLock){
    int status;
    celix_thread_rwlock_t lock;
    celixThreadRwlock_create(&lock, NULL);

    status = celixThreadRwlock_writeLock(&lock);
    LONGS_EQUAL(0, status);
    status = celixThreadRwlock_writeLock(&lock);
    //EDEADLK ErNo: Resource deadlock avoided
    LONGS_EQUAL(EDEADLK, status);

    celixThreadRwlock_unlock(&lock);
}

TEST(celix_thread_rwlock, attr){
    celix_thread_rwlockattr_t attr;
    celixThreadRwlockAttr_create(&attr);
    celixThreadRwlockAttr_destroy(&attr);
}

//----------------------TEST THREAD FUNCTION DEFINES----------------------
extern "C" {
static void * thread_test_func_create(void * arg) {
    char ** test_str = (char**) arg;
    *test_str = my_strdup("SUCCESS");

    return NULL;
}

static void * thread_test_func_exit(void *) {
    int *pi = (int*) calloc(1, sizeof(int));
    *pi = 666;

    celixThread_exit(pi);
    return NULL;
}

static void * thread_test_func_detach(void *) {
    return NULL;
}

static void * thread_test_func_self(void * arg) {
    *((celix_thread*) arg) = celixThread_self();
    return NULL;
}

static void * thread_test_func_once(void * arg) {
    struct func_param *param = (struct func_param *) arg;
    celixThread_once(&param->once_control, thread_test_func_once_init);

    return NULL;
}

static void thread_test_func_once_init(void) {
    mock_c()->actualCall("thread_test_func_once_init");
}

static void * thread_test_func_lock(void *arg) {
    struct func_param *param = (struct func_param *) arg;

    celixThreadMutex_lock(&param->mu2);
    celixThreadMutex_lock(&param->mu);
    param->i = 666;
    celixThreadMutex_unlock(&param->mu);
    celixThreadMutex_unlock(&param->mu2);

    return NULL;
}

static void * thread_test_func_cond_wait(void *arg) {
    struct func_param *param = (struct func_param *) arg;

    celixThreadMutex_lock(&param->mu);

    param->i = 666;

    celixThreadCondition_signal(&param->cond);
    celixThreadMutex_unlock(&param->mu);
    return NULL;
}

static void * thread_test_func_cond_broadcast(void *arg) {
    struct func_param *param = (struct func_param *) arg;

    celixThreadMutex_lock(&param->mu);
    celixThreadCondition_wait(&param->cond, &param->mu);
    celixThreadMutex_unlock(&param->mu);
    celixThreadMutex_lock(&param->mu);
    param->i++;
    celixThreadMutex_unlock(&param->mu);
    return NULL;
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

static void * thread_test_func_kill(void __attribute__((unused)) *arg){
    int * ret = (int*) malloc(sizeof(*ret));
    //sleep for a about a minute, or until a kill signal (USR1) is received
    *ret = usleep(60000000);
    return ret;
}

static void thread_test_func_kill_handler(int signo){
    celix_thread_t inThread = celixThread_self();

    mock_c()->actualCall("thread_test_func_kill_handler")
            ->withLongIntParameters("signo", signo)
            ->withParameterOfType("celix_thread_t", "inThread", (const void*) &inThread);
}
}
