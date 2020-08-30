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
 * linked_list_test.cpp
 *
 *  \date       Sep 15, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C"
{
#include "linked_list.h"
#include "linked_list_private.h"
#include "linked_list_iterator.h"
}

int main(int argc, char** argv) {
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s){
    char *d = (char*) malloc (strlen (s) + 1);
    if (d == NULL) return NULL;
    strcpy (d,s);
    return d;
}

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(linked_list){
    linked_list_pt list;

    void setup(void) {
        linkedList_create(&list);
    }

    void teardown(void) {
        linkedList_destroy(list);
    }
};

TEST_GROUP(linked_list_iterator){
    linked_list_pt list;

    void setup(void) {
        linkedList_create(&list);
    }

    void teardown(void) {
        linkedList_destroy(list);
    }
};

//----------------------LINKED LIST TESTS----------------------

TEST(linked_list, create){
    CHECK(list != NULL);
    LONGS_EQUAL(0, linkedList_size(list));
}

TEST(linked_list, add){
    char * value = my_strdup("element");

    LONGS_EQUAL(0, linkedList_size(list));
    linkedList_addElement(list, value);
    LONGS_EQUAL(1, linkedList_size(list));

    free(value);
}

TEST(linked_list, addFirst){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);

    STRCMP_EQUAL(value, (char*) linkedList_getFirst(list));

    linkedList_addFirst(list, value3);

    STRCMP_EQUAL(value3, (char*) linkedList_getFirst(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, addLast){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);

    STRCMP_EQUAL(value2, (char*) linkedList_getLast(list));

    linkedList_addLast(list, value3);

    STRCMP_EQUAL(value3, (char*) linkedList_getLast(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, addIndex){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    char * value4 = my_strdup("element4");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    linkedList_addIndex(list, 2, value4);

    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value4, (char*) linkedList_get(list, 2));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 3));

    free(value);
    free(value2);
    free(value3);
    free(value4);
}

TEST(linked_list, addBefore){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    char * value4 = my_strdup("element4");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    linkedList_addBefore(list, value4, linkedList_entry(list, 2));

    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value4, (char*) linkedList_get(list, 2));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 3));

    free(value);
    free(value2);
    free(value3);
    free(value4);
}

TEST(linked_list, remove){
    char * value = strdup("element");

    LONGS_EQUAL(0, linkedList_size(list));
    linkedList_addElement(list, value);
    LONGS_EQUAL(1, linkedList_size(list));

    linkedList_removeElement(list, value);
    LONGS_EQUAL(0, linkedList_size(list));

    free(value);
}

TEST(linked_list, removeFirst){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value, (char*) linkedList_getFirst(list));

    linkedList_removeFirst(list);

    STRCMP_EQUAL(value2, (char*) linkedList_getFirst(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, removeLast){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value3, (char*) linkedList_getLast(list));

    linkedList_removeLast(list);

    STRCMP_EQUAL(value2, (char*) linkedList_getLast(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, removeElement){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    //add 6 elements
    linkedList_addElement(list, NULL);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, NULL);
    linkedList_addElement(list, value3);
    LONGS_EQUAL(6, list->size);

    linkedList_removeElement(list, NULL);
    linkedList_removeElement(list, NULL);
    LONGS_EQUAL(4, list->size);

    linkedList_removeElement(list, value2);
    LONGS_EQUAL(3, list->size);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, removeIndex){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));

    linkedList_removeIndex(list, 1);

    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 1));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, removeEntry){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    linkedList_removeEntry(list, list->header);
    LONGS_EQUAL(3, list->size);

    linkedList_removeEntry(list, linkedList_entry(list, 0));
    LONGS_EQUAL(2, list->size);

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, clone){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_pt list2;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    linkedList_clone(list, &list2);
    LONGS_EQUAL(list->size, list2->size);
    STRCMP_EQUAL( (char*) linkedList_getFirst(list), (char*) linkedList_getFirst(list2));
    STRCMP_EQUAL( (char*) linkedList_getLast(list), (char*) linkedList_getLast(list2));

    free(value);
    free(value2);
    free(value3);
    linkedList_destroy(list2);
}

