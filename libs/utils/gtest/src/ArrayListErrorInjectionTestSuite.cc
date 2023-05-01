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
#include "malloc_ei.h"

class ArrayListErrorInjectionTestSuite : public ::testing::Test {
public:
    ArrayListErrorInjectionTestSuite() = default;
    ~ArrayListErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_realloc(nullptr, 0, nullptr);
    }
};

TEST_F(ArrayListErrorInjectionTestSuite, TestAddFunctions) {
    //Given an array list with a capacity of 10 (whitebox knowledge)
    auto* list = celix_arrayList_create();

    //When adding 10 elements, no error is expected
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(CELIX_SUCCESS, celix_arrayList_addInt(list, i));
    }
    EXPECT_EQ(10, celix_arrayList_size(list));

    //And realloc is primed to fail
    celix_ei_expect_realloc(CELIX_EI_UNKNOWN_CALLER, 1, nullptr);

    //Then adding an element should fail
    EXPECT_EQ(CELIX_ENOMEM, celix_arrayList_addInt(list, 10));
    EXPECT_EQ(10, celix_arrayList_size(list));
}
