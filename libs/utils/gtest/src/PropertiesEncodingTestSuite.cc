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

TEST_F(PropertiesSerializationTestSuite, EncodeEmptyPropertiesTest) {
    //Given an empty properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    //And an in-memory stream
    celix_autofree char* buf = nullptr;
    size_t bufLen = 0;
    FILE* stream = open_memstream(&buf, &bufLen);

    //When encoding the properties to the stream
    auto status = celix_properties_encodeToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains an empty JSON object
    fclose(stream);
    EXPECT_STREQ("{}", buf);
}

TEST_F(PropertiesSerializationTestSuite, EncodePropertiesWithSingleValuesTest) {
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

        //When encoding the properties to the stream
        auto status = celix_properties_encodeToStream(props, stream, 0);
        ASSERT_EQ(CELIX_SUCCESS, status);

        //Then the stream contains the JSON representation snippets of the properties
        fclose(stream);
        EXPECT_NE(nullptr, strstr(buf, R"("key1":"value1")")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key2":"value2")")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key3":3)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key4":4.0)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key5":true)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key6":"celix_version<1.2.3.qualifier>")")) << "JSON: " << buf;

        //And the buf is a valid JSON object
        json_error_t error;
        json_t* root = json_loads(buf, 0, &error);
        EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
        json_decref(root);
}

TEST_F(PropertiesSerializationTestSuite, EncodePropertiesWithNaNAndInfValuesTest) {
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
        auto status = celix_properties_encodeToStream(props, stream, 0);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And an error msg is added to celix_err
        EXPECT_EQ(1, celix_err_getErrorCount());
    }
}


