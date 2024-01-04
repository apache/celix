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
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"

class ArrayListTestSuite : public ::testing::Test {
public:
};

TEST_F(ArrayListTestSuite, TestCreateDestroyHashMap) {
    auto* list = celix_arrayList_create();
    EXPECT_TRUE(list != nullptr);
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, TestArrayListWithEquals) {
    celix_array_list_create_options_t opts{};
    opts.equalsCallback = [](celix_array_list_entry_t a, celix_array_list_entry_t b) -> bool {
        const char* sa = (char*)a.voidPtrVal;
        const char* sb = (char*)b.voidPtrVal;
        return celix_utils_stringEquals(sa, sb);
    };
    auto* list = celix_arrayList_createWithOptions(&opts);
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

    celix_arrayList_destroy(list);
}

template<typename T>
void testArrayListForTemplateType(int nrEntries) {
    auto* list = celix_arrayList_create();
    //fill
    for (int i = 0; i < nrEntries; ++i) {
        if constexpr (std::is_same_v<void*, T>) {
            celix_arrayList_add(list, (void*)(intptr_t )i);
        } else if constexpr (std::is_same_v<int, T>) {
            celix_arrayList_addInt(list, i);
        } else if constexpr (std::is_same_v<long, T>) {
            celix_arrayList_addLong(list, i);
        } else if constexpr (std::is_same_v<unsigned int, T>) {
            celix_arrayList_addUInt(list, i);
        } else if constexpr (std::is_same_v<unsigned long, T>) {
            celix_arrayList_addULong(list, i);
        } else if constexpr (std::is_same_v<float, T>) {
            celix_arrayList_addFloat(list, i + 0.0f);
        } else if constexpr (std::is_same_v<double, T>) {
            celix_arrayList_addDouble(list, i + 0.0);
        } else if constexpr (std::is_same_v<bool, T>) {
            celix_arrayList_addBool(list, i % 2 == 0);
        } else if constexpr (std::is_same_v<size_t, T>) {
            celix_arrayList_addSize(list, i);
        }
    }
    EXPECT_EQ(celix_arrayList_size(list), nrEntries);

    //get
    for (int i = 0; i < nrEntries; ++i) {
        if constexpr (std::is_same_v<void*, T>) {
            EXPECT_EQ(celix_arrayList_get(list, i), (void*)(intptr_t )i);
        } else if constexpr (std::is_same_v<int, T>) {
            EXPECT_EQ(celix_arrayList_getInt(list, i), i);
        } else if constexpr (std::is_same_v<long, T>) {
            EXPECT_EQ(celix_arrayList_getLong(list, i), i);
        } else if constexpr (std::is_same_v<unsigned int, T>) {
            EXPECT_EQ(celix_arrayList_getUInt(list, i), i);
        } else if constexpr (std::is_same_v<unsigned long, T>) {
            EXPECT_EQ(celix_arrayList_getULong(list, i), i);
        } else if constexpr (std::is_same_v<float, T>) {
            EXPECT_EQ(celix_arrayList_getFloat(list, i), i + 0.0);
        } else if constexpr (std::is_same_v<double, T>) {
            EXPECT_EQ(celix_arrayList_getDouble(list, i), i + 0.0);
        } else if constexpr (std::is_same_v<bool, T>) {
            EXPECT_EQ(celix_arrayList_getBool(list, i), i % 2 == 0);
        } else if constexpr (std::is_same_v<size_t, T>) {
            EXPECT_EQ(celix_arrayList_getSize(list, i), i);
        }
    }

    //remove
    for (int i = 0; i < nrEntries; ++i) {
        if constexpr (std::is_same_v<void*, T>) {
            celix_arrayList_remove(list, (void*)(intptr_t)i);
        } else if constexpr (std::is_same_v<int, T>) {
            celix_arrayList_removeInt(list, i);
        } else if constexpr (std::is_same_v<long, T>) {
            celix_arrayList_removeLong(list, i);
        } else if constexpr (std::is_same_v<unsigned int, T>) {
            celix_arrayList_removeUInt(list, i);
        } else if constexpr (std::is_same_v<unsigned long, T>) {
            celix_arrayList_removeULong(list, i);
        } else if constexpr (std::is_same_v<float, T>) {
            celix_arrayList_removeFloat(list, i + 0.0);
        } else if constexpr (std::is_same_v<double, T>) {
            celix_arrayList_removeDouble(list, i + 0.0);
        } else if constexpr (std::is_same_v<bool, T>) {
            celix_arrayList_removeBool(list, i % 2 == 0);
        } else if constexpr (std::is_same_v<size_t, T>) {
            celix_arrayList_removeSize(list, i);
        }
    }
    EXPECT_EQ(celix_arrayList_size(list), 0);

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, GetEntryTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    ASSERT_TRUE(list != nullptr);

    celix_arrayList_addInt(list, 1);
    celix_arrayList_addInt(list, 2);
    celix_arrayList_addInt(list, 3);

    celix_array_list_entry_t entry = celix_arrayList_getEntry(list, 0);
    EXPECT_EQ(entry.intVal, 1);
    entry = celix_arrayList_getEntry(list, 1);
    EXPECT_EQ(entry.intVal, 2);
    entry = celix_arrayList_getEntry(list, 2);
    EXPECT_EQ(entry.intVal, 3);
}

