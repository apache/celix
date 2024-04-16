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

#include "celix_array_list.h"
#include "celix_version.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_filter.h"

class ArrayListTestSuite : public ::testing::Test {
public:
};

TEST_F(ArrayListTestSuite, CreateDestroyArrayListTest) {
    auto* list = celix_arrayList_create();
    EXPECT_TRUE(list != nullptr);
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, ArrayListWithEqualsTest) {
    celix_array_list_create_options_t opts{};
    opts.equalsCallback = [](celix_array_list_entry_t a, celix_array_list_entry_t b) -> bool {
        const char* sa = (char*)a.voidPtrVal;
        const char* sb = (char*)b.voidPtrVal;
        return celix_utils_stringEquals(sa, sb);
    };
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createWithOptions(&opts);
    EXPECT_EQ(0, celix_arrayList_size(list));

    celix_arrayList_add(list, (void*)"val0");
    celix_arrayList_add(list, (void*)"val1");
    celix_arrayList_add(list, (void*)"val2");
    celix_arrayList_add(list, (void*)"val3");
    EXPECT_EQ(celix_arrayList_size(list), 4);

    //std string to ensure pointer is different.
    std::string val0{"val0"};
    std::string val1{"val1"};
    std::string val2{"val2"};
    std::string val3{"val3"};
    std::string val4{"val4"};

    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = (void*)val0.c_str();
    EXPECT_EQ(celix_arrayList_indexOf(list, entry), 0);
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = (void*)val1.c_str();
    EXPECT_EQ(celix_arrayList_indexOf(list, entry), 1);
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = (void*)val2.c_str();
    EXPECT_EQ(celix_arrayList_indexOf(list, entry), 2);
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = (void*)val3.c_str();
    EXPECT_EQ(celix_arrayList_indexOf(list, entry), 3);
    memset(&entry, 0, sizeof(entry));
    entry.voidPtrVal = (void*)val4.c_str();
    EXPECT_EQ(celix_arrayList_indexOf(list, entry), -1); //note present

    celix_arrayList_remove(list, (void*)val2.c_str()); //index 3
    celix_arrayList_removeAt(list, 0); //val0
    EXPECT_EQ(celix_arrayList_size(list), 2);
    EXPECT_STREQ((char*)celix_arrayList_get(list, 0), "val1");
    EXPECT_STREQ((char*)celix_arrayList_get(list, 1), "val3");
}

template<typename T>
void testArrayListForTemplateType(const std::vector<T>& entries,
                                  const std::function<celix_array_list_t*()>& create,
                                  const std::function<celix_status_t(celix_array_list_t*, T)>& add,
                                  const std::function<T(celix_array_list_t*, int)>& get,
                                  const std::function<void(celix_array_list_t*, T)>& remove) {
    auto* list = create();
    //fill
    for (const auto& entry : entries) {
        add(list, entry);
    }
    EXPECT_EQ(celix_arrayList_size(list), entries.size());

    //get
    for (int i = 0; i < (int)entries.size(); ++i) {
        EXPECT_EQ(get(list, i), entries[i]);
    }

    //remove
    for (int i = 0; i < (int)entries.size(); ++i) {
        remove(list, entries[i]);
    }
    EXPECT_EQ(celix_arrayList_size(list), 0);

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, TestDifferentEntyTypesForArrayListTest) {
    std::vector<long> longEntries{1L, 2L, 3L, 4L, 5L};
    testArrayListForTemplateType<long>(longEntries,
                                       celix_arrayList_createLongArray,
                                       celix_arrayList_addLong,
                                       celix_arrayList_getLong,
                                       celix_arrayList_removeLong);

    std::vector<double> doubleEntries{1.0, 2.0, 3.0, 4.0, 5.0};
    testArrayListForTemplateType<double>(doubleEntries,
                                         celix_arrayList_createDoubleArray,
                                         celix_arrayList_addDouble,
                                         celix_arrayList_getDouble,
                                         celix_arrayList_removeDouble);

    std::vector<bool> boolEntries{true, false, true, false, true};
    testArrayListForTemplateType<bool>(boolEntries,
                                       celix_arrayList_createBoolArray,
                                       celix_arrayList_addBool,
                                       celix_arrayList_getBool,
                                       celix_arrayList_removeBool);

    std::vector<void*> voidPtrEntries{(void*)0x11, (void*)0x22, (void*)0x33, (void*)0x44, (void*)0x55};
    testArrayListForTemplateType<void*>(voidPtrEntries,
                                        celix_arrayList_createPointerArray,
                                        celix_arrayList_add,
                                        celix_arrayList_get,
                                        celix_arrayList_remove);
}