TEST_F(PropertiesSerializationTestSuite, EncodePropertiesWithArrayListsTest) {
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
    auto status = celix_properties_encodeToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

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


TEST_F(PropertiesSerializationTestSuite, EncodeEmptyArrayTest) {
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

    //When encoding the properties to the stream
    auto status = celix_properties_encodeToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains an empty JSON object, because empty arrays are treated as unset
    fclose(stream);
    EXPECT_STREQ("{}", buf);
}

TEST_F(PropertiesSerializationTestSuite, EncodeJPathKeysTest) {
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

    //When encoding the properties to the stream
    auto status = celix_properties_encodeToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

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

TEST_F(PropertiesSerializationTestSuite, EncodeJPathKeysWithCollisionTest) {
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

    //When encoding the properties to the stream
    auto status = celix_properties_encodeToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

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


//TODO check desired behaviour, currently every "/" leads to a new object (except if an collision occurs)
//TEST_F(PropertiesSerializationTestSuite, EncodePropertiesWithSpecialKeyNamesTest) {
//    //Given a properties set with special key names (slashes)
//    celix_autoptr(celix_properties_t) props = celix_properties_create();
//    celix_properties_set(props, "/", "value1");
//    celix_properties_set(props, "keyThatEndsWithSlash/", "value2");
//    celix_properties_set(props, "key//With//Double//Slash", "value3");
//    celix_properties_set(props, "object/", "value5");
//    celix_properties_set(props, "object//", "value4");
//    celix_properties_set(props, "object/keyThatEndsWithSlash/", "value6");
//    celix_properties_set(props, "object/key//With//Double//Slash", "value7");
//
//    //And an in-memory stream
//    celix_autofree char* buf = nullptr;
//    size_t bufLen = 0;
//    FILE* stream = open_memstream(&buf, &bufLen);
//
//    //When encoding the properties to the stream
//    auto status = celix_properties_encodeToStream(props, stream, 0);
//    ASSERT_EQ(CELIX_SUCCESS, status);
//
//    std::cout << buf << std::endl;
//
//    //Then the stream contains the JSON representation snippets of the properties
//    fclose(stream);
//    EXPECT_NE(nullptr, strstr(buf, R"("/":"value1")")) << "JSON: " << buf;
//    EXPECT_NE(nullptr, strstr(buf, R"("keyThatEndsWithSlash/":"value2")")) << "JSON: " << buf;
//    EXPECT_NE(nullptr, strstr(buf, R"("key//With//Double//Slash":"value3")")) << "JSON: " << buf;
//    EXPECT_NE(nullptr, strstr(buf, R"("object/":"value5")")) << "JSON: " << buf;
//    EXPECT_NE(nullptr, strstr(buf, R"("/":"value5")")) << "JSON: " << buf; //child of object
//    EXPECT_NE(nullptr, strstr(buf, R"("keyThatEndsWithSlash/":"value6")")) << "JSON: " << buf; //child of object
//    EXPECT_NE(nullptr, strstr(buf, R"("key//With//Double//Slash":"value7")")) << "JSON: " << buf; //child of object
//
//
//    //And the buf is a valid JSON object
//    json_error_t error;
//    json_t* root = json_loads(buf, 0, &error);
//    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
//    json_decref(root);
//}


TEST_F(PropertiesSerializationTestSuite, DecodeEmptyPropertiesTest) {
    //Given an empty JSON object
    const char* json = "{}";
    FILE* stream = fmemopen((void*)json, strlen(json), "r");

    //When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the properties object is empty
    EXPECT_EQ(0, celix_properties_size(props));

    fclose(stream);
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithSingleValuesTest) {
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

    //When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);
    ASSERT_EQ(CELIX_SUCCESS, status);

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

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithArrayListsTest) {
    //Given a JSON object with array values for types string, long, double, bool and version
    const char* jsonInput = R"({
        "strArr":["value1","value2"],
        "intArr":[1,2],
        "realArr":[1.0,2.0],
        "boolArr":[true,false],
        "versionArr":["celix_version<1.2.3.qualifier>","celix_version<4.5.6.qualifier>"],
        "mixedRealAndIntArr1":[1,2.0,2,3.0],
        "mixedRealAndIntArr2":[1.0,2,2.0,3]
    })";

    //And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    //When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the properties object contains the array values
    EXPECT_EQ(7, celix_properties_size(props));

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

    //And the mixed json real and int arrays are correctly loaded as double arrays
    auto* mixedRealAndIntArr1 = celix_properties_getArrayList(props, "mixedRealAndIntArr1");
    ASSERT_NE(nullptr, mixedRealAndIntArr1);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE, celix_arrayList_getElementType(mixedRealAndIntArr1));
    EXPECT_EQ(4, celix_arrayList_size(mixedRealAndIntArr1));
    EXPECT_DOUBLE_EQ(1.0, celix_arrayList_getDouble(mixedRealAndIntArr1, 0));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(mixedRealAndIntArr1, 1));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(mixedRealAndIntArr1, 2));
    EXPECT_DOUBLE_EQ(3.0, celix_arrayList_getDouble(mixedRealAndIntArr1, 3));

    auto* mixedRealAndIntArr2 = celix_properties_getArrayList(props, "mixedRealAndIntArr2");
    ASSERT_NE(nullptr, mixedRealAndIntArr2);
    EXPECT_EQ(CELIX_ARRAY_LIST_ELEMENT_TYPE_DOUBLE, celix_arrayList_getElementType(mixedRealAndIntArr2));
    EXPECT_EQ(4, celix_arrayList_size(mixedRealAndIntArr2));
    EXPECT_DOUBLE_EQ(1.0, celix_arrayList_getDouble(mixedRealAndIntArr2, 0));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(mixedRealAndIntArr2, 1));
    EXPECT_DOUBLE_EQ(2.0, celix_arrayList_getDouble(mixedRealAndIntArr2, 2));
    EXPECT_DOUBLE_EQ(3.0, celix_arrayList_getDouble(mixedRealAndIntArr2, 3));
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithInvalidInputTest) {
    auto invalidInputs = {
        R"({)",                            // invalid JSON (caught by jansson)
        R"([])",                           // unsupported JSON (top level array not supported)
        R"(42)",                           // invalid JSON (caught by jansson)
    };
    for (auto& invalidInput: invalidInputs) {
        //Given an invalid JSON object
        FILE* stream = fmemopen((void*)invalidInput, strlen(invalidInput), "r");

        //When decoding the properties from the stream
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_decodeFromStream(stream, 0, &props);

        //Then loading fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stderr, "Error: ", "\n");

        fclose(stream);
    }
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithEmptyArrayTest) {
    //Given a JSON object with an empty array
    auto* emptyArrays = R"({"key1":[]})";

    //And a stream with the JSON object
    FILE* stream = fmemopen((void*)emptyArrays, strlen(emptyArrays), "r");

    //When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);

    //Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    //And the properties object is empty, because empty arrays are treated as unset
    EXPECT_EQ(0, celix_properties_size(props));
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithNestedObjectsTest) {
    // Given a complex JSON object
    const char* jsonInput = R"({
        "key1":"value1",
        "key2":"value2",
        "object1": {
            "key3":"value3",
            "key4":true
        },
        "object2": {
            "key5":5.0
        },
        "object3":{
            "object4":{
                "key6":6
            }
        }
    })";

    // And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    // When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);
    ASSERT_EQ(CELIX_SUCCESS, status);

    // Then the properties object contains the nested objects
    EXPECT_EQ(6, celix_properties_size(props));
    EXPECT_STREQ("value1", celix_properties_getString(props, "key1"));
    EXPECT_STREQ("value2", celix_properties_getString(props, "key2"));
    EXPECT_STREQ("value3", celix_properties_getString(props, "object1/key3"));
    EXPECT_EQ(true, celix_properties_getBool(props, "object1/key4", false));
    EXPECT_DOUBLE_EQ(5., celix_properties_getDouble(props, "object2/key5", 0.0));
    EXPECT_EQ(6, celix_properties_getLong(props, "object3/object4/key6", 0));
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithNestedObjectsAndJPathCollisionTest) {
    // Given a complex JSON object with jpath keys that collide
    const char* jsonInput = R"({
        "object1": {
            "object2": {
                "key1":true
            }
        },
        "object1/object2/key1":6,
        "key2":2,
        "key2":3
    })";

    // And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    // When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);

    // Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the properties object contains the last values of the jpath keys
    EXPECT_EQ(2, celix_properties_size(props));
    EXPECT_EQ(6, celix_properties_getLong(props, "object1/object2/key1", 0));
    EXPECT_EQ(3, celix_properties_getLong(props, "key2", 0));

    // When the stream is reset
    fseek(stream, 0, SEEK_SET);

    // And decoding the properties from the stream using a flog that does not allow collisions
    celix_autoptr(celix_properties_t) props2 = nullptr;
    status = celix_properties_decodeFromStream(stream, CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES, &props2);

    // Then loading fails, because of a duplicate key
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithStrictEnabledDisabledTest) {
    auto invalidInputs = {
        R"({"mixedArr":["string", true]})", // Mixed array gives error on strict
        R"({"key1":null})",                 // Null value gives error on strict
        R"({"":"value"})",                  // "" key gives error on strict
        R"({"emptyArr":[]})",               // Empty array gives error on strict
        R"({"key1":"val1", "key1":"val2"})",// Duplicate key gives error on strict
        R"({"nullArr":[null,null]})",       // Array with null values gives error on strict
    };

    for (auto& invalidInput: invalidInputs) {
        //Given an invalid JSON object
        FILE* stream = fmemopen((void*)invalidInput, strlen(invalidInput), "r");

        //When decoding the properties from the stream with an empty flags
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_decodeFromStream(stream, 0, &props);
        celix_err_printErrors(stderr, "Error: ", "\n");

        //Then decoding succeeds, because strict is disabled
        ASSERT_EQ(CELIX_SUCCESS, status);
        EXPECT_GE(celix_err_getErrorCount(), 0);

        //But the properties size is 0 or 1, because the all invalid inputs are ignored, except the duplicate key
        auto size = celix_properties_size(props);
        EXPECT_TRUE(size == 0 || size == 1);

        fclose(stream);
    }

    for (auto& invalidInput: invalidInputs) {
        //Given an invalid JSON object
        FILE* stream = fmemopen((void*)invalidInput, strlen(invalidInput), "r");

        //When decoding the properties from the stream with a strict flag
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_decodeFromStream(stream, CELIX_PROPERTIES_DECODE_STRICT, &props);

        //Then decoding fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stderr, "Error: ", "\n");

        fclose(stream);
    }
}

