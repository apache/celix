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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#include <gtest/gtest.h>
#include <string>

#include "celix/Properties.h"
#include "celix_cleanup.h"
#include "celix_convert_utils.h"
#include "celix_err.h"
#include "celix_properties.h"
#include "celix_properties_private.h"
#include "celix_utils_private_constants.h"
#include "celix_version.h"

#include "asprintf_ei.h"
#include "celix_array_list_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_utils_ei.h"
#include "celix_version_ei.h"
#include "malloc_ei.h"
#include "stdio_ei.h"

class PropertiesErrorInjectionTestSuite : public ::testing::Test {
  public:
    PropertiesErrorInjectionTestSuite() = default;
    ~PropertiesErrorInjectionTestSuite() override {
        celix_err_resetErrors();
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, -1);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_fopen(nullptr, 0, nullptr);
        celix_ei_expect_fputc(nullptr, 0, 0);
        celix_ei_expect_fseek(nullptr, 0, 0);
        celix_ei_expect_ftell(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_version_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_copy(nullptr, 0, nullptr);
    }

    /**
     * Fills the optimization cache of the given properties object to ensure the next added entries will need
     * allocation.
     */
    void fillOptimizationCache(celix_properties_t* props) {
        int index = 0;
        size_t written = 0;
        size_t nrOfEntries = 0;
        while (written < CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE ||
               nrOfEntries++ < CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE) {
            const char* val = "1234567890";
            char key[10];
            snprintf(key, sizeof(key), "key%i", index++);
            written += strlen(key) + strlen(val) + 2;
            celix_properties_set(props, key, val);
        }
    }
};

TEST_F(PropertiesErrorInjectionTestSuite, CreateFailureTest) {
    // C API
    celix_ei_expect_malloc((void*)celix_properties_create, 0, nullptr);
    ASSERT_EQ(nullptr, celix_properties_create());

    // C++ API
    celix_ei_expect_malloc((void*)celix_properties_create, 0, nullptr);
    ASSERT_THROW(celix::Properties(), std::bad_alloc);
}

TEST_F(PropertiesErrorInjectionTestSuite, CopyFailureTest) {
    // C API

    //Given a celix properties object with more entries than the optimization cache
    celix_autoptr(celix_properties_t) prop = celix_properties_create();
    ASSERT_NE(nullptr, prop);
    fillOptimizationCache(prop);
    celix_properties_set(prop, "additionalKey", "value");

    // When a hash map create error injection is set for celix_properties_create
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)celix_properties_create, 0, nullptr);
    // Then the celix_properties_copy call fails
    ASSERT_EQ(nullptr, celix_properties_copy(prop));
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a malloc error injection is set for celix_properties_allocEntry (during set)
    celix_ei_expect_malloc((void*)celix_properties_allocEntry, 0, nullptr);
    // Then the celix_properties_copy call fails
    ASSERT_EQ(nullptr, celix_properties_copy(prop));
    ASSERT_GE(celix_err_getErrorCount(), 1);
    celix_err_resetErrors();

    // C++ API
    const celix::Properties cxxProp{};
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)celix_properties_create, 0, nullptr);
    ASSERT_THROW(celix::Properties{cxxProp}, std::bad_alloc);
}