TEST_F(ArrayListTestSuite, StringArrayList) {
    celix_autoptr(celix_array_list_t) stringList = celix_arrayList_createStringArray();
    const char* str1 = "1";
    celix_arrayList_addString(stringList, str1);
    celix_arrayList_addString(stringList, "2");
    celix_arrayList_assignString(stringList, strdup("3"));

    EXPECT_EQ(3, celix_arrayList_size(stringList));
    EXPECT_STREQ("1", celix_arrayList_getString(stringList, 0));
    EXPECT_NE((void*)str1, (void*)celix_arrayList_getString(stringList, 0)); //string is added as copy
    EXPECT_STREQ("2", celix_arrayList_getString(stringList, 1));
    EXPECT_STREQ("3", celix_arrayList_getString(stringList, 2));

    celix_arrayList_removeString(stringList, "2");
    EXPECT_EQ(2, celix_arrayList_size(stringList));
}

TEST_F(ArrayListTestSuite, VersionArrayList) {
    celix_autoptr(celix_array_list_t) versionList = celix_arrayList_createVersionArray();
    celix_version_t* v1 = celix_version_create(1, 2, 3, "a");
    celix_arrayList_addVersion(versionList, v1); //copy
    celix_arrayList_assignVersion(versionList, v1); //transfer ownership
    celix_arrayList_assignVersion(versionList, celix_version_create(2, 3, 4, "b"));

    EXPECT_EQ(3, celix_arrayList_size(versionList));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 0), 1, 2));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 1), 1, 2));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 2), 2, 3));

    EXPECT_NE((void*)v1, (void*)celix_arrayList_getVersion(versionList, 0)); //version is added as copy
    EXPECT_EQ((void*)v1, (void*)celix_arrayList_getVersion(versionList, 1)); //version is added as reference

    celix_autoptr(celix_version_t) vRef = celix_version_create(1, 2, 3, "a");
    celix_arrayList_removeVersion(versionList, vRef);
    EXPECT_EQ(2, celix_arrayList_size(versionList));
}

TEST_F(ArrayListTestSuite, SortTypedArrayListsTest) {
    // Given a ptr, string, int, long, uint, ulong, float, double, bool, size and version list
    // with unsorted values (including duplicates)
    celix_autoptr(celix_array_list_t) ptrList = celix_arrayList_createPointerArray();
    celix_arrayList_add(ptrList, (void*)0x33);
    celix_arrayList_add(ptrList, (void*)0x11);
    celix_arrayList_add(ptrList, (void*)0x22);
    celix_arrayList_add(ptrList, (void*)0x11);

    celix_autoptr(celix_array_list_t) stringList = celix_arrayList_createStringArray();
    celix_arrayList_addString(stringList, "3");
    celix_arrayList_addString(stringList, "1");
    celix_arrayList_addString(stringList, "2");
    celix_arrayList_addString(stringList, "1");


    celix_autoptr(celix_array_list_t) longList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList, 3L);
    celix_arrayList_addLong(longList, 1L);
    celix_arrayList_addLong(longList, 2L);
    celix_arrayList_addLong(longList, 1L);

    celix_autoptr(celix_array_list_t) doubleList = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleList, 3.0);
    celix_arrayList_addDouble(doubleList, 1.0);
    celix_arrayList_addDouble(doubleList, 2.0);
    celix_arrayList_addDouble(doubleList, 1.0);

    celix_autoptr(celix_array_list_t) boolList = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolList, false);
    celix_arrayList_addBool(boolList, true);
    celix_arrayList_addBool(boolList, false);

    celix_autoptr(celix_array_list_t) versionList = celix_arrayList_createVersionArray();
    celix_arrayList_assignVersion(versionList, celix_version_create(2, 1, 0, ""));
    celix_arrayList_assignVersion(versionList, celix_version_create(3, 2, 1, ""));
    celix_arrayList_assignVersion(versionList, celix_version_create(2, 1, 0, ""));
    celix_arrayList_assignVersion(versionList, celix_version_create(1, 0, 0, "b"));
    celix_arrayList_assignVersion(versionList, celix_version_create(1, 0, 0, "a"));

    // Then the element type is correctly set
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER, celix_arrayList_getElementType(ptrList));
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING, celix_arrayList_getElementType(stringList));
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG, celix_arrayList_getElementType(longList));
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE, celix_arrayList_getElementType(doubleList));
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL, celix_arrayList_getElementType(boolList));
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION, celix_arrayList_getElementType(versionList));

    // When sorting the lists
    celix_arrayList_sort(ptrList);
    celix_arrayList_sort(stringList);
    celix_arrayList_sort(longList);
    celix_arrayList_sort(doubleList);
    celix_arrayList_sort(boolList);
    celix_arrayList_sort(versionList);

    // Then the lists are sorted
    EXPECT_EQ((void*)0x11, celix_arrayList_get(ptrList, 0));
    EXPECT_EQ((void*)0x11, celix_arrayList_get(ptrList, 1));
    EXPECT_EQ((void*)0x22, celix_arrayList_get(ptrList, 2));
    EXPECT_EQ((void*)0x33, celix_arrayList_get(ptrList, 3));

    EXPECT_STREQ("1", celix_arrayList_getString(stringList, 0));
    EXPECT_STREQ("1", celix_arrayList_getString(stringList, 1));
    EXPECT_STREQ("2", celix_arrayList_getString(stringList, 2));
    EXPECT_STREQ("3", celix_arrayList_getString(stringList, 3));

    EXPECT_EQ(1L, celix_arrayList_getLong(longList, 0));
    EXPECT_EQ(1L, celix_arrayList_getLong(longList, 1));
    EXPECT_EQ(2L, celix_arrayList_getLong(longList, 2));
    EXPECT_EQ(3L, celix_arrayList_getLong(longList, 3));

    EXPECT_DOUBLE_EQ(1.0, celix_arrayList_getDouble(doubleList, 0));
    EXPECT_DOUBLE_EQ(1.0, celix_arrayList_getDouble(doubleList, 1));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(doubleList, 2));
    EXPECT_DOUBLE_EQ(3.0, celix_arrayList_getDouble(doubleList, 3));

    EXPECT_FALSE(celix_arrayList_getBool(boolList, 0));
    EXPECT_FALSE(celix_arrayList_getBool(boolList, 1));
    EXPECT_TRUE(celix_arrayList_getBool(boolList, 2));

    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 0), 1, 0));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 1), 1, 0));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 2), 2, 1));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 3), 2, 1));
    EXPECT_EQ(0, celix_version_compareToMajorMinor(celix_arrayList_getVersion(versionList, 4), 3, 2));
}

