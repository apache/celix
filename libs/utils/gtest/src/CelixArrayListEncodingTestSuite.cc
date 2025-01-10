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
#include <cmath>

#include "celix_array_list_encoding.h"
#include "celix_version.h"
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"
#include "celix_stdio_cleanup.h"

class CelixArrayListEncodingTestSuite : public ::testing::Test {
public:
    CelixArrayListEncodingTestSuite() { celix_err_resetErrors(); }
};

TEST_F(CelixArrayListEncodingTestSuite, LoadStringArrayListTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_loadFromString(R"(["str1", "str2", "str3"])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(list != nullptr);
    ASSERT_EQ(3, celix_arrayList_size(list));

    auto str1 = celix_arrayList_getString(list, 0);
    EXPECT_STREQ("str1", str1);
    auto str2 = celix_arrayList_getString(list, 1);
    EXPECT_STREQ("str2", str2);
    auto str3 = celix_arrayList_getString(list, 2);
    EXPECT_STREQ("str3", str3);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadLongArrayListTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([1, 2, 3])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(list != nullptr);
    ASSERT_EQ(3, celix_arrayList_size(list));

    auto l1 = celix_arrayList_getLong(list, 0);
    EXPECT_EQ(1, l1);
    auto l2 = celix_arrayList_getLong(list, 1);
    EXPECT_EQ(2, l2);
    auto l3 = celix_arrayList_getLong(list, 2);
    EXPECT_EQ(3, l3);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadDoubleArrayListTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([1.1, 2.2, 3.3])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(list != nullptr);
    ASSERT_EQ(3, celix_arrayList_size(list));

    auto d1 = celix_arrayList_getDouble(list, 0);
    EXPECT_DOUBLE_EQ(1.1, d1);
    auto d2 = celix_arrayList_getDouble(list, 1);
    EXPECT_DOUBLE_EQ(2.2, d2);
    auto d3 = celix_arrayList_getDouble(list, 2);
    EXPECT_DOUBLE_EQ(3.3, d3);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadBoolArrayListTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([true, false, true])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(list != nullptr);
    ASSERT_EQ(3, celix_arrayList_size(list));

    auto b1 = celix_arrayList_getBool(list, 0);
    EXPECT_TRUE(b1);
    auto b2 = celix_arrayList_getBool(list, 1);
    EXPECT_FALSE(b2);
    auto b3 = celix_arrayList_getBool(list, 2);
    EXPECT_TRUE(b3);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadVersionArrayListTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_loadFromString(R"(["version<1.2.3>", "version<2.3.4>", "version<3.4.5>"])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(list != nullptr);
    ASSERT_EQ(3, celix_arrayList_size(list));

    auto v1 = celix_arrayList_getVersion(list, 0);
    EXPECT_EQ(1, celix_version_getMajor(v1));
    EXPECT_EQ(2, celix_version_getMinor(v1));
    EXPECT_EQ(3, celix_version_getMicro(v1));
    auto v2 = celix_arrayList_getVersion(list, 1);
    EXPECT_EQ(2, celix_version_getMajor(v2));
    EXPECT_EQ(3, celix_version_getMinor(v2));
    EXPECT_EQ(4, celix_version_getMicro(v2));
    auto v3 = celix_arrayList_getVersion(list, 2);
    EXPECT_EQ(3, celix_version_getMajor(v3));
    EXPECT_EQ(4, celix_version_getMinor(v3));
    EXPECT_EQ(5, celix_version_getMicro(v3));
}

TEST_F(CelixArrayListEncodingTestSuite, LoadArrayListFromFileTest) {
    const char *input = R"(["str1", "str2", "str3"])";
    const char *filename = "/tmp/celix_array_list_encoding_test.txt";
    auto stream = fopen(filename, "w");
    ASSERT_TRUE(stream != nullptr);
    auto size = fwrite(input, 1, strlen(input), stream);
    EXPECT_EQ(strlen(input), size);
    fclose(stream);

    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_load(filename, 0, &list);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(list != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(list));
    auto str1 = celix_arrayList_getString(list, 0);
    EXPECT_STREQ("str1", str1);
    auto str2 = celix_arrayList_getString(list, 1);
    EXPECT_STREQ("str2", str2);
    auto str3 = celix_arrayList_getString(list, 2);
    EXPECT_STREQ("str3", str3);

    remove(filename);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveStringArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createStringArray();
    celix_arrayList_addString(list, "str1");
    celix_arrayList_addString(list, "str2");
    celix_arrayList_addString(list, "str3");

    celix_autofree char* output = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_STREQ(R"(["str1","str2","str3"])", output);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveLongArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    celix_arrayList_addLong(list, 2);
    celix_arrayList_addLong(list, 3);

    celix_autofree char* output = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_STREQ("[1,2,3]", output);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveDoubleArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(list, 1.1);
    celix_arrayList_addDouble(list, 2.2);
    celix_arrayList_addDouble(list, 3.3);

    celix_autofree char* output = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_TRUE(strstr(output, "1.") != nullptr);
    ASSERT_TRUE(strstr(output, "2.") != nullptr);
    ASSERT_TRUE(strstr(output, "3.") != nullptr);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveBoolArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(list, true);
    celix_arrayList_addBool(list, false);
    celix_arrayList_addBool(list, true);

    celix_autofree char* output = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_STREQ("[true,false,true]", output);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveVersionArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createVersionArray();
    celix_arrayList_assignVersion(list, celix_version_create(1, 2, 3, nullptr));
    celix_arrayList_assignVersion(list, celix_version_create(2, 3, 4, nullptr));
    celix_arrayList_assignVersion(list, celix_version_create(3, 4, 5, nullptr));

    celix_autofree char* output = nullptr;
    auto status = celix_arrayList_saveToString(list, 0, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_STREQ(R"(["version<1.2.3>","version<2.3.4>","version<3.4.5>"])", output);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveArrayListWithEncodePrettyTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list, 1);
    celix_arrayList_addLong(list, 2);
    celix_arrayList_addLong(list, 3);

    celix_autofree char* output = nullptr;
    auto status = celix_arrayList_saveToString(list, CELIX_ARRAY_LIST_ENCODE_PRETTY, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_STREQ("[\n  1,\n  2,\n  3\n]", output);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveArrayListToFileTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createStringArray();
    celix_arrayList_addString(list, "str1");
    celix_arrayList_addString(list, "str2");
    celix_arrayList_addString(list, "str3");

    const char *filename = "/tmp/celix_array_list_encoding_test.txt";
    auto status = celix_arrayList_save(list, 0, filename);
    ASSERT_EQ(CELIX_SUCCESS, status);

    celix_autoptr(celix_array_list_t) loadedList = nullptr;
    status = celix_arrayList_load(filename, 0, &loadedList);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(loadedList != nullptr);
    EXPECT_EQ(3, celix_arrayList_size(loadedList));
    auto str1 = celix_arrayList_getString(loadedList, 0);
    EXPECT_STREQ("str1", str1);
    auto str2 = celix_arrayList_getString(loadedList, 1);
    EXPECT_STREQ("str2", str2);
    auto str3 = celix_arrayList_getString(loadedList, 2);
    EXPECT_STREQ("str3", str3);

    remove(filename);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadArrayListFromStreamWithInvalidArgsTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromStream(nullptr, 0, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    FILE *stream = fmemopen((void *) "[]", 2, "r");
    ASSERT_TRUE(stream != nullptr);
    status = celix_arrayList_loadFromStream(stream, 0, nullptr);
    fclose(stream);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadArrayListFromFileWithInvalidArgsTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_load(nullptr, 0, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = celix_arrayList_load("array_list_file.txt", 0, nullptr);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = celix_arrayList_load("not_existed_array_list_file.txt", 0, &list);
    ASSERT_EQ(CELIX_FILE_IO_EXCEPTION, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadArrayListFromStringWithInvalidArgsTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(nullptr, 0, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = celix_arrayList_loadFromString("[1,2,3]", 0, nullptr);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadInvalidArrayTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"(invalid array)", 0, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadNotArrayTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"({})", 0, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadEmptyArrayTest) {
    celix_array_list_t* list;
    auto status = celix_arrayList_loadFromString(R"([])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_EQ(nullptr, list);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadEmptyArrayWithEmptyErrorFlagsTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([])",
                            CELIX_ARRAY_LIST_DECODE_ERROR_ON_EMPTY_ARRAYS, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadNotSupportTypeArrayTest) {
    celix_array_list_t* list;
    auto status = celix_arrayList_loadFromString(R"([{}])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_EQ(nullptr, list);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadUnsupportTypeArrayWithUnsupportTypeErrorTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([{}])",
                                                 CELIX_ARRAY_LIST_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadMixedIntegerAndRealArrayTest) {
    {
        //integer before real
        celix_autoptr(celix_array_list_t) list = nullptr;
        auto status = celix_arrayList_loadFromString(R"([1, 2.2, 3])", 0, &list);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_EQ(3, celix_arrayList_size(list));
        ASSERT_EQ(1, celix_arrayList_getDouble(list, 0));
        ASSERT_EQ(2.2, celix_arrayList_getDouble(list, 1));
        ASSERT_EQ(3, celix_arrayList_getDouble(list, 2));
    }

    {
        //real before integer
        celix_autoptr(celix_array_list_t) list = nullptr;
        auto status = celix_arrayList_loadFromString(R"([1.1, 2, 3])", 0, &list);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_EQ(3, celix_arrayList_size(list));
        ASSERT_EQ(1.1, celix_arrayList_getDouble(list, 0));
        ASSERT_EQ(2, celix_arrayList_getDouble(list, 1));
        ASSERT_EQ(3, celix_arrayList_getDouble(list, 2));
    }
}

TEST_F(CelixArrayListEncodingTestSuite, LoadMixedIntegerAndStringArrayTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"([1, "str", 3])", CELIX_ARRAY_LIST_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = celix_arrayList_loadFromString(R"([1, "str", 3])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_EQ(nullptr, list);
}

TEST_F(CelixArrayListEncodingTestSuite, LoadMixedVersionAndStringArrayTest) {
    celix_array_list_t* list = nullptr;
    auto status = celix_arrayList_loadFromString(R"(["version<1.0.0>", "str", "version<2.0.0>"])", CELIX_ARRAY_LIST_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS, &list);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = celix_arrayList_loadFromString(R"(["version<1.0.0>", "str", "version<2.0.0>"])", 0, &list);
    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_EQ(nullptr, list);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveArrayListToStreamWithInvalidArgsTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_saveToStream(list, 0, nullptr);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    celix_autofree char* buffer = nullptr;
    size_t size = 0;
    celix_autoptr(FILE) stream = open_memstream(&buffer, &size);
    ASSERT_TRUE(stream != nullptr);
    status = celix_arrayList_saveToStream(nullptr, 0, stream);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveArrayListToFileWithInvalidArgsTest) {
    celix_autoptr(celix_array_list_t) list = nullptr;
    auto status = celix_arrayList_save(nullptr, 0, "array_list_file.txt");
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = celix_arrayList_save(list, 0, nullptr);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveArrayListToStringWithInvalidArgsTest) {
    char* out= nullptr;
    auto status = celix_arrayList_saveToString(nullptr, 0, &out);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    celix_autoptr(celix_array_list_t) list = celix_arrayList_createStringArray();
    status = celix_arrayList_saveToString(list, 0, nullptr);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(CelixArrayListEncodingTestSuite, SaveEmptyArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createLongArray();
    {
        celix_autofree char* output = nullptr;
        auto status = celix_arrayList_saveToString(list, 0, &output);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_STREQ("[]", output);
    }

    {
        celix_autofree char* output = nullptr;
        auto status = celix_arrayList_saveToString(list, CELIX_ARRAY_LIST_ENCODE_ERROR_ON_EMPTY_ARRAYS, &output);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }
}

TEST_F(CelixArrayListEncodingTestSuite, SaveNanArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(list, NAN);
    {
        celix_autofree char* output = nullptr;
        auto status = celix_arrayList_saveToString(list, 0, &output);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_STREQ("[]", output);
    }

    {
        celix_autofree char* output = nullptr;
        auto status = celix_arrayList_saveToString(list, CELIX_ARRAY_LIST_ENCODE_ERROR_ON_NAN_INF, &output);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }
}

TEST_F(CelixArrayListEncodingTestSuite, SaveInfArrayListTest) {
    celix_autoptr(celix_array_list_t) list = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(list, INFINITY);
    {
        celix_autofree char* output = nullptr;
        auto status = celix_arrayList_saveToString(list, 0, &output);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_STREQ("[]", output);
    }

    {
        celix_autofree char* output = nullptr;
        auto status = celix_arrayList_saveToString(list, CELIX_ARRAY_LIST_ENCODE_ERROR_ON_NAN_INF, &output);
        ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }
}