TEST_F(PropertiesErrorInjectionTestSuite, SetFailureTest) {
    // C API
    // Given a celix properties object with a filled optimization cache
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    fillOptimizationCache(props);

    // When a malloc error injection is set for celix_properties_allocEntry (during set)
    celix_ei_expect_malloc((void*)celix_properties_allocEntry, 0, nullptr);
    // Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

    // When a celix_utils_strdup error injection is set for celix_properties_set (during strdup key)
    celix_ei_expect_celix_utils_strdup((void*)celix_properties_createString, 0, nullptr);
    // Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

    // When a celix_stringHashMap_put error injection is set for celix_properties_set with level 1 (during put)
    celix_ei_expect_celix_stringHashMap_put((void*)celix_properties_set, 2, CELIX_ENOMEM);
    // Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

    // When a celix_utils_strdup error injection is when calling celix_properties_set, for the creation of the
    // key string.
    celix_ei_expect_celix_utils_strdup((void*)celix_properties_createString, 0, nullptr, 2);
    // Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

    celix_ei_expect_celix_utils_strdup((void*)celix_properties_createString, 0, nullptr);
    ASSERT_EQ(CELIX_ENOMEM, celix_properties_setLong(props, "key", 1000));

    celix_ei_expect_celix_utils_strdup((void*)celix_properties_createString, 0, nullptr);
    ASSERT_EQ(CELIX_ENOMEM, celix_properties_setDouble(props, "key", 1.2));

    double largeNumber = 123456789012345.0;
    celix_ei_expect_asprintf((void*)celix_properties_setDouble, 3, -1);
    ASSERT_EQ(CELIX_ENOMEM, celix_properties_setDouble(props, "key", largeNumber));

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_ei_expect_open_memstream((void*)celix_utils_arrayListToString, 1, nullptr);
    ASSERT_EQ(CELIX_ENOMEM, celix_properties_setArrayList(props, "key", list));

    // C++ API
    // Given a celix properties object with a filled optimization cache
    celix::Properties cxxProps{};
    for (int i = 0; i < 50; ++i) {
        const char* val = "1234567890";
        char key[10];
        snprintf(key, sizeof(key), "key%i", i);
        cxxProps.set(key, val);
    }

    // When a malloc error injection is set for celix_properties_set (during alloc entry)
    celix_ei_expect_malloc((void*)celix_properties_allocEntry, 0, nullptr);
    // Then the Properties:set throws a bad_alloc exception
    ASSERT_THROW(cxxProps.set("key", "value"), std::bad_alloc);
}

TEST_F(PropertiesErrorInjectionTestSuite, StoreFailureTest) {
    // C API
    // Given a celix properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key", "value");

    // When a fopen error injection is set for celix_properties_store (during fopen)
    celix_ei_expect_fopen((void*)celix_properties_store, 0, nullptr);
    // Then the celix_properties_store call fails
    auto status = celix_properties_store(props, "file", nullptr);
    ASSERT_EQ(status, CELIX_FILE_IO_EXCEPTION);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a fputc error injection is set for celix_properties_store (during fputc)
    celix_ei_expect_fputc((void*)celix_properties_store, 0, EOF);
    // Then the celix_properties_store call fails
    status = celix_properties_store(props, "file", nullptr);
    ASSERT_EQ(status, CELIX_FILE_IO_EXCEPTION);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // C++ API
    // Given a C++ celix properties object.
    auto cxxProps = celix::Properties{};
    cxxProps.set("key", "value");

    // When a fopen error injection is set for celix_properties_store (during fopen)
    celix_ei_expect_fopen((void*)celix_properties_store, 0, nullptr);
    // Then the Properties:store throws a celix::IOException exception
    EXPECT_THROW(cxxProps.store("file"), celix::IOException);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();
}

TEST_F(PropertiesErrorInjectionTestSuite, LoadFailureTest) {
    // C API
    // Given a fmemstream buffer with a properties file
    const char* content = "key=value\n";
    auto* memStream = fmemopen((void*)content, strlen(content), "r");

    // When a fopen error injection is set for celix_properties_load (during fopen)
    celix_ei_expect_fopen((void*)celix_properties_load, 0, nullptr);
    // Then the celix_properties_load call fails
    auto props = celix_properties_load("file");
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a malloc error injection is set for celix_properties_loadWithStream (during properties create)
    celix_ei_expect_malloc((void*)celix_properties_create, 0, nullptr);
    // Then the celix_properties_loadWithStream call fails
    props = celix_properties_loadWithStream(memStream);
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a fseek error injection is set for celix_properties_loadWithStream
    celix_ei_expect_fseek((void*)celix_properties_loadWithStream, 0, -1);
    // Then the celix_properties_loadWithStream call fails
    props = celix_properties_loadWithStream(memStream);
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a fseek error injection is set for celix_properties_loadWithStream, ordinal 2
    celix_ei_expect_fseek((void*)celix_properties_loadWithStream, 0, -1, 2);
    // Then the celix_properties_loadWithStream call fails
    props = celix_properties_loadWithStream(memStream);
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a ftell error injection is set for celix_properties_loadWithStream
    celix_ei_expect_ftell((void*)celix_properties_loadWithStream, 0, -1);
    // Then the celix_properties_loadWithStream call fails
    props = celix_properties_loadWithStream(memStream);
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    // When a malloc error injection is set for celix_properties_loadWithStream
    celix_ei_expect_malloc((void*)celix_properties_loadWithStream, 0, nullptr);
    // Then the celix_properties_loadWithStream call fails
    props = celix_properties_loadWithStream(memStream);
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    //C++ API
    // When a fopen error injection is set for celix_properties_load (during fopen)
    celix_ei_expect_fopen((void*)celix_properties_load, 0, nullptr);
    // Then the celix::Properties::load call throws a celix::IOException exception
    EXPECT_THROW(celix::Properties::load("file"), celix::IOException);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    fclose(memStream);
}