TEST_F(ArrayListTestSuite, EqualCheckTest) {
    //Given a selection of long list and a double list
    celix_autoptr(celix_array_list_t) list1 = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list1, 1L);
    celix_autoptr(celix_array_list_t) list2 = celix_arrayList_createLongArray(); //same as list1
    celix_arrayList_addLong(list2, 1L);
    celix_autoptr(celix_array_list_t) list3 = celix_arrayList_createLongArray(); //different values than list1
    celix_arrayList_addLong(list3, 2L);
    celix_autoptr(celix_array_list_t) list4 = celix_arrayList_createLongArray(); //different size than list1
    celix_arrayList_addLong(list4, 1L);
    celix_arrayList_addLong(list4, 2L);
    celix_autoptr(celix_array_list_t) list5 = celix_arrayList_createDoubleArray(); //different type than list1
    celix_arrayList_addDouble(list5, 1.0);

    // And 2 custom pointer list, with different equals callbacks
    celix_array_list_create_options_t opts{};
    opts.elementType = CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER;
    opts.equalsCallback = [](celix_array_list_entry_t, celix_array_list_entry_t) -> bool {
        return true; //dummy equals callback
    };
    celix_autoptr(celix_array_list_t) list6 = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_add(list6, (void*)1);
    opts.equalsCallback = [](celix_array_list_entry_t, celix_array_list_entry_t) -> bool {
        return false; //dummy equals callback
    };
    celix_autoptr(celix_array_list_t) list7 = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_add(list7, (void*)1);


    //The lists can be checked for equality
    EXPECT_TRUE(celix_arrayList_equals(list1, list2));
    EXPECT_TRUE(celix_arrayList_equals(list1, list1));
    EXPECT_TRUE(celix_arrayList_equals(nullptr, nullptr));

    EXPECT_FALSE(celix_arrayList_equals(nullptr, list1));
    EXPECT_FALSE(celix_arrayList_equals(list1, nullptr));
    EXPECT_FALSE(celix_arrayList_equals(list1, list3));
    EXPECT_FALSE(celix_arrayList_equals(list1, list4));
    EXPECT_FALSE(celix_arrayList_equals(list1, list5));
    EXPECT_FALSE(celix_arrayList_equals(list6, list7));
}

