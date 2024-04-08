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

#include <cmath>
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

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithSingleValuesTest) {
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

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithNaNAndInfValuesTest) {
    //Given a NAN, INF and -INF value
    auto keys = {"NAN", "INF", "-INF"};
    for (const auto& key : keys) {
        //For every value

        //Given a properties object with a NAN, INF or -INF value
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setDouble(props, key, strtod(key, nullptr));

        //And an in-memory stream
        celix_autofree char* buf = nullptr;
        size_t bufLen = 0;
        FILE* stream = open_memstream(&buf, &bufLen);

        //Then saving the properties to the stream fails, because JSON does not support NAN, INF and -INF
        celix_err_resetErrors();
        auto status = celix_properties_saveToStream(props, stream);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And an error msg is added to celix_err
        EXPECT_EQ(1, celix_err_getErrorCount());
    }
}


TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithArrayListsTest) {
    // Given a properties object with array list values
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_array_list_t* list1 = celix_arrayList_createStringArray();
    celix_arrayList_addString(list1, "value1");
    celix_arrayList_addString(list1, "value2");
    celix_properties_assignArrayList(props, "key1", list1);
    celix_array_list_t* list2 = celix_arrayList_createLongArray();
    celix_arrayList_addLong(list2, 1);
    celix_arrayList_addLong(list2, 2);
    celix_properties_assignArrayList(props, "key2", list2);
    celix_array_list_t* list3 = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(list3, 1.0);
    celix_arrayList_addDouble(list3, 2.0);
    celix_properties_assignArrayList(props, "key3", list3);
    celix_array_list_t* list4 = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(list4, true);
    celix_arrayList_addBool(list4, false);
    celix_properties_assignArrayList(props, "key4", list4);
    celix_array_list_t* list5 = celix_arrayList_createVersionArray();
    celix_arrayList_addVersion(list5, celix_version_create(1, 2, 3, "qualifier"));
    celix_arrayList_addVersion(list5, celix_version_create(4, 5, 6, "qualifier"));
    celix_properties_assignArrayList(props, "key5", list5);

    // And an in-memory stream
    celix_autofree char* buf = nullptr;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);

    // When saving the properties to the stream
    auto status = celix_properties_saveToStream(props, stream);
    EXPECT_EQ(CELIX_SUCCESS, status);

    // Then the stream contains the JSON representation snippets of the properties
    fclose(stream);
    EXPECT_NE(nullptr, strstr(buf, R"("key1":["value1","value2"])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key2":[1,2])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key3":[1.0,2.0])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key4":[true,false])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key5":["celix_version<1.2.3.qualifier>","celix_version<4.5.6.qualifier>"])"))
        << "JSON: " << buf;

    // And the buf is a valid JSON object
    json_error_t error;
    json_t* root = json_loads(buf, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
    json_decref(root);
}


TEST_F(PropertiesSerializationTestSuite, SaveEmptyArrayTest) {
    //Given a properties object with an empty array list of with el types string, long, double, bool, version
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_assignArrayList(props, "key1", celix_arrayList_createStringArray());
    celix_properties_assignArrayList(props, "key2", celix_arrayList_createLongArray());
    celix_properties_assignArrayList(props, "key3", celix_arrayList_createDoubleArray());
    celix_properties_assignArrayList(props, "key4", celix_arrayList_createBoolArray());
    celix_properties_assignArrayList(props, "key5", celix_arrayList_createVersionArray());
    EXPECT_EQ(5, celix_properties_size(props));

    //And an in-memory stream
    celix_autofree char* buf = nullptr;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);

    //When saving the properties to the stream
    auto status = celix_properties_saveToStream(props, stream);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains an empty JSON object, because empty arrays are treated as unset
    fclose(stream);
    EXPECT_STREQ("{}", buf);
}

TEST_F(PropertiesSerializationTestSuite, SaveJPathKeysTest) {
    //Given a properties object with jpath keys
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_set(props, "object1/key3", "value3");
    celix_properties_set(props, "object1/key4", "value4");
    celix_properties_set(props, "object2/key5", "value5");
    celix_properties_set(props, "object3/object4/key6", "value6");

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
    EXPECT_NE(nullptr, strstr(buf, R"("object1":{"key3":"value3","key4":"value4"})")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("object2":{"key5":"value5"})")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("object3":{"object4":{"key6":"value6"}})")) << "JSON: " << buf;

    //And the buf is a valid JSON object
    json_error_t error;
    json_t* root = json_loads(buf, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
    json_decref(root);
}