TEST_F(PropertiesErrorInjectionTestSuite, GetAsVersionWithVersionCopyFailedTest) {
    //Given a properties set with a version
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_version_t* v = celix_version_create(1, 2, 3, "qualifier");
    celix_properties_assignVersion(props, "key", v);

    // When a celix_version_copy error injection is set for celix_properties_getAsVersion
    celix_ei_expect_celix_version_copy((void*)celix_properties_getAsVersion, 0, nullptr);

    // Then the celix_properties_getAsVersion call fails
    celix_version_t* version = nullptr;
    auto status =  celix_properties_getAsVersion(props, "key", nullptr, &version);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, version);
}

TEST_F(PropertiesErrorInjectionTestSuite, GetAsArrayWithArrayListCopyFailedTest) {
    //Given a properties set with a string array, long array, double array, bool array and version array
    const char* str1 = "string1";
    const char* str2 = "string2";
    celix_autoptr(celix_array_list_t) stringList = celix_arrayList_createStringArray();
    celix_arrayList_addString(stringList, str1);
    celix_arrayList_addString(stringList, str2);
    celix_autoptr(celix_array_list_t) longList = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longList, 1);
    celix_arrayList_addLong(longList, 2);
    celix_autoptr(celix_array_list_t) doubleList = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleList, 1.1);
    celix_arrayList_addDouble(doubleList, 2.2);
    celix_autoptr(celix_array_list_t) boolList = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolList, true);
    celix_arrayList_addBool(boolList, false);
    celix_autoptr(celix_version_t) v = celix_version_create(1, 2, 3, "qualifier");
    celix_autoptr(celix_array_list_t) versionList = celix_arrayList_createVersionArray();
    celix_arrayList_addVersion(versionList, v);
    celix_arrayList_addVersion(versionList, v);

    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setArrayList(props, "stringArray", stringList);
    celix_properties_setArrayList(props, "longArray", longList);
    celix_properties_setArrayList(props, "doubleArray", doubleList);
    celix_properties_setArrayList(props, "boolArray", boolList);
    celix_properties_setArrayList(props, "versionArray", versionList);
    celix_properties_setString(props, "string", "Hello world");

    // When a celix_arrayList_createWithOptions error injection is set for celix_properties_getAsStringArrayList
    celix_ei_expect_celix_arrayList_copy((void*)celix_properties_getAsStringArrayList, 1, nullptr);

    // Then the celix_properties_getAsStringArrayList call fails
    celix_array_list_t* strings = nullptr;
    auto status = celix_properties_getAsStringArrayList(props, "stringArray", nullptr, &strings);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, strings);

    //When a celix_arrayList_copy error injection is set for celix_properties_getAsLongArrayList
    celix_ei_expect_celix_arrayList_copy((void*)celix_properties_getAsLongArrayList, 1, nullptr);

    // Then the celix_properties_getAsLongArrayList call fails
    celix_array_list_t* longs = nullptr;
    status = celix_properties_getAsLongArrayList(props, "longArray", nullptr, &longs);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, longs);

    //When a celix_arrayList_copy error injection is set for celix_properties_getAsDoubleArrayList
    celix_ei_expect_celix_arrayList_copy((void*)celix_properties_getAsDoubleArrayList, 1, nullptr);

    // Then the celix_properties_getAsDoubleArrayList call fails
    celix_array_list_t* doubles = nullptr;
    status = celix_properties_getAsDoubleArrayList(props, "doubleArray", nullptr, &doubles);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, doubles);

    //When a celix_arrayList_copy error injection is set for celix_properties_getAsBoolArrayList
    celix_ei_expect_celix_arrayList_copy((void*)celix_properties_getAsBoolArrayList, 1, nullptr);

    // Then the celix_properties_getAsBoolArrayList call fails
    celix_array_list_t* bools = nullptr;
    status = celix_properties_getAsBoolArrayList(props, "boolArray", nullptr, &bools);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, bools);

    //When a celix_arrayList_createWithOptions error injection is set for celix_properties_getAsVersionArrayList
    celix_ei_expect_celix_arrayList_copy((void*)celix_properties_getAsVersionArrayList, 1, nullptr);

    // Then the celix_properties_getAsVersionArrayList call fails
    celix_array_list_t* versions = nullptr;
    status = celix_properties_getAsVersionArrayList(props, "versionArray", nullptr, &versions);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, versions);


    celix_ei_expect_open_memstream((void*)celix_utils_convertStringToVersionArrayList, 1, nullptr);
    versions = nullptr;
    status = celix_properties_getAsVersionArrayList(props, "string", nullptr, &versions);
    ASSERT_EQ(status, CELIX_ENOMEM);
    ASSERT_EQ(nullptr, versions);
}