TEST_F(ArrayListTestSuite, CopyArrayTest) {
    // Given a long, string, string ref and version list
    celix_autoptr(celix_array_list_t) longList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList, 1L);
    celix_arrayList_addLong(longList, 2L);
    celix_arrayList_addLong(longList, 3L);

    celix_autoptr(celix_array_list_t) stringList = celix_arrayList_createStringArray();
    celix_arrayList_addString(stringList, "1");
    celix_arrayList_addString(stringList, "2");
    celix_arrayList_addString(stringList, "3");

    celix_autoptr(celix_array_list_t) versionList = celix_arrayList_createVersionArray();
    celix_arrayList_assignVersion(versionList, celix_version_create(1, 0, 0, ""));
    celix_arrayList_assignVersion(versionList, celix_version_create(2, 0, 0, ""));
    celix_arrayList_assignVersion(versionList, celix_version_create(3, 0, 0, ""));

    // And a custom pointer list configured for handling celix_filter_t
    celix_array_list_create_options_t opts{};
    opts.elementType = CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER;
    opts.simpleRemovedCallback = [](void* d) { celix_filter_destroy((celix_filter_t*)d); };
    opts.copyCallback = [](celix_array_list_entry_t src, celix_array_list_entry_t* dst) -> celix_status_t {
        auto* filter = (celix_filter_t*)src.voidPtrVal;
        dst->voidPtrVal = celix_filter_create(celix_filter_getFilterString(filter));
        return (dst->voidPtrVal) ? CELIX_SUCCESS : CELIX_ENOMEM;
    };
    opts.equalsCallback = [](celix_array_list_entry_t a, celix_array_list_entry_t b) -> bool {
        auto* fa = (celix_filter_t*)a.voidPtrVal;
        auto* fb = (celix_filter_t*)b.voidPtrVal;
        return celix_utils_stringEquals(celix_filter_getFilterString(fa), celix_filter_getFilterString(fb));
    };
    celix_autoptr(celix_array_list_t) ptrList = celix_arrayList_createWithOptions(&opts);
    celix_filter_t* f1 = celix_filter_create("(a=1)");
    celix_filter_t* f2 = celix_filter_create("(a=2)");
    celix_filter_t* f3 = celix_filter_create("(a=3)");
    celix_arrayList_add(ptrList, f1);
    celix_arrayList_add(ptrList, f2);
    celix_arrayList_add(ptrList, f3);

    // When copying the lists
    celix_autoptr(celix_array_list_t) longListCopy = celix_arrayList_copy(longList);
    celix_autoptr(celix_array_list_t) stringListCopy = celix_arrayList_copy(stringList);
    celix_autoptr(celix_array_list_t) versionListCopy = celix_arrayList_copy(versionList);
    celix_autoptr(celix_array_list_t) ptrListCopy = celix_arrayList_copy(ptrList);

    // Then the size of the copied array list is 3
    EXPECT_EQ(3, celix_arrayList_size(longListCopy));
    EXPECT_EQ(3, celix_arrayList_size(stringListCopy));
    EXPECT_EQ(3, celix_arrayList_size(versionListCopy));
    EXPECT_EQ(3, celix_arrayList_size(ptrListCopy));

    // And the copied lists are equal to the original lists
    EXPECT_TRUE(celix_arrayList_equals(longList, longListCopy));
    EXPECT_TRUE(celix_arrayList_equals(stringList, stringListCopy));
    EXPECT_TRUE(celix_arrayList_equals(versionList, versionListCopy));
    EXPECT_TRUE(celix_arrayList_equals(ptrList, ptrListCopy));

    auto* result = celix_arrayList_copy(nullptr);
    EXPECT_EQ(nullptr, result);
}

TEST_F(ArrayListTestSuite, SimpleRemovedCallbacksForArrayListTest) {
    celix_array_list_create_options_t opts{};
    opts.simpleRemovedCallback = free;
    auto* list = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_add(list, (void*) celix_utils_strdup("value"));
    celix_arrayList_add(list, (void*) celix_utils_strdup("value"));
    celix_arrayList_add(list, (void*) celix_utils_strdup("value"));
    celix_arrayList_add(list, (void*) celix_utils_strdup("value"));
    EXPECT_EQ(celix_arrayList_size(list), 4);
    celix_arrayList_destroy(list); //will call free for every entry
}

TEST_F(ArrayListTestSuite, AddStringToArrayListOfUndefinedTypeTest) {
    celix_array_list_create_options_t opts{};
    opts.simpleRemovedCallback = free;
    auto* list = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_addString(list, celix_utils_strdup("value"));
    celix_arrayList_addString(list, celix_utils_strdup("value"));
    celix_arrayList_addString(list, celix_utils_strdup("value"));
    celix_arrayList_addString(list, celix_utils_strdup("value"));
    EXPECT_EQ(celix_arrayList_size(list), 4);
    celix_arrayList_destroy(list); //will call free for every entry
}

