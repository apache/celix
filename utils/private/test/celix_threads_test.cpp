/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * array_list_test.cpp
 *
 * 	\date       Sep 15, 2015
 *  \author    	Menno van der Graaf & Alexander
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
#include "celix_threads.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s) {
	char *d = (char*) malloc(strlen(s) + 1);
	if (d == NULL)
		return NULL;
	strcpy(d, s);
	return d;
}

//----------------------TEST THREAD FUNCTION DECLARATIONS----------------------

static void * thread_test_func_create(void *);
static void * thread_test_func_exit(void *);
static void * thread_test_func_detach(void *);
static void * thread_test_func_self(void *);
static void * thread_test_func_lock(void *);
static void * thread_test_func_cond_wait(void *arg);
static void * thread_test_func_cond_broadcast(void *arg);
static int thread_test_func_recur_lock(celix_thread_mutex_t*, int);
struct func_param{
	int i, i2;
	celix_thread_mutex_t mu, mu2;
	celix_thread_cond_t cond, cond2;
};
//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(celix_thread) {
	celix_thread thread;

	void setup(void) {
	}

	void teardown(void) {
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

//HORIBLE TEST
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

TEST(celix_thread, initalized) {
	CHECK(!celixThread_initalized(thread));
	celixThread_create(&thread, NULL, thread_test_func_detach, NULL);
	CHECK(celixThread_initalized(thread));
	celixThread_detach(thread);
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
	celixThreadMutexAttr_settype(&mu_attr, PTHREAD_MUTEX_RECURSIVE);
	celixThreadMutex_create(&mu, &mu_attr);
	//if program doesnt deadlock: succes! also check factorial of 10, for reasons unknown
	LONGS_EQUAL(3628800, thread_test_func_recur_lock(&mu, 10));
	celixThreadMutex_destroy(&mu);

	//test deadlock check mutex
	celixThreadMutexAttr_settype(&mu_attr, PTHREAD_MUTEX_ERRORCHECK);
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
	celixThreadMutex_lock(&param->mu2);
	LONGS_EQUAL(0, param->i);
	celixThreadMutex_unlock(&param->mu2);

	celixThreadMutex_lock(&param->mu);
	celixThreadCondition_broadcast(&param->cond);
	celixThreadMutex_unlock(&param->mu);
	sleep(1);
	celixThreadMutex_lock(&param->mu2);
	LONGS_EQUAL(2, param->i);
	celixThreadMutex_unlock(&param->mu2);

	celixThread_join(thread, NULL);
	celixThread_join(thread2, NULL);
	free(param);
}

//----------------------TEST THREAD FUNCTION DEFINES----------------------

static void * thread_test_func_create(void * arg) {
	char ** test_str = (char**) arg;
	*test_str = my_strdup("SUCCESS");
	celixThread_exit(NULL);
}

static void * thread_test_func_exit(void *) {
	int *pi = (int*) calloc(1, sizeof(int));
	*pi = 666;
	celixThread_exit(pi);
}

static void * thread_test_func_detach(void *) {
	celixThread_exit(NULL);
}

static void * thread_test_func_self(void * arg) {
	*((celix_thread*) arg) = celixThread_self();
	celixThread_exit(NULL);
}

static void * thread_test_func_lock(void *arg) {
	struct func_param *param = (struct func_param *) arg;

	celixThreadMutex_lock(&param->mu2);
	celixThreadMutex_lock(&param->mu);
	param->i = 666;
	celixThreadMutex_unlock(&param->mu);
	celixThreadMutex_unlock(&param->mu2);

	celixThread_exit(NULL);
}

static void * thread_test_func_cond_wait(void *arg) {
	struct func_param *param = (struct func_param *) arg;

	celixThreadMutex_lock(&param->mu);

	param->i = 666;

	celixThreadCondition_signal(&param->cond);
	celixThreadMutex_unlock(&param->mu);
	celixThread_exit(NULL);
}

static void * thread_test_func_cond_broadcast(void *arg) {
	struct func_param *param = (struct func_param *) arg;

	celixThreadMutex_lock(&param->mu);
	celixThreadCondition_wait(&param->cond, &param->mu);
	celixThreadMutex_unlock(&param->mu);
	celixThreadMutex_lock(&param->mu2);
	param->i++;
	celixThreadMutex_unlock(&param->mu2);
	celixThread_exit(NULL);
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