TEST(linked_list, clear){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    LONGS_EQUAL(3, list->size);

    linkedList_clear(list);
    LONGS_EQUAL(0, list->size);

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, get){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, getFirst){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value, (char*) linkedList_getFirst(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, getLast){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value3, (char*) linkedList_getLast(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, set){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    linkedList_set(list, 1, value3);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, contains){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    linkedList_addElement(list, value);
    CHECK(linkedList_contains(list, value));
    CHECK(!linkedList_contains(list, value2));
    CHECK(!linkedList_contains(list, value3));

    linkedList_addElement(list, value2);
    CHECK(linkedList_contains(list, value));
    CHECK(linkedList_contains(list, value2));
    CHECK(!linkedList_contains(list, value3));

    linkedList_addElement(list, value3);
    CHECK(linkedList_contains(list, value));
    CHECK(linkedList_contains(list, value2));
    CHECK(linkedList_contains(list, value3));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, size){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    LONGS_EQUAL(0, linkedList_size(list));

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    LONGS_EQUAL(3, linkedList_size(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, isEmpty){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    CHECK(linkedList_isEmpty(list));

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    CHECK(!linkedList_isEmpty(list));

    linkedList_clear(list);
    CHECK(linkedList_isEmpty(list));

    free(value);
    free(value2);
    free(value3);
}

TEST(linked_list, entry){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    char * value4 = my_strdup("element4");
    char * value5 = my_strdup("element5");

    POINTERS_EQUAL(NULL, linkedList_entry(list, 666));

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    linkedList_addElement(list, value4);
    linkedList_addElement(list, value5);

    STRCMP_EQUAL(value2, (char*) linkedList_entry(list, 1)->element);
    STRCMP_EQUAL(value4, (char*) linkedList_entry(list, 3)->element);

    free(value);
    free(value2);
    free(value3);
    free(value4);
    free(value5);
}

TEST(linked_list, indexOf){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");

    LONGS_EQUAL(-1, linkedList_indexOf(list, value));
    LONGS_EQUAL(-1, linkedList_indexOf(list, value2));
    LONGS_EQUAL(-1, linkedList_indexOf(list, value3));
    LONGS_EQUAL(-1, linkedList_indexOf(list, NULL));

    linkedList_addElement(list, value);
    LONGS_EQUAL(0, linkedList_indexOf(list, value));

    linkedList_addElement(list, value2);
    LONGS_EQUAL(1, linkedList_indexOf(list, value2));

    linkedList_addElement(list, value3);
    LONGS_EQUAL(2, linkedList_indexOf(list, value3));

    linkedList_addElement(list, NULL);
    LONGS_EQUAL(3, linkedList_indexOf(list, NULL));

    free(value);
    free(value2);
    free(value3);
}

//----------------------LINKED LIST ITERATOR TESTS----------------------

TEST(linked_list_iterator, create){
    linked_list_iterator_pt it_list = linkedListIterator_create(list,0);
    CHECK(it_list != NULL);
    linkedListIterator_destroy(it_list);

    it_list = linkedListIterator_create(list,666);
    POINTERS_EQUAL(NULL, it_list);
}

TEST(linked_list_iterator, hasNext){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    it_list = linkedListIterator_create(list, 0);
    CHECK(linkedListIterator_hasNext(it_list));

    linkedListIterator_next(it_list);
    CHECK(linkedListIterator_hasNext(it_list));

    linkedListIterator_next(it_list);
    CHECK(linkedListIterator_hasNext(it_list));

    linkedListIterator_next(it_list);
    CHECK(!linkedListIterator_hasNext(it_list));

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, hasPrevious){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);

    it_list = linkedListIterator_create(list, 3);
    CHECK(linkedListIterator_hasPrevious(it_list));

    linkedListIterator_previous(it_list);
    CHECK(linkedListIterator_hasPrevious(it_list));

    linkedListIterator_previous(it_list);
    CHECK(linkedListIterator_hasPrevious(it_list));

    linkedListIterator_previous(it_list);
    CHECK(!linkedListIterator_hasPrevious(it_list));

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, next){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 0);
    STRCMP_EQUAL(value, (char*) linkedListIterator_next(it_list));
    STRCMP_EQUAL(value2, (char*) linkedListIterator_next(it_list));
    STRCMP_EQUAL(value3, (char*) linkedListIterator_next(it_list));
    POINTERS_EQUAL(NULL, linkedListIterator_next(it_list));

    //mess up the expected and real changecount, code should check and handle
    linkedList_addElement(list, value);
    linkedListIterator_next(it_list);

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, previous){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 3);
    STRCMP_EQUAL(value3, (char*) linkedListIterator_previous(it_list));
    STRCMP_EQUAL(value2, (char*) linkedListIterator_previous(it_list));
    STRCMP_EQUAL(value, (char*) linkedListIterator_previous(it_list));
    POINTERS_EQUAL(NULL, linkedListIterator_previous(it_list));

    //mess up the expected and real changecount, code should check and handle
    linkedListIterator_destroy(it_list);
    it_list = linkedListIterator_create(list, 3);
    linkedList_addElement(list, value);
    linkedListIterator_previous(it_list);

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, nextIndex){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 0);
    LONGS_EQUAL(0, linkedListIterator_nextIndex(it_list));
    linkedListIterator_next(it_list);
    LONGS_EQUAL(1, linkedListIterator_nextIndex(it_list));
    linkedListIterator_next(it_list);
    LONGS_EQUAL(2, linkedListIterator_nextIndex(it_list));

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, previousIndex){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 3);
    LONGS_EQUAL(2, linkedListIterator_previousIndex(it_list));
    linkedListIterator_previous(it_list);
    LONGS_EQUAL(1, linkedListIterator_previousIndex(it_list));
    linkedListIterator_previous(it_list);
    LONGS_EQUAL(0, linkedListIterator_previousIndex(it_list));

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, set){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 0);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    linkedListIterator_set(it_list, NULL);
    linkedListIterator_next(it_list);
    linkedListIterator_next(it_list);
    linkedListIterator_set(it_list, value3);
    linkedListIterator_next(it_list);
    linkedListIterator_set(it_list, value2);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 2));

    //mess up the expected and real changecount, code should check and handle
    linkedList_addElement(list, value);
    linkedListIterator_set(it_list, value);

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, add){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 0);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 1));

    linkedListIterator_next(it_list);
    linkedListIterator_add(it_list, value2);

    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    //mess up the expected and real changecount, code should check and handle
    linkedList_addElement(list, value);
    linkedListIterator_add(it_list, value2);

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}

TEST(linked_list_iterator, remove){
    char * value = my_strdup("element");
    char * value2 = my_strdup("element2");
    char * value3 = my_strdup("element3");
    linked_list_iterator_pt it_list;

    linkedList_addElement(list, value);
    linkedList_addElement(list, value2);
    linkedList_addElement(list, value3);
    it_list = linkedListIterator_create(list, 0);

    LONGS_EQUAL(3, list->size);
    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value2, (char*) linkedList_get(list, 1));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 2));

    linkedListIterator_next(it_list);
    linkedListIterator_next(it_list);
    linkedListIterator_remove(it_list);
    LONGS_EQUAL(2, list->size);
    STRCMP_EQUAL(value, (char*) linkedList_get(list, 0));
    STRCMP_EQUAL(value3, (char*) linkedList_get(list, 1));

    //mess up the expected and real changecount, code should check and handle
    linkedList_addElement(list, value);
    linkedListIterator_remove(it_list);

    free(value);
    free(value2);
    free(value3);
    linkedListIterator_destroy(it_list);
}
