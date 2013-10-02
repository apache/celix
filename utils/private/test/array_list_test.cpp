/*
 * array_list_test.cpp
 *
 *  Created on: Jun 6, 2012
 *      Author: alexander
 */

#include "CPPUTest/TestHarness.h"
#include "CPPUTest/TestHarness_c.h"
#include "CPPUTest/CommandLineTestRunner.h"

extern "C"
{
#include "array_list.h"
#include "array_list_private.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(array_list) {
	array_list_t list;
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);

		arrayList_create(pool, &list);
	}
};


TEST(array_list, create) {
	CHECK_C(list != NULL);
    LONGS_EQUAL(0, list->size);
}

TEST(array_list, trimToSize) {
	bool added;
	char * entry;
	arrayList_clear(list);

	entry = "entry";
	added = arrayList_add(list, entry);
	LONGS_EQUAL(list->size, 1);
	LONGS_EQUAL(list->capacity, 10);

	arrayList_trimToSize(list);
	LONGS_EQUAL(list->size, 1);
	LONGS_EQUAL(list->capacity, 1);
}

TEST(array_list, ensureCapacity) {
	int i;
	arrayList_create(pool, &list);
	arrayList_clear(list);

	LONGS_EQUAL(list->capacity, 10);
	LONGS_EQUAL(list->size, 0);

	for (i = 0; i < 100; i++) {
		bool added;
		char *entry = "entry";
		added = arrayList_add(list, entry);
	}
	LONGS_EQUAL(list->capacity, 133);
	LONGS_EQUAL(list->size, 100);
	arrayList_create(pool, &list);
}

TEST(array_list, clone) {
	int i;
	arrayList_create(pool, &list);
	arrayList_clear(list);

	LONGS_EQUAL(list->capacity, 10);
	LONGS_EQUAL(list->size, 0);

	for (i = 0; i < 12; i++) {
		bool added;
		char entry[11];
		sprintf(entry, "|%s|%d|", "entry", i);
		added = arrayList_add(list, strdup(entry));
	}
	LONGS_EQUAL(16, list->capacity);
	LONGS_EQUAL(12, list->size);

	array_list_t clone = NULL;
	clone = arrayList_clone(pool, list);

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

	arrayList_create(pool, &list);
}
