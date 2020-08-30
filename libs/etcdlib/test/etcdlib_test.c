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
 * Test program for testing the etcdlib.
 * Prerequisite is that etcdlib is started on localhost on default port (2379)
 * tested with etcd 2.3.7
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "etcd.h"

#include <pthread.h>

static etcdlib_t *etcdlib;

int simplewritetest() {
	int res = 0;
	char*value = NULL;
	etcdlib_set(etcdlib, "simplekey", "testvalue", 5, false);
	etcdlib_get(etcdlib, "simplekey", &value, NULL);
	if (value && strcmp(value, "testvalue")) {
		printf("etcdlib test error: expected testvalue got %s\n", value);
		res = -1;
	}
	free(value);
	return res;
}

void* waitForChange(void*arg) {
	int *idx = (int*)arg;
	char *action = NULL;
	char *prevValue = NULL;
	char *value = NULL;
	char *rkey = NULL;
	long long modifiedIndex;

	printf("Watching for index %d\n", *idx);

	if(etcdlib_watch(etcdlib, "hier/ar", *idx, &action, &prevValue, &value, &rkey, &modifiedIndex) == 0){
		printf(" New value from watch : [%s]%s => %s\n", rkey, prevValue, value);
		if(action != NULL) free(action);
		if(prevValue != NULL) free(prevValue);
		if(rkey != NULL) free(rkey);
		if(value != NULL) free(value);
	}

	*idx = modifiedIndex+1;

	action = NULL;
	prevValue = NULL;
	value = NULL;
	rkey = NULL;

	if(etcdlib_watch(etcdlib, "hier/ar", *idx, &action, &prevValue, &value, &rkey, &modifiedIndex) == 0){
		printf(" New value from watch : [%s]%s => %s\n", rkey, prevValue, value);
		if(action != NULL) free(action);
		if(prevValue != NULL) free(prevValue);
		if(rkey != NULL) free(rkey);
	}

	return value;
}

int waitforchangetest() {
	int res = 0;
	char*value = NULL;

	etcdlib_set(etcdlib, "hier/ar/chi/cal", "testvalue1", 5, false);

	int index;
	etcdlib_get(etcdlib, "hier/ar/chi/cal", &value, &index);
	free(value);
	pthread_t waitThread;
	index++;
	pthread_create(&waitThread, NULL, waitForChange, &index);
	sleep(1);
	etcdlib_set(etcdlib, "hier/ar/chi/cal", "testvalue2", 5, false);
	sleep(1);
	etcdlib_set(etcdlib, "hier/ar/chi/cal", "testvalue3", 5, false);
	void *resVal = NULL;
	printf("joining\n");
	pthread_join(waitThread, &resVal);
	if(resVal == NULL || strcmp((char*)resVal,"testvalue3" ) != 0) {
		printf("etcdtest::waitforchange1 expected 'testvalue3', got '%s'\n", (char*)resVal);
		res = -1;
	}
	free(resVal);
	return res;
}

int main (void) {
	etcdlib = etcdlib_create("localhost", 2379, 0);

//	long long index = 0;
//	char* action;
//	char* prevValue;
//	char* value;
//	char* rkey;
//
//	while (1) {
//		long long modIndex;
//		int rc = etcd_watch("entries", index, &action, &prevValue, &value, &rkey, &modIndex);
//		if (rc == ETCDLIB_RC_OK) {
//			printf("Got result on index %lli, action:'%s', prevValue:'%s', value:'%s', rkey:'%s', modIndex %lli\n",
//				   index, action, prevValue, value, rkey, modIndex);
//			index = modIndex + 1;
//		} else if (rc == ETCDLIB_RC_TIMEOUT) {
//			printf("timeout\n");
//		} else {
//			printf("Got error %i, action:'%s', prevValue:'%s', value:'%s', rkey:'%s', modIndex %lli\n", rc, action, prevValue, value, rkey, modIndex);
//		}
//		free(action);
//		free(prevValue);
//		free(value);
//		free(rkey);
//		action = NULL;
//		prevValue = NULL;
//		value = NULL;
//		rkey = NULL;
//	}


	int res = simplewritetest(); if(res) return res; else printf("simplewrite test success\n");
	res = waitforchangetest(); if(res) return res;else printf("waitforchange1 test success\n");

	etcdlib_destroy(etcdlib);

	return 0;
}

