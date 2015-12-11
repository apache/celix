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
#include "thpool.h"
}

celix_thread_mutex_t mutex;
int sum=0;


void * increment(void *) {
	celixThreadMutex_lock(&mutex);
	sum ++;
	celixThreadMutex_unlock(&mutex);
	return NULL;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}


//----------------------TEST THREAD FUNCTION DECLARATIONS----------------------

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(thread_pool) {
	threadpool	myPool;

	void setup(void) {
	}

	void teardown(void) {
	}
};


//----------------------THREAD_POOL TESTS----------------------

TEST(thread_pool, create) {

	myPool = thpool_init(5);	// pool of 5 threads
	CHECK((myPool != NULL));
	thpool_destroy(myPool);
}

TEST(thread_pool, do_work) {

	myPool = thpool_init(5);	// pool of 5 threads
	celixThreadMutex_create(&mutex, NULL);
	CHECK((myPool != NULL));
	int n;
	sum = 0;
	int num_jobs = 1000;
	for (n = 0; n < num_jobs; n++){
		thpool_add_work(myPool, increment, NULL);
	}
	thpool_wait(myPool);
	thpool_destroy(myPool);
	CHECK_EQUAL(1000, sum);
	celixThreadMutex_destroy(&mutex);
}

TEST(thread_pool, do_work_with_pause) {

	myPool = thpool_init(5);	// pool of 5 threads
	celixThreadMutex_create(&mutex, NULL);
	CHECK((myPool != NULL));
	int n;
	sum = 0;
	int num_jobs = 500000;
	for (n = 0; n < num_jobs; n++){
		thpool_add_work(myPool, increment, NULL);
	}
	sleep(1);
	thpool_pause(myPool);
	for (n = 0; n < num_jobs; n++){
		thpool_add_work(myPool, increment, NULL);
	}
	thpool_resume(myPool);
	thpool_wait(myPool);
	thpool_destroy(myPool);
	CHECK_EQUAL(1000000, sum);
	celixThreadMutex_destroy(&mutex);
}
