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

#include <random>

#include "celix_err.h"
#include "celix_hash_map_private.h"
#include "celix_long_hash_map.h"
#include "celix_string_hash_map.h"

#include "malloc_ei.h"

class HashMapErrorInjectionTestSuite : public ::testing::Test {
  public:
    HashMapErrorInjectionTestSuite() {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(HashMapErrorInjectionTestSuite, CreateFailureTest) {
    // When a calloc error injection is set for celix_hashMap_init
    celix_ei_expect_calloc((void*)celix_hashMap_init, 0, nullptr);
    // Then celix_stringHashMap_create will return nullptr
    auto* sProps = celix_stringHashMap_create();
    ASSERT_EQ(nullptr, sProps);

    // When a calloc error injection is set for celix_hashMap_init
    celix_ei_expect_calloc((void*)celix_hashMap_init, 0, nullptr);
    // Then celix_stringHashMap_create will return nullptr
    sProps = celix_stringHashMap_create();
    ASSERT_EQ(nullptr, sProps);

    // When a calloc error injection is set for celix_hashMap_init
    celix_ei_expect_calloc((void*)celix_hashMap_init, 0, nullptr);
    // Then celix_longHashMap_create will return nullptr
    auto* lProps = celix_longHashMap_create();
    ASSERT_EQ(nullptr, lProps);

    // When a calloc error injection is set for celix_hashMap_init
    celix_ei_expect_calloc((void*)celix_hashMap_init, 0, nullptr);
    // Then celix_longHashMap_create will return nullptr
    lProps = celix_longHashMap_create();
    ASSERT_EQ(nullptr, lProps);

    EXPECT_EQ(celix_err_getErrorCount(), 4); // 4x calloc error
    celix_err_resetErrors();
}

TEST_F(HashMapErrorInjectionTestSuite, PutFailureTest) {
    // Given a celix_string_hash_map_t
    celix_autoptr(celix_string_hash_map_t) sProps = celix_stringHashMap_create();
    ASSERT_NE(nullptr, sProps);

    // When a malloc error injection is set for celix_hashMap_addEntry
    celix_ei_expect_malloc((void*)celix_hashMap_addEntry, 0, nullptr);
    // Then celix_stringHashMap_putLong will return CELIX_ENOMEM
    auto status = celix_stringHashMap_putLong(sProps, "key", 1L);
    ASSERT_EQ(CELIX_ENOMEM, status);

    // Given a celix_long_hash_map_t
    celix_autoptr(celix_long_hash_map_t) lProps = celix_longHashMap_create();
    ASSERT_NE(nullptr, lProps);

    // When a malloc error injection is set for celix_hashMap_addEntry
    celix_ei_expect_malloc((void*)celix_hashMap_addEntry, 0, nullptr);
    // Then celix_stringHashMap_putLong will return CELIX_ENOMEM
    status = celix_longHashMap_putLong(lProps, 1, 1L);
    ASSERT_EQ(CELIX_ENOMEM, status);

    EXPECT_EQ(celix_err_getErrorCount(), 2); // 2x malloc error
    celix_err_resetErrors();
}

TEST_F(HashMapErrorInjectionTestSuite, ResizeFailureTest) {
    // Given a hashmap with a low load factor
    celix_long_hash_map_create_options_t opts{};
    opts.maxLoadFactor = 0.1;
    celix_autoptr(celix_long_hash_map_t) lProps = celix_longHashMap_createWithOptions(&opts);

    // And when the hash map is filled 1 entry before the resize threshold
    celix_longHashMap_putLong(lProps, 0, 0);

    // When a realloc error injection is set for celix_hashMap_resize
    celix_ei_expect_realloc((void*)celix_hashMap_resize, 0, nullptr);
    // Then celix_stringHashMap_putLong will return CELIX_ENOMEM
    auto status = celix_longHashMap_putLong(lProps, 1, 1L);
    ASSERT_EQ(CELIX_ENOMEM, status);

    EXPECT_EQ(celix_err_getErrorCount(), 1); // 2x realloc error
    celix_err_resetErrors();
}
