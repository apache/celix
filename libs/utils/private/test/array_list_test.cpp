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
#include <string.h>
#include <iostream>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "celix_array_list.h"
#include "array_list.h"
#include "array_list_private.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
	char *d = (char*) malloc (strlen (s) + 1);
	if (d == NULL) return NULL;
	strcpy (d,s);
	return d;
}

//----------------------TESTS GROUP DEFINES----------------------

TEST_GROUP(array_list) {
	array_list_pt list;

	void setup(){
		arrayList_create(&list);
	}
	void teardown() {
		arrayList_destroy(list);
	}
};

TEST_GROUP(array_list_iterator) {
	array_list_pt list;

	void setup(){
		arrayList_create(&list);
	}
	void teardown() {
		arrayList_destroy(list);
	}
};

//----------------------ARRAY LIST TESTS----------------------

TEST(array_list, create) {
	CHECK_C(list != NULL);
	LONGS_EQUAL(0, list->size);
}

TEST(array_list, trimToSize) {
	char * entry = my_strdup("entry");
	arrayList_clear(list);

	arrayList_add(list, entry);
	LONGS_EQUAL(1, list->size);
	LONGS_EQUAL(10, list->capacity);

	arrayList_trimToSize(list);
	LONGS_EQUAL(1, list->size);
	LONGS_EQUAL(1, list->capacity);

	free(entry);
}

TEST(array_list, ensureCapacity) {
	int i;
	arrayList_clear(list);

	LONGS_EQUAL(10, list->capacity);
	LONGS_EQUAL(0, list->size);

	for (i = 0; i < 100; i++) {
		char * entry = my_strdup("entry");
		arrayList_add(list, entry);
	}
	LONGS_EQUAL(133, list->capacity);
	LONGS_EQUAL(100, list->size);

	for (i = 99; i >= 0; i--) {
		free(arrayList_remove(list, i));
	}
}

TEST(array_list, clone) {
	int i;
	arrayList_clear(list);

	LONGS_EQUAL(10, list->capacity);
	LONGS_EQUAL(0, list->size);

	for (i = 0; i < 12; i++) {
		bool added;
		char entry[11];
		sprintf(entry, "|%s|%d|", "entry", i);
		added = arrayList_add(list, my_strdup(entry));
		CHECK(added);
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

	for (i = 11; i >= 0; i--) {
		free(arrayList_remove(list, i));
	}
	arrayList_destroy(clone);
}

TEST(array_list, size){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry");
	char * entry3 = my_strdup("entry");
	arrayList_clear(list);
	LONGS_EQUAL(0, list->size);

	arrayList_add(list, entry);
	LONGS_EQUAL(1, list->size);

	arrayList_add(list, entry2);
	LONGS_EQUAL(2, list->size);

	arrayList_add(list, entry3);
	LONGS_EQUAL(3, list->size);

	free(entry);
	free(entry2);
	free(entry3);
}

TEST(array_list, isEmpty){
	arrayList_clear(list);
	LONGS_EQUAL(0, list->size);
	CHECK(arrayList_isEmpty(list));
}

TEST(array_list, contains){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = NULL;
	bool contains;

	arrayList_clear(list);

	arrayList_add(list, entry);

	arrayList_add(list, entry2);

	CHECK(arrayList_contains(list, entry));
	CHECK(arrayList_contains(list, entry2));
	contains = arrayList_contains(list, NULL);
	CHECK(!contains);

	arrayList_add(list, entry3);

	CHECK(arrayList_contains(list, entry3));

	free(entry);
	free(entry2);
}

TEST(array_list, indexOf){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");

	arrayList_clear(list);

	arrayList_add(list, entry);

	arrayList_add(list, entry2);
	arrayList_add(list, entry2);
	arrayList_add(list, entry2);
	arrayList_add(list, entry2);

	LONGS_EQUAL(0, arrayList_indexOf(list, entry));
	LONGS_EQUAL(1, arrayList_indexOf(list, entry2));
	LONGS_EQUAL(0, arrayList_lastIndexOf(list, entry));
	LONGS_EQUAL(4, arrayList_lastIndexOf(list, entry2));

	free(entry);
	free(entry2);
}

TEST(array_list, get){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = NULL;
	char * get;

	arrayList_clear(list);

	arrayList_add(list, entry);
	arrayList_add(list, entry2);

	get = (char*) arrayList_get(list, 0);
	STRCMP_EQUAL(entry, get);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry2, get);

	arrayList_add(list, entry3);

	get = (char*) arrayList_get(list, 2);
	CHECK(get == NULL);

	get = (char*) arrayList_get(list, 42);
	CHECK(get == NULL);

	free(entry);
	free(entry2);
}

TEST(array_list, set){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	char * get;
	char * old;

	arrayList_clear(list);

	arrayList_add(list, entry);
	arrayList_add(list, entry2);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry2, get);

	old = (char*) arrayList_set(list, 1, entry3);
	STRCMP_EQUAL(entry2, old);
	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry3, get);

	free(entry);
	free(entry2);
	free(entry3);
}