TEST_F(PropertiesSerializationTestSuite, DecodePropertiesWithSpecialKeyNamesTest) {
    // Given a complex JSON object
    const char* jsonInput = R"({
        "/": "value1",
        "keyThatEndsWithSlash/": "value2",
        "key//With//Double//Slash": "value3",
        "object": {
            "/": "value4",
            "keyThatEndsWithSlash/": "value5",
            "key//With//Double//Slash": "value6"
        }
    })";

    // And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    // When decoding the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_decodeFromStream(stream, 0, &props);
    celix_err_printErrors(stderr, "Error: ", "\n");
    ASSERT_EQ(CELIX_SUCCESS, status);

    // Then the properties object contains the nested objects
    EXPECT_EQ(6, celix_properties_size(props));
    EXPECT_STREQ("value1", celix_properties_getString(props, "/"));
    EXPECT_STREQ("value2", celix_properties_getString(props, "keyThatEndsWithSlash/"));
    EXPECT_STREQ("value3", celix_properties_getString(props, "key//With//Double//Slash"));
    EXPECT_STREQ("value4", celix_properties_getString(props, "object//"));
    EXPECT_STREQ("value5", celix_properties_getString(props, "object/keyThatEndsWithSlash/"));
    EXPECT_STREQ("value6", celix_properties_getString(props, "object/key//With//Double//Slash"));
}

//TODO test with invalid version string
//TODO is there a strict option needed for version (e.g. not parseable as version handle as string)
//TODO test encoding flags
//TODO error injection tests and wrappers for jansson functions