TEST_F(PropertiesSerializationTestSuite, SaveJPathKeysWithCollisionTest) {
    //Given a properties object with jpath keys that collide
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1/key2/key3", "value1");
    celix_properties_set(props, "key1/key2", "value2"); //collision with object "key1/key2"
    celix_properties_set(props, "key4/key5/key6", "value3");
    celix_properties_set(props, "key4/key5/key6/key7", "value4"); //collision with field "key3/key4/key5"

    //And an in-memory stream
    celix_autofree char* buf = nullptr;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);

    //When saving the properties to the stream
    auto status = celix_properties_saveToStream(props, stream);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains the JSON representation snippets of the properties
    fclose(stream);
    EXPECT_NE(nullptr, strstr(buf, R"("key1":{"key2":{"key3":"value1"}})")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key1/key2":"value2")")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key4/key5/key6":"value3")")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key4":{"key5":{"key6":{"key7":"value4"}}})")) << "JSON: " << buf;
    //Note whether "key1/key2/key3" or "key1/key2" is serializer first depends on the hash order of the keys,
    //so this test can change if the string hash map implementation changes.

    //And the buf is a valid JSON object
    json_error_t error;
    json_t* root = json_loads(buf, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
    json_decref(root);
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

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithSingleValuesTest) {
    //Given a JSON object with single values for types string, long, double, bool and version
    const char* jsonInput = R"({
        "strKey":"strValue",
        "longKey":42,
        "doubleKey":2.0,
        "boolKey":true,
        "versionKey":"celix_version<1.2.3.qualifier>"
    })";

    //And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");


    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, &props);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the properties object contains the single values
    EXPECT_EQ(5, celix_properties_size(props));
    EXPECT_STREQ("strValue", celix_properties_getString(props, "strKey"));
    EXPECT_EQ(42, celix_properties_getLong(props, "longKey", -1));
    EXPECT_DOUBLE_EQ(2.0, celix_properties_getDouble(props, "doubleKey", NAN));
    EXPECT_TRUE(celix_properties_getBool(props, "boolKey", false));
    auto* v = celix_properties_getVersion(props, "versionKey");
    ASSERT_NE(nullptr, v);
    celix_autofree char* vStr = celix_version_toString(v);
    EXPECT_STREQ("1.2.3.qualifier", vStr);
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithArrayListsTest) {
    //Given a JSON object with array values for types string, long, double, bool and version
    const char* jsonInput = R"({
        "strArr":["value1","value2"],
        "intArr":[1,2],
        "realArr":[1.0,2.0],
        "boolArr":[true,false],
        "versionArr":["celix_version<1.2.3.qualifier>","celix_version<4.5.6.qualifier>"]
    })";

    //And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, &props);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Then the properties object contains the array values
    EXPECT_EQ(5, celix_properties_size(props));

    //And the string array is correctly loaded
    auto* strArr = celix_properties_getArrayList(props, "strArr");
    ASSERT_NE(nullptr, strArr);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_STRING, celix_arrayList_getElementType(strArr));
    EXPECT_EQ(2, celix_arrayList_size(strArr));
    EXPECT_STREQ("value1", celix_arrayList_getString(strArr, 0));
    EXPECT_STREQ("value2", celix_arrayList_getString(strArr, 1));

    //And the long array is correctly loaded
    auto* intArr = celix_properties_getArrayList(props, "intArr");
    ASSERT_NE(nullptr, intArr);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_LONG, celix_arrayList_getElementType(intArr));
    EXPECT_EQ(2, celix_arrayList_size(intArr));
    EXPECT_EQ(1, celix_arrayList_getLong(intArr, 0));
    EXPECT_EQ(2, celix_arrayList_getLong(intArr, 1));

    //And the double array is correctly loaded
    auto* realArr = celix_properties_getArrayList(props, "realArr");
    ASSERT_NE(nullptr, realArr);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE, celix_arrayList_getElementType(realArr));
    EXPECT_EQ(2, celix_arrayList_size(realArr));
    EXPECT_DOUBLE_EQ(1.0, celix_arrayList_getDouble(realArr, 0));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(realArr, 1));

    //And the bool array is correctly loaded
    auto* boolArr = celix_properties_getArrayList(props, "boolArr");
    ASSERT_NE(nullptr, boolArr);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_BOOL, celix_arrayList_getElementType(boolArr));
    EXPECT_EQ(2, celix_arrayList_size(boolArr));
    EXPECT_TRUE(celix_arrayList_getBool(boolArr, 0));
    EXPECT_FALSE(celix_arrayList_getBool(boolArr, 1));

    //And the version array is correctly loaded
    auto* versionArr = celix_properties_getArrayList(props, "versionArr");
    ASSERT_NE(nullptr, versionArr);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_VERSION, celix_arrayList_getElementType(versionArr));
    EXPECT_EQ(2, celix_arrayList_size(versionArr));
    auto* v1 = celix_arrayList_getVersion(versionArr, 0);
    ASSERT_NE(nullptr, v1);
    celix_autofree char* v1Str = celix_version_toString(v1);
    EXPECT_STREQ("1.2.3.qualifier", v1Str);
    auto* v2 = celix_arrayList_getVersion(versionArr, 1);
    ASSERT_NE(nullptr, v2);
    celix_autofree char* v2Str = celix_version_toString(v2);
    EXPECT_STREQ("4.5.6.qualifier", v2Str);
}

//TODO test with combination json_int and json_real, this should be promoted to double

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithInvalidInputTest) {
    auto invalidInputs = {
        R"({)",                            // invalid JSON (caught by jansson)
        R"({"emptyArr":[]})",              // Empty array, not supported
        R"({"mixedArr":["string", true]})", // Mixed array, not supported
        R"({"mixedArr":[1.9, 2]})", // Mixed array, TODO this should be supported
    };
    for (auto& invalidInput: invalidInputs) {
        //Given an invalid JSON object
        FILE* stream = fmemopen((void*)invalidInput, strlen(invalidInput), "r");

        //When loading the properties from the stream
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_loadFromStream(stream, &props);

        //Then loading fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stderr, "Error: ", "\n");

        fclose(stream);
    }
}

//TODO test deserialize null values
//TODO test serialize with empty array (treated as unset)
//TODO test with jpath subset keys and json serialization
//TODO test with key starting and ending with slash
