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

#include "celix_array_list_encoding.h"
#include "celix_err.h"
#include "malloc_ei.h"
#include "celix_array_list_ei.h"
#include "stdio_ei.h"
#include "jansson_ei.h"

class CelixArrayListEncodingErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixArrayListEncodingErrorInjectionTestSuite() = default;
    ~CelixArrayListEncodingErrorInjectionTestSuite() override {
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_addLong(nullptr, 0, 0);
        celix_ei_expect_fmemopen(nullptr, 0, nullptr);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        celix_ei_expect_json_dumpf(nullptr, 0, 0);
        celix_ei_expect_fopen(nullptr, 0, nullptr);
        celix_ei_expect_fclose(nullptr, 0, 0);
        celix_ei_expect_json_array(nullptr, 0, nullptr);
        celix_err_resetErrors();
    }
};

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, CreateArrayListErrorTest) {
    celix_ei_expect_celix_arrayList_createWithOptions((void*)&celix_arrayList_loadFromString, 2, nullptr);

    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([1, 2, 3])", 0, &list);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, ArrayListAddElementErrorTest) {
    celix_ei_expect_celix_arrayList_addLong((void*)&celix_arrayList_loadFromString, 2, ENOMEM);

    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([1, 2, 3])", 0, &list);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, FmemopenErrorWhenLoadArrayListtFromStringTest) {
    celix_ei_expect_fmemopen((void*)&celix_arrayList_loadFromString, 0, nullptr);

    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([1, 2, 3])", 0, &list);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, OpenmemstreamErrorWhenSaveArrayListToStringTest) {
    celix_ei_expect_open_memstream((void*)&celix_arrayList_saveToString, 0, nullptr);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    char* out = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &out);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, JsondumpfErrorWhenSaveArrayListToStringTest) {
    celix_ei_expect_json_dumpf((void*)&celix_arrayList_saveToStream, 0, -1);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    char* out = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &out);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, FopenErrorWhenSaveArrayListToFileTest) {
    celix_ei_expect_fopen((void*)&celix_arrayList_save, 0, nullptr);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    const char* filename = "/tmp/array_list_file.txt";
    auto status = celix_arrayList_save(list, 0, filename);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, FcloseErrorWhenSaveArrayListToFileTest) {
    celix_ei_expect_fclose((void*)&celix_arrayList_save, 0, -1);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    const char* filename = "/tmp/array_list_file.txt";
    auto status = celix_arrayList_save(list, 0, filename);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, status);

    remove(filename);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, CreateJsonArrayErrorWhenSaveArrayListTest) {
    celix_ei_expect_json_array((void*)&celix_arrayList_saveToString, 2, nullptr);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    char* out = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &out);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, CreateJsonIntegerErrorWhenSaveArrayListTest) {
    celix_ei_expect_json_integer((void*)&celix_arrayList_saveToString, 3, nullptr);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    char* out = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &out);
    ASSERT_EQ(ENOMEM, status);
}

TEST_F(CelixArrayListEncodingErrorInjectionTestSuite, JsonArrayAppendNewErrorWhenSaveArrayListTest) {
    celix_ei_expect_json_array_append_new((void*)&celix_arrayList_saveToString, 2, -1);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    char* out = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &out);
    ASSERT_EQ(ENOMEM, status);
}