TEST(array_list, add){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	char * get;

	arrayList_clear(list);

	arrayList_add(list, entry);
	arrayList_add(list, entry2);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry2, get);

	arrayList_addIndex(list, 1, entry3);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry3, get);

	get = (char*) arrayList_get(list, 2);
	STRCMP_EQUAL(entry2, get);

	free(entry);
	free(entry2);
	free(entry3);
}

TEST(array_list, addAll){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	char * get;
	array_list_pt toAdd;
	bool changed;

	arrayList_clear(list);

	arrayList_create(&toAdd);
	arrayList_add(toAdd, entry);
	arrayList_add(toAdd, entry2);

	arrayList_add(list, entry3);

	get = (char*) arrayList_get(list, 0);
	STRCMP_EQUAL(entry3, get);

	changed = arrayList_addAll(list, toAdd);
	CHECK(changed);
	LONGS_EQUAL(3, arrayList_size(list));

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry, get);

	get = (char*) arrayList_get(list, 2);
	STRCMP_EQUAL(entry2, get);

	free(entry);
	free(entry2);
	free(entry3);
	arrayList_destroy(toAdd);
}

TEST(array_list, remove){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	char * get;
	char * removed;

	arrayList_clear(list);

	arrayList_add(list, entry);
	arrayList_add(list, entry2);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry2, get);

	// Remove first entry
	removed = (char*) arrayList_remove(list, 0);
	STRCMP_EQUAL(entry, removed);

	// Check the new first entry
	get = (char*) arrayList_get(list, 0);
	STRCMP_EQUAL(entry2, get);

	// Add a new entry
	arrayList_add(list, entry3);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry3, get);

	free(entry);
	free(entry2);
	free(entry3);
}

TEST(array_list, removeElement){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	char * get;

	arrayList_clear(list);

	arrayList_add(list, entry);
	arrayList_add(list, entry2);

	// Remove entry
	CHECK(arrayList_removeElement(list, entry));

	// Check the new first element
	get = (char*) arrayList_get(list, 0);
	STRCMP_EQUAL(entry2, get);

	// Add a new entry
	arrayList_add(list, entry3);

	get = (char*) arrayList_get(list, 1);
	STRCMP_EQUAL(entry3, get);

	free(entry);
	free(entry2);
	free(entry3);
}

TEST(array_list, clear){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");

	arrayList_clear(list);

	arrayList_add(list, entry);
	arrayList_add(list, entry2);

	LONGS_EQUAL(2, arrayList_size(list));
	arrayList_clear(list);
	LONGS_EQUAL(0, arrayList_size(list));

	free(entry);
	free(entry2);
}

//----------------------ARRAY LIST ITERATOR TESTS----------------------

TEST(array_list_iterator, create){
	array_list_iterator_pt it_list = arrayListIterator_create(list);
	CHECK(it_list != NULL);
	POINTERS_EQUAL(list, it_list->list);
	arrayListIterator_destroy(it_list);
}

TEST(array_list_iterator, hasNext){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	array_list_iterator_pt it_list;

	arrayList_add(list, entry);
	arrayList_add(list, entry2);
	arrayList_add(list, entry3);
	it_list = arrayListIterator_create(list);
	CHECK(arrayListIterator_hasNext(it_list));

	arrayListIterator_next(it_list);
	CHECK(arrayListIterator_hasNext(it_list));

	arrayListIterator_next(it_list);
	CHECK(arrayListIterator_hasNext(it_list));

	arrayListIterator_next(it_list);
	CHECK(!arrayListIterator_hasNext(it_list));

	free(entry);
	free(entry2);
	free(entry3);
	arrayListIterator_destroy(it_list);
}

TEST(array_list_iterator, hasPrevious){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	array_list_iterator_pt it_list;

	arrayList_add(list, entry);
	arrayList_add(list, entry2);
	arrayList_add(list, entry3);

	it_list = arrayListIterator_create(list);
	arrayListIterator_next(it_list);
	arrayListIterator_next(it_list);
	arrayListIterator_next(it_list);
	CHECK(arrayListIterator_hasPrevious(it_list));

	arrayListIterator_previous(it_list);
	CHECK(arrayListIterator_hasPrevious(it_list));

	arrayListIterator_previous(it_list);
	CHECK(arrayListIterator_hasPrevious(it_list));

	arrayListIterator_previous(it_list);
	CHECK(!arrayListIterator_hasPrevious(it_list));

	free(entry);
	free(entry2);
	free(entry3);
	arrayListIterator_destroy(it_list);
}