TEST_F(PropertiesErrorInjectionTestSuite, SetArrayWithArrayListCopyFailedTest) {
    //Given a properties set
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    //And a (empty) array list
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();

    // When a celix_arrayList_copy error injection is set for celix_properties_setArrayList
    celix_ei_expect_celix_arrayList_copy((void*)celix_properties_setArrayList, 0, nullptr);

    // Then the celix_properties_setArrayList call fails
    EXPECT_EQ(CELIX_ENOMEM, celix_properties_setArrayList(props, "longArray", list));
}


TEST_F(PropertiesErrorInjectionTestSuite, AssignFailureTest) {
    //Given a filled properties and a key and value
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    fillOptimizationCache(props);
    char* key = celix_utils_strdup("key");
    char* val = celix_utils_strdup("value");

    // When a malloc error injection is set for celix_properties_setWithoutCopy (during alloc entry)
    celix_ei_expect_malloc((void*)celix_properties_allocEntry, 0, nullptr);
    // Then the celix_properties_setWithoutCopy call fails
    auto status = celix_properties_assign(props, key, val);
    ASSERT_EQ(status, CELIX_ENOMEM);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    //Given an allocated key and valu
    key = celix_utils_strdup("key");
    val = celix_utils_strdup("value");
    // When a celix_stringHashMap_put error injection is set for celix_properties_setWithoutCopy
    celix_ei_expect_celix_stringHashMap_put((void*)celix_properties_assign, 0, CELIX_ENOMEM);
    // Then the celix_properties_setWithoutCopy call fails
    status = celix_properties_assign(props, key, val);
    ASSERT_EQ(status, CELIX_ENOMEM);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();
}

TEST_F(PropertiesErrorInjectionTestSuite, LoadFromStringFailureTest) {
    // When a strdup error injection is set for celix_properties_loadFromString (during strdup)
    celix_ei_expect_celix_utils_strdup((void*)celix_properties_loadFromString, 0, nullptr);
    // Then the celix_properties_loadFromString call fails
    auto props = celix_properties_loadFromString("key=value");
    ASSERT_EQ(nullptr, props);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();
}

TEST_F(PropertiesErrorInjectionTestSuite, SetVersionFailureTest) {
    // Given a celix properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    // And a version object
    celix_autoptr(celix_version_t) version = celix_version_create(1, 2, 3, "qualifier");
    // When a celix_version_copy error injection is set for celix_properties_setVersion
    celix_ei_expect_celix_version_copy((void*)celix_properties_setVersion, 0, nullptr);
    // Then the celix_properties_setVersion call fails
    auto status = celix_properties_setVersion(props, "key", version);
    ASSERT_EQ(status, CELIX_ENOMEM);
    //And a celix err msg is pushed
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    celix_autoptr(celix_version_t) version2 = celix_version_create(1, 2, 3, "aaaaaaaaaaaaaaaaaaaaaaaaaa");
    celix_ei_expect_calloc((void*) celix_version_create, 0, nullptr);
    status = celix_properties_setVersion(props, "key", version2);
    ASSERT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(2, celix_err_getErrorCount());
    ASSERT_STREQ("Failed to copy version", celix_err_popLastError());
    ASSERT_STREQ("Failed to allocate memory for celix_version_create", celix_err_popLastError());
    celix_err_resetErrors();

    fillOptimizationCache(props);
    celix_ei_expect_celix_utils_strdup((void*)celix_properties_createString, 0, nullptr);
    status = celix_properties_setVersion(props, "key1", version);
    ASSERT_EQ(status, CELIX_ENOMEM);
    //And a celix err msg is pushed
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();
}
