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
#include "celix_properties.h"

#include "malloc_ei.h"
#include "celix_hash_map_ei.h"

#include <gtest/gtest.h>

class PropertiesErrorInjectionTestSuite : public ::testing::Test {
public:
    PropertiesErrorInjectionTestSuite() = default;
    ~PropertiesErrorInjectionTestSuite() override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
    }
};

TEST_F(PropertiesErrorInjectionTestSuite, CreateFailureTest) {
    celix_ei_expect_malloc((void *)celix_properties_create, 0, nullptr);
    ASSERT_EQ(nullptr, celix_properties_create());
}

TEST_F(PropertiesErrorInjectionTestSuite, CopyFailureTest) {
    celix_autoptr(celix_properties_t) prop = celix_properties_create();
    ASSERT_NE(nullptr, prop);
    celix_ei_expect_celix_stringHashMap_createWithOptions((void *)celix_properties_create, 0, nullptr);
    ASSERT_EQ(nullptr, celix_properties_copy(prop));
}
