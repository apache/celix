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
#include "celix_err.h"
#include "celix_utils_ei.h"
#include "celix_version_ei.h"
#include "malloc_ei.h"

class ArrayListErrorInjectionTestSuite : public ::testing::Test {
public:
    ArrayListErrorInjectionTestSuite() = default;
    ~ArrayListErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_realloc(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_version_copy(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(ArrayListErrorInjectionTestSuite, CreateTest) {
    //Given an error is injected for calloc (used for the array struct)
    celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 1, nullptr);
    //Then creating an array list should fail
    EXPECT_EQ(nullptr, celix_arrayList_create());
    //And an error is logged to the celix_err
    EXPECT_EQ(1, celix_err_getErrorCount());

    //Given an error is injected for malloc (used for the element data)
    celix_ei_expect_malloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    //Then creating an array list should fail
    EXPECT_EQ(nullptr, celix_arrayList_create());
    //And an error is logged to the celix_err
    EXPECT_EQ(2, celix_err_getErrorCount());
}


TEST_F(ArrayListErrorInjectionTestSuite, AddFunctionsTest) {
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
    //And an error is logged to the celix_err
    EXPECT_EQ(1, celix_err_getErrorCount());

    celix_arrayList_destroy(list);
}

TEST_F(ArrayListErrorInjectionTestSuite, AddStringAndAddVersionFailureTest) {
    // Given a string array list
    celix_autoptr(celix_array_list_t) stringList = celix_arrayList_createStringArray();
    // When an error is injected for celix_utils_strdup
    celix_ei_expect_celix_utils_strdup((void*)celix_arrayList_addString, 0, nullptr);
    // Then adding a string should fail
    EXPECT_EQ(CELIX_ENOMEM, celix_arrayList_addString(stringList, "test"));

    // Given a version array list
    celix_autoptr(celix_array_list_t) versionList = celix_arrayList_createVersionArray();
    // When an error is injected for celix_version_copy
    celix_ei_expect_celix_version_copy((void*)celix_arrayList_addVersion, 0, nullptr);
    // Then adding a version should fail
    celix_autoptr(celix_version_t) version = celix_version_createVersionFromString("1.0.0");
    EXPECT_EQ(CELIX_ENOMEM, celix_arrayList_addVersion(versionList, version));
}

TEST_F(ArrayListErrorInjectionTestSuite, CopyArrayListFailureTest) {
    // Given a string array list with 10 elements (whitebox knowledge)
    celix_autoptr(celix_array_list_t) stringList = celix_arrayList_createStringArray();
    celix_arrayList_addString(stringList, "test1");

    // When an error is injected for calloc
    celix_ei_expect_calloc((void*)celix_arrayList_copy, 1, nullptr);
    // Then copying an array list should fail
    EXPECT_EQ(nullptr, celix_arrayList_copy(stringList));
    // And a celix_err is expected
    EXPECT_EQ(1, celix_err_getErrorCount());

    // When an error is injected for celix_utils_strdup
    celix_ei_expect_celix_utils_strdup((void*)celix_arrayList_addString, 0, nullptr);
    // Then copying an array list should fail
    EXPECT_EQ(nullptr, celix_arrayList_copy(stringList));
    // And a celix_err is expected
    EXPECT_EQ(2, celix_err_getErrorCount());
}
