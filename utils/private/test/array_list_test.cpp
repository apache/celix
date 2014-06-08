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
 *  Created on: Jun 6, 2012
 *      Author: alexander
 */
#include <stdio.h>
#include <stdlib.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "array_list.h"
#include "array_list_private.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(array_list) {
	array_list_pt list;

	void setup(void) {
		arrayList_create(&list);
	}
	void teardown() {
		arrayList_destroy(list);
	}
};


TEST(array_list, create) {
	CHECK_C(list != NULL);
    LONGS_EQUAL(0, list->size);
}

TEST(array_list, trimToSize) {
	bool added;
	std::string entry;
	arrayList_clear(list);

	entry = "entry";
	added = arrayList_add(list, (char *) entry.c_str());
	LONGS_EQUAL(list->size, 1);
	LONGS_EQUAL(list->capacity, 10);

	arrayList_trimToSize(list);
	LONGS_EQUAL(list->size, 1);
	LONGS_EQUAL(list->capacity, 1);
}

TEST(array_list, ensureCapacity) {
	int i;
	arrayList_clear(list);

	LONGS_EQUAL(list->capacity, 10);
	LONGS_EQUAL(list->size, 0);

	for (i = 0; i < 100; i++) {
		bool added;
		std::string entry = "entry";
		added = arrayList_add(list, (char *) entry.c_str());
	}
	LONGS_EQUAL(list->capacity, 133);
	LONGS_EQUAL(list->size, 100);
}

TEST(array_list, clone) {
	int i;
	arrayList_clear(list);

	LONGS_EQUAL(list->capacity, 10);
	LONGS_EQUAL(list->size, 0);

	for (i = 0; i < 12; i++) {
		bool added;
		char entry[11];
		sprintf(entry, "|%s|%d|", "entry", i);
		added = arrayList_add(list, entry);
	}
	LONGS_EQUAL(16, list->capacity);
	LONGS_EQUAL(12, list->size);

	array_list_pt clone = NULL;
	clone = arrayList_clone(list);

	LONGS_EQUAL(16, clone->capacity);
	LONGS_EQUAL(12, clone->size);

	unsigned int j;
	for (j = 0; j < 12; j++) {
		void *entry = NULL;
		char entrys[11];
		sprintf(entrys, "|%s|%d|", "entry", j);

		entry = arrayList_get(clone, j);
		STRCMP_EQUAL((char *) entry, entrys);
	}

	arrayList_destroy(clone);
}