TEST_F(ArrayListTestSuite, AddVersionToArrayListOfUndefinedTypeTest) {
    celix_array_list_create_options_t opts{};
    opts.simpleRemovedCallback = [](void* data) {
        celix_version_destroy((celix_version_t*)data);
    };
    auto* list = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_addVersion(list, celix_version_create(1, 3, 0, nullptr));
    celix_arrayList_addVersion(list, celix_version_create(1, 3, 0, nullptr));
    celix_arrayList_addVersion(list, celix_version_create(1, 3, 0, nullptr));
    celix_arrayList_addVersion(list, celix_version_create(1, 3, 0, nullptr));
    EXPECT_EQ(celix_arrayList_size(list), 4);
    celix_arrayList_destroy(list); //will call free for every entry
}

TEST_F(ArrayListTestSuite, RemovedCallbacksForArrayListTest) {
    int count = 0 ;
    celix_array_list_create_options_t opts{};
    opts.removedCallbackData = &count;
    opts.removedCallback = [](void *data, celix_array_list_entry_t entry) {
        int* c = (int*)data;
        if (entry.longVal == 1 || entry.longVal == 2 || entry.longVal == 3 || entry.longVal == 4) {
            (*c)++;
        }
    };
    auto* list = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_addLong(list, 1);
    celix_arrayList_addLong(list, 2);
    celix_arrayList_addLong(list, 3);
    celix_arrayList_addLong(list, 4);
    EXPECT_EQ(celix_arrayList_size(list), 4);
    celix_arrayList_clear(list); //will call removed callback for every entry
    EXPECT_EQ(count, 4);
    EXPECT_EQ(celix_arrayList_size(list), 0);

    celix_arrayList_destroy(list);

}

TEST_F(ArrayListTestSuite, SortForArrayListTest) {
    auto* list = celix_arrayList_create();
    celix_arrayList_addLong(list, 3);
    celix_arrayList_addLong(list, 2);
    celix_arrayList_addLong(list, 1);
    celix_arrayList_addLong(list, 4);
    EXPECT_EQ(celix_arrayList_getLong(list, 0), 3);
    EXPECT_EQ(celix_arrayList_getLong(list, 1), 2);
    EXPECT_EQ(celix_arrayList_getLong(list, 2), 1);
    EXPECT_EQ(celix_arrayList_getLong(list, 3), 4);


    celix_array_list_compare_entries_fp sort = [](celix_array_list_entry_t a, celix_array_list_entry_t b) -> int {
        return a.longVal - b.longVal;
    };
    celix_arrayList_sortEntries(list, sort);
    EXPECT_EQ(celix_arrayList_getLong(list, 0), 1);
    EXPECT_EQ(celix_arrayList_getLong(list, 1), 2);
    EXPECT_EQ(celix_arrayList_getLong(list, 2), 3);
    EXPECT_EQ(celix_arrayList_getLong(list, 3), 4);

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, ReturnStatusAddFunctionsTest) {
    auto* list = celix_arrayList_create();
    ASSERT_TRUE(list != nullptr);
    EXPECT_EQ(0, celix_arrayList_size(list));

    //no error, return status is CELIX_SUCCESS
    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addLong(list, 2L));
    EXPECT_EQ(1, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addDouble(list, 4.0));
    EXPECT_EQ(2, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addBool(list, true));
    EXPECT_EQ(3, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_add(list, (void*)0x42));
    EXPECT_EQ(4, celix_arrayList_size(list));

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, AutoCleanupTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    EXPECT_NE(nullptr, list);
}

TEST_F(ArrayListTestSuite, ReallocTest) {
    // Given a list with more than 10 elements (whitebox knowledge that the initial capacity is 10)
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    for (int i = 0; i < 20; ++i) {
        celix_status_t status = celix_arrayList_addLong(list, i);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }
    // Then the size is 20
    EXPECT_EQ(20, celix_arrayList_size(list));

    // And the elements are as expected
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(i, celix_arrayList_getLong(list, i));
    }
}

TEST_F(ArrayListTestSuite, ElementTypeToStringTest) {
    EXPECT_STREQ("Undefined", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_UNDEFINED));
    EXPECT_STREQ("Pointer", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_POINTER));
    EXPECT_STREQ("String", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING));
    EXPECT_STREQ("Long", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG));
    EXPECT_STREQ("Double", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE));
    EXPECT_STREQ("Bool", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL));
    EXPECT_STREQ("Version", celix_arrayList_elementTypeToString(CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION));
    EXPECT_STREQ("Undefined",
                 celix_arrayList_elementTypeToString((celix_array_list_element_type_t)100 /*non existing*/));
}
