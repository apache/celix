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
#include <jansson.h>

#include "celix_err.h"
#include "celix_properties.h"
#include "celix_stdlib_cleanup.h"

using ::testing::MatchesRegex;

class PropertiesSerializationTestSuite : public ::testing::Test {
  public:
    PropertiesSerializationTestSuite() { celix_err_resetErrors(); }
};

TEST_F(PropertiesSerializationTestSuite, SaveEmptyPropertiesTest) {
    //Given an empty properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    //And an in-memory stream
    celix_autofree char* buf = nullptr;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);

    //When saving the properties to the stream
    auto status = celix_properties_saveToStream(props, stream);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains an empty JSON object
    fclose(stream);
    EXPECT_STREQ("{}", buf);
}

TEST_F(PropertiesSerializationTestSuite, LoadEmptyPropertiesTest) {
    //Given an empty JSON object
    const char* json = "{}";
    FILE* stream = fmemopen((void*)json, strlen(json), "r");

    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, &props);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the properties object is empty
    EXPECT_EQ(0, celix_properties_size(props));

    fclose(stream);
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithSingelValuesTest) {
        //Given a properties object with single values
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, "key1", "value1");
        celix_properties_set(props, "key2", "value2");
        celix_properties_setLong(props, "key3", 3);
        celix_properties_setDouble(props, "key4", 4.0);
        celix_properties_setBool(props, "key5", true);
        celix_properties_assignVersion(props, "key6", celix_version_create(1, 2, 3, "qualifier"));

        //And an in-memory stream
        celix_autofree char* buf = nullptr;
        size_t bufLen = 0;
        FILE* stream = open_memstream(&buf, &bufLen);

        //When saving the properties to the stream
        auto status = celix_properties_saveToStream(props, stream);
        EXPECT_EQ(CELIX_SUCCESS, status);

        //Then the stream contains the JSON representation snippets of the properties
        fclose(stream);
        EXPECT_NE(nullptr, strstr(buf, R"("key1":"value1")")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key2":"value2")")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key3":3)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key4":4.0)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key5":true)")) << "JSON: " << buf;

        //TODO how are versions serialized? A string representation is needed to reconstruct the version from JSON
        EXPECT_NE(nullptr, strstr(buf, R"("key6":"celix_version<1.2.3.qualifier>")")) << "JSON: " << buf;

        //And the buf is a valid JSON object
        json_error_t error;
        json_t* root = json_loads(buf, 0, &error);
        EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
        json_decref(root);
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithArrayListsTest) {
    //Given a properties object with array list values
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_array_list_t* list1 = celix_arrayList_createStringArray();
    celix_arrayList_addString(list1, "value1");
    celix_arrayList_addString(list1, "value2");
    celix_properties_assignArrayList(props, "key1", list1);
    //TODO long, double, bool, version

    //And an in-memory stream
    celix_autofree char* buf = nullptr;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);

    //When saving the properties to the stream
    auto status = celix_properties_saveToStream(props, stream);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains the JSON representation snippets of the properties
    fclose(stream);
    EXPECT_NE(nullptr, strstr(buf, R"("key1":["value1","value2"])")) << "JSON: " << buf;

    //And the buf is a valid JSON object
    json_error_t error;
    json_t* root = json_loads(buf, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
    json_decref(root);
}