TEST_F(ArrayListTestSuite, CopyArrayList) {
    //Given an array list with a custom (and strange) equals function
    celix_array_list_create_options_t opts{};
    opts.equalsCallback = [](celix_array_list_entry_t a, celix_array_list_entry_t b) -> bool {
        //equal if both are even or both are odd, note only for testing
        return (a.intVal % 2 == 0 && b.intVal % 2 == 0) || (a.intVal % 2 != 0 && b.intVal % 2 != 0);
    };
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createWithOptions(&opts);
    ASSERT_TRUE(list != nullptr);

    //And 2 entries
    celix_arrayList_addInt(list, 1);
    celix_arrayList_addInt(list, 2);
    EXPECT_EQ(2, celix_arrayList_size(list));

    //When copying the array list
    celix_autoptr(celix_array_list_t) copy = celix_arrayList_copy(list);
    ASSERT_TRUE(copy != nullptr);

    //Then the copy should have the same size and entries
    EXPECT_EQ(celix_arrayList_size(copy), 2);
    EXPECT_EQ(celix_arrayList_getInt(copy, 0), 1);
    EXPECT_EQ(celix_arrayList_getInt(copy, 1), 2);

    //And the copy should have the same equals function
    EXPECT_EQ(2, celix_arrayList_size(copy));
    celix_arrayList_removeInt(copy, 3); //note in this test case 3 equals 1
    EXPECT_EQ(1, celix_arrayList_size(copy));
}

TEST_F(ArrayListTestSuite, TestDifferentEntyTypesForArrayList) {
    testArrayListForTemplateType<void*>(10);
    testArrayListForTemplateType<int>(10);
    testArrayListForTemplateType<long>(10);
    testArrayListForTemplateType<unsigned int>(10);
    testArrayListForTemplateType<unsigned long>(10);
    testArrayListForTemplateType<float>(10);
    testArrayListForTemplateType<double>(10);
    testArrayListForTemplateType<bool>(10);
    testArrayListForTemplateType<size_t>(10);
}

TEST_F(ArrayListTestSuite, TestSimpleRemovedCallbacksForArrayList) {
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

TEST_F(ArrayListTestSuite, TestRemovedCallbacksForArrayList) {
    int count = 0 ;
    celix_array_list_create_options_t opts{};
    opts.removedCallbackData = &count;
    opts.removedCallback = [](void *data, celix_array_list_entry_t entry) {
        int* c = (int*)data;
        if (entry.intVal == 1 || entry.intVal == 2 || entry.intVal == 3 || entry.intVal == 4) {
            (*c)++;
        }
    };
    auto* list = celix_arrayList_createWithOptions(&opts);
    celix_arrayList_addInt(list, 1);
    celix_arrayList_addInt(list, 2);
    celix_arrayList_addInt(list, 3);
    celix_arrayList_addInt(list, 4);
    EXPECT_EQ(celix_arrayList_size(list), 4);
    celix_arrayList_clear(list); //will call removed callback for every entry
    EXPECT_EQ(count, 4);
    EXPECT_EQ(celix_arrayList_size(list), 0);

    celix_arrayList_destroy(list);

}

TEST_F(ArrayListTestSuite, TestSortForArrayList) {
    auto* list = celix_arrayList_create();
    celix_arrayList_addInt(list, 3);
    celix_arrayList_addInt(list, 2);
    celix_arrayList_addInt(list, 1);
    celix_arrayList_addInt(list, 4);
    EXPECT_EQ(celix_arrayList_getInt(list, 0), 3);
    EXPECT_EQ(celix_arrayList_getInt(list, 1), 2);
    EXPECT_EQ(celix_arrayList_getInt(list, 2), 1);
    EXPECT_EQ(celix_arrayList_getInt(list, 3), 4);


    celix_array_list_sort_entries_fp sort = [](celix_array_list_entry_t a, celix_array_list_entry_t b) -> int {
        return a.intVal - b.intVal;
    };
    celix_arrayList_sortEntries(list, sort);
    EXPECT_EQ(celix_arrayList_getInt(list, 0), 1);
    EXPECT_EQ(celix_arrayList_getInt(list, 1), 2);
    EXPECT_EQ(celix_arrayList_getInt(list, 2), 3);
    EXPECT_EQ(celix_arrayList_getInt(list, 3), 4);

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, TestReturnStatusAddFunctions) {
    auto* list = celix_arrayList_create();
    ASSERT_TRUE(list != nullptr);
    EXPECT_EQ(0, celix_arrayList_size(list));

    //no error, return status is CELIX_SUCCESS
    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addInt(list, 1));
    EXPECT_EQ(1, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addLong(list, 2L));
    EXPECT_EQ(2, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addFloat(list, 3.0f));
    EXPECT_EQ(3, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addDouble(list, 4.0));
    EXPECT_EQ(4, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addBool(list, true));
    EXPECT_EQ(5, celix_arrayList_size(list));

    EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_add(list, (void*)0x42));
    EXPECT_EQ(6, celix_arrayList_size(list));

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListTestSuite, AutoCleanupTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_create();
    EXPECT_NE(nullptr, list);
}
