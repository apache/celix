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

#include "celix_cleanup.h"
#include "celix_properties.h"
#include "celix/Properties.h"

#include "malloc_ei.h"
#include "celix_hash_map_ei.h"
#include "celix_utils_ei.h"

class PropertiesErrorInjectionTestSuite : public ::testing::Test {
public:
    PropertiesErrorInjectionTestSuite() = default;
    ~PropertiesErrorInjectionTestSuite() override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
    }
};

TEST_F(PropertiesErrorInjectionTestSuite, CreateFailureTest) {
    //C API
    celix_ei_expect_malloc((void *)celix_properties_create, 0, nullptr);
    ASSERT_EQ(nullptr, celix_properties_create());

    //C++ API
    celix_ei_expect_malloc((void *)celix_properties_create, 0, nullptr);
    ASSERT_THROW(celix::Properties(), std::bad_alloc);
}

TEST_F(PropertiesErrorInjectionTestSuite, CopyFailureTest) {
    //C API
    celix_autoptr(celix_properties_t) prop = celix_properties_create();
    ASSERT_NE(nullptr, prop);
    celix_ei_expect_celix_stringHashMap_createWithOptions((void *)celix_properties_create, 0, nullptr);
    ASSERT_EQ(nullptr, celix_properties_copy(prop));

    //C++ API
    const celix::Properties cxxProp{};
    celix_ei_expect_celix_stringHashMap_createWithOptions((void *)celix_properties_create, 0, nullptr);
    ASSERT_THROW(celix::Properties{cxxProp}, std::bad_alloc);
}

//TODO store, load, setWithoutCopy

TEST_F(PropertiesErrorInjectionTestSuite, SetFailureTest) {
    //C API
    //Given a celix properties object with a filled optimization cache
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    for (int i = 0; i < 50; ++i) {
        const char* val = "1234567890";
        char key[10];
        snprintf(key, sizeof(key), "key%i", i);
        celix_properties_set(props, key, val);
    }

    //When a malloc error injection is set for celix_properties_set with level 2 (during alloc entry)
    celix_ei_expect_malloc((void *)celix_properties_set, 3, nullptr);
    //Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

    //When a celix_utils_strdup error injection is set for celix_properties_set with level 4 (during strdup key)
    celix_ei_expect_celix_utils_strdup((void *)celix_properties_set, 4, nullptr);
    //Then the celix_properties_set call fails
    ASSERT_EQ(celix_properties_set(props, "key", "value"), CELIX_ENOMEM);

    //C++ API
    //Given a celix properties object with a filled optimization cache
    celix::Properties cxxProps{};
    for (int i = 0; i < 50; ++i) {
        const char* val = "1234567890";
        char key[10];
        snprintf(key, sizeof(key), "key%i", i);
        cxxProps.set(key, val);
    }

    //When a malloc error injection is set for celix_properties_set with level 2 and ordinal 1 (during alloc entry)
    celix_ei_expect_malloc((void *)celix_properties_set, 3, nullptr, 1);
    //Then the Properties:set throws a bad_alloc exception
    ASSERT_THROW(cxxProps.set("key", "value"), std::bad_alloc);
}
