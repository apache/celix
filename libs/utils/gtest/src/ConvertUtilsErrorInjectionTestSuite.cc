/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <gtest/gtest.h>

#include "celix_array_list_ei.h"
#include "celix_convert_utils.h"
#include "celix_version_ei.h"
#include "stdio_ei.h"
#include "celix_err.h"

class ConvertUtilsWithErrorInjectionTestSuite : public ::testing::Test {
public:
    ~ConvertUtilsWithErrorInjectionTestSuite() override {
        celix_ei_expect_celix_version_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_version_createVersionFromString(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createLongArray(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_addLong(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        celix_ei_expect_fputs(nullptr, 0, 0);
        celix_ei_expect_fputc(nullptr, 0, 0);

        celix_err_printErrors(stderr, nullptr, nullptr);
    }
};

TEST_F(ConvertUtilsWithErrorInjectionTestSuite, ConvertToVersionTest) {
    celix_version_t* defaultVersion = celix_version_create(1, 2, 3, "B");

    //Fail on first copy usage
    celix_ei_expect_celix_version_copy((void*)celix_utils_convertStringToVersion, 0, nullptr);
    celix_version_t* result;
    celix_status_t status = celix_utils_convertStringToVersion(nullptr, defaultVersion, &result);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(nullptr, result);

    //Fail on second copy usage
    celix_ei_expect_celix_version_copy((void*)celix_utils_convertStringToVersion, 0, nullptr);
    status = celix_utils_convertStringToVersion("invalid version str", defaultVersion, &result);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(nullptr, result);

    //Fail on parse version
    celix_ei_expect_celix_version_parse((void*)celix_utils_convertStringToVersion, 0, CELIX_ENOMEM);
    status = celix_utils_convertStringToVersion("1.2.3.B", defaultVersion, &result);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(nullptr, result);

    celix_version_destroy(defaultVersion);
}

TEST_F(ConvertUtilsWithErrorInjectionTestSuite, ConvertToLongArrayTest) {
    //Given an error injection for celix_arrayList_create
    celix_ei_expect_celix_arrayList_createLongArray((void*)celix_utils_convertStringToLongArrayList, 0, nullptr);
    //When calling celix_utils_convertStringToLongArrayList
    celix_array_list_t* result;
    celix_status_t status = celix_utils_convertStringToLongArrayList("1,2,3", nullptr, &result);
    //Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(nullptr, result);

    //Given an error injection for celix_arrayList_addLong
    celix_ei_expect_celix_arrayList_addLong((void*)celix_utils_convertStringToLongArrayList, 2, CELIX_ENOMEM);
    //When calling celix_utils_convertStringToLongArrayList
    status = celix_utils_convertStringToLongArrayList("1,2,3", nullptr, &result);
    //Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);

    celix_autoptr(celix_array_list_t) defaultList = celix_arrayList_create();

    //Given an error injection for celix_arrayList_copy
    celix_ei_expect_celix_arrayList_copy((void*)celix_utils_convertStringToLongArrayList, 1, nullptr);
    //When calling celix_utils_convertStringToLongArrayList with a nullptr as value
    status = celix_utils_convertStringToLongArrayList(nullptr, defaultList, &result);
    //Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);

    //Given an error injection for celix_arrayList_copy
    celix_ei_expect_celix_arrayList_copy((void*)celix_utils_convertStringToLongArrayList, 1, nullptr);
    //When calling celix_utils_convertStringToLongArrayList with an invalid value
    status = celix_utils_convertStringToLongArrayList("invalid", defaultList, &result);
    //Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);
}

TEST_F(ConvertUtilsWithErrorInjectionTestSuite, LongArrayToStringTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1L);
    celix_arrayList_addLong(list, 2L);

    //Given an error injection for opem_memstream
    celix_ei_expect_open_memstream((void*)celix_utils_arrayListToString, 1, nullptr);
    //When calling celix_utils_longArrayListToString
    char* result = celix_utils_arrayListToString(list);
    //Then the result is null
    EXPECT_EQ(nullptr, result);

    //Given an error injection for fputs
    celix_ei_expect_fputs((void*)celix_utils_arrayListToString, 1, -1);
    //When calling celix_utils_longArrayListToString
    result = celix_utils_arrayListToString(list);
    //Then the result is null
    EXPECT_EQ(nullptr, result);
}

TEST_F(ConvertUtilsWithErrorInjectionTestSuite, StringToStringArrayTest) {
    // Given an error injection for open_memstream (on the second call)
    celix_ei_expect_open_memstream((void*)celix_utils_convertStringToStringArrayList, 1, nullptr);
    // When calling celix_utils_convertStringToStringArrayList
    celix_array_list_t* result;
    celix_status_t status = celix_utils_convertStringToStringArrayList("a,b,c", nullptr, &result);
    // Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(nullptr, result);

    // Given an error injection for fputc
    celix_ei_expect_fputc((void*)celix_utils_convertStringToStringArrayList, 1, EOF);
    // When calling celix_utils_convertStringToStringArrayList
    status = celix_utils_convertStringToStringArrayList("a,b,c", nullptr, &result);
    // Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);

    // Given an error injection for fputc (on writing an escaped char)
    celix_ei_expect_fputc((void*)celix_utils_convertStringToStringArrayList, 1, EOF);
    // When calling celix_utils_convertStringToStringArrayList
    status = celix_utils_convertStringToStringArrayList(R"(\\)", nullptr, &result);
    // Then the result is null and the status is ENOMEM
    EXPECT_EQ(status, CELIX_ENOMEM);
}
