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

#include "celix/Properties.h"
#include "celix_cleanup.h"
#include "celix_err.h"
#include "celix_properties.h"
#include "celix_properties_private.h"
#include "celix_utils_private_constants.h"
#include "celix_version.h"

#include "celix_string_hash_map_ei.h"
#include "celix_utils_ei.h"
#include "malloc_ei.h"
#include "stdio_ei.h"
#include "celix_utils_ei.h"
#include "celix_version_ei.h"

class PropertiesErrorInjectionTestSuite : public ::testing::Test {
  public:
    PropertiesErrorInjectionTestSuite() = default;
    ~PropertiesErrorInjectionTestSuite() override {
        celix_err_resetErrors();
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_fopen(nullptr, 0, nullptr);
        celix_ei_expect_fputc(nullptr, 0, 0);
        celix_ei_expect_fseek(nullptr, 0, 0);
        celix_ei_expect_ftell(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
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
    celix_ei_expect_celix_stringHashMap_put((void*)celix_properties_set, 1, CELIX_ENOMEM);
    // Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

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

TEST_F(PropertiesErrorInjectionTestSuite, SetWithoutCopyFailureTest) {
    //Given a filled properties and a key and value
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    fillOptimizationCache(props);
    char* key = celix_utils_strdup("key");
    char* val = celix_utils_strdup("value");

    // When a malloc error injection is set for celix_properties_setWithoutCopy (during alloc entry)
    celix_ei_expect_malloc((void*)celix_properties_allocEntry, 0, nullptr);
    // Then the celix_properties_setWithoutCopy call fails
    auto status = celix_properties_setWithoutCopy(props, key, val);
    ASSERT_EQ(status, CELIX_ENOMEM);
    // And a celix err msg is set
    ASSERT_EQ(1, celix_err_getErrorCount());
    celix_err_resetErrors();

    //Given an allocated key and valu
    key = celix_utils_strdup("key");
    val = celix_utils_strdup("value");
    // When a celix_stringHashMap_put error injection is set for celix_properties_setWithoutCopy
    celix_ei_expect_celix_stringHashMap_put((void*)celix_properties_setWithoutCopy, 0, CELIX_ENOMEM);
    // Then the celix_properties_setWithoutCopy call fails
    status = celix_properties_setWithoutCopy(props, key, val);
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

TEST_F(PropertiesErrorInjectionTestSuite, LoadSetVersionFailureTest) {
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
}