TEST(array_list_iterator, next){
	char * entry = my_strdup("entry");
	char * entry2 = my_strdup("entry2");
	char * entry3 = my_strdup("entry3");
	array_list_iterator_pt it_list;

	arrayList_add(list, entry);
	arrayList_add(list, entry2);
	arrayList_add(list, entry3);
	it_list = arrayListIterator_create(list);
	STRCMP_EQUAL(entry, (char*) arrayListIterator_next(it_list));
	STRCMP_EQUAL(entry2, (char*) arrayListIterator_next(it_list));
	STRCMP_EQUAL(entry3, (char*) arrayListIterator_next(it_list));
	POINTERS_EQUAL(NULL, arrayListIterator_next(it_list));

	//mess up the expected and real changecount, code should check and handle
	arrayList_add(list, entry);
	arrayListIterator_next(it_list);

	free(entry);
	free(entry2);
	free(entry3);
	arrayListIterator_destroy(it_list);
}

TEST(array_list_iterator, previous){
	char * value = my_strdup("entry");
	char * value2 = my_strdup("entry2");
	char * value3 = my_strdup("entry3");
	array_list_iterator_pt it_list;

	arrayList_add(list, value);
	arrayList_add(list, value2);
	arrayList_add(list, value3);
	it_list = arrayListIterator_create(list);

	arrayListIterator_next(it_list);
	arrayListIterator_next(it_list);
	arrayListIterator_next(it_list);
	STRCMP_EQUAL(value3, (char*) arrayListIterator_previous(it_list));
	STRCMP_EQUAL(value2, (char*) arrayListIterator_previous(it_list));
	STRCMP_EQUAL(value, (char*) arrayListIterator_previous(it_list));
	POINTERS_EQUAL(NULL, arrayListIterator_previous(it_list));

	//mess up the expected and real changecount, code should check and handle
	arrayListIterator_destroy(it_list);
	it_list = arrayListIterator_create(list);
	arrayList_add(list, value);
	arrayListIterator_previous(it_list);

	free(value);
	free(value2);
	free(value3);
	arrayListIterator_destroy(it_list);
}

TEST(array_list_iterator, remove){
	char * value = my_strdup("entry");
	char * value2 = my_strdup("entry2");
	char * value3 = my_strdup("entry3");
	array_list_iterator_pt it_list;

	arrayList_add(list, value);
	arrayList_add(list, value2);
	arrayList_add(list, value3);
	it_list = arrayListIterator_create(list);

	LONGS_EQUAL(3, list->size);
	STRCMP_EQUAL(value, (char*) arrayList_get(list, 0));
	STRCMP_EQUAL(value2, (char*) arrayList_get(list, 1));
	STRCMP_EQUAL(value3, (char*) arrayList_get(list, 2));

	arrayListIterator_next(it_list);
	arrayListIterator_next(it_list);
	arrayListIterator_remove(it_list);
	LONGS_EQUAL(2, list->size);
	STRCMP_EQUAL(value, (char*) arrayList_get(list, 0));
	STRCMP_EQUAL(value3, (char*) arrayList_get(list, 1));

	//mess up the expected and real changecount, code should check and handle
	arrayList_add(list, value);
	arrayListIterator_remove(it_list);

	free(value);
	free(value2);
	free(value3);
	arrayListIterator_destroy(it_list);
}


template<typename I>
void testCelixArrayForType(void(*add)(celix_array_list_t *, I val), void(*rem)(celix_array_list_t *list, I val), I val1, I val2, I val3) {
    celix_array_list_t *array = celix_arrayList_create();
    add(array, val1);
    add(array, val2);
    add(array, val3);

    CHECK_EQUAL(3, celix_arrayList_size(array));

    rem(array, val2);
    CHECK_EQUAL(2, celix_arrayList_size(array));
    rem(array, val3);
    CHECK_EQUAL(1, celix_arrayList_size(array));

    celix_arrayList_destroy(array);
}

TEST(array_list_iterator, addAndRemovesTypes) {
    testCelixArrayForType(celix_arrayList_add, celix_arrayList_remove, (void*)0x41, (void*)0x42, (void*)0x43);
    testCelixArrayForType(celix_arrayList_addInt, celix_arrayList_removeInt, 41, -42, 43);
    testCelixArrayForType(celix_arrayList_addLong, celix_arrayList_removeLong, 41L, -42L, 43L);
    testCelixArrayForType(celix_arrayList_addUInt, celix_arrayList_removeUInt, 41U, 42U, 43U);
    testCelixArrayForType(celix_arrayList_addULong, celix_arrayList_removeULong, 41UL, 42UL, 43UL);
    testCelixArrayForType(celix_arrayList_addFloat, celix_arrayList_removeFloat, 41.2f, 42.2f, 43.2f);
    testCelixArrayForType(celix_arrayList_addDouble, celix_arrayList_removeDouble, 41.2, 42.2, 43.2);
    //testCelixArrayForType(celix_arrayList_addBool, celix_arrayList_removeBool, true, false, true);
    testCelixArrayForType(celix_arrayList_addSize, celix_arrayList_removeSize, (size_t)41, (size_t)42, (size_t)43);
}
