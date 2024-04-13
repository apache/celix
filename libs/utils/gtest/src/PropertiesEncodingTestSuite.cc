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
    auto status = celix_properties_saveToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

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
        auto status = celix_properties_saveToStream(props, stream, 0);
        ASSERT_EQ(CELIX_SUCCESS, status);

        //Then the stream contains the JSON representation snippets of the properties
        fclose(stream);
        EXPECT_NE(nullptr, strstr(buf, R"("key1":"value1")")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key2":"value2")")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key3":3)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key4":4.0)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key5":true)")) << "JSON: " << buf;
        EXPECT_NE(nullptr, strstr(buf, R"("key6":"version<1.2.3.qualifier>")")) << "JSON: " << buf;

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
        auto status = celix_properties_saveToStream(props, stream, 0);
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
    auto status = celix_properties_saveToStream(props, stream, 0);
    ASSERT_EQ(CELIX_SUCCESS, status);

    // Then the stream contains the JSON representation snippets of the properties
    fclose(stream);
    EXPECT_NE(nullptr, strstr(buf, R"("key1":["value1","value2"])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key2":[1,2])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key3":[1.0,2.0])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key4":[true,false])")) << "JSON: " << buf;
    EXPECT_NE(nullptr, strstr(buf, R"("key5":["version<1.2.3.qualifier>","version<4.5.6.qualifier>"])"))
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

    //When saving the properties to a string
    char* output = nullptr;
    auto status = celix_properties_saveToString(props, 0, &output);

    //Then the save went ok
    ASSERT_EQ(CELIX_SUCCESS, status);

    //And the output contains an empty JSON object, because empty arrays are treated as unset
    EXPECT_STREQ("{}", output);

    //When saving the properties to a string with an error on  empty array flag
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS, &output);

    //Then the save fails, because the empty array generates an error
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");
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
    auto status = celix_properties_saveToStream(props, stream, CELIX_PROPERTIES_ENCODE_NESTED);
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

TEST_F(PropertiesSerializationTestSuite, SaveJPathKeysWithCollisionTest) {
    // note this tests depends on the key iteration order for properties and
    // properties key order is based on hash order of the keys, so this test can change if the string hash map
    // implementation changes.

    //Given a properties object with jpath keys that collide
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1/key2/key3", "value1");
    celix_properties_set(props, "key1/key2", "value2"); //collision with object "key1/key2/key3" -> overwrite
    celix_properties_set(props, "key4/key5/key6/key7", "value4");
    celix_properties_set(props, "key4/key5/key6", "value3"); //collision with field "key4/key5/key6/key7" -> overwrite

    //When saving the properties to a string
    celix_autofree char* output = nullptr;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains the JSON representation of the properties with the collisions resolved
    EXPECT_NE(nullptr, strstr(output, R"({"key1":{"key2":"value2"},"key4":{"key5":{"key6":"value3"}}})"))
        << "JSON: " << output;

    //And the buf is a valid JSON object
    json_error_t error;
    json_t* root = json_loads(output, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
    json_decref(root);
}



TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithKeyNamesWithSlashesTest) {
    //Given a properties set with key names with slashes
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "a/key/name/with/slashes", "value1");
    celix_properties_set(props, "/keyThatStartsWithSlash", "value3");
    celix_properties_set(props, "keyThatEndsWithSlash/", "value5");
    celix_properties_set(props, "keyThatEndsWithDoubleSlashes//", "value6");
    celix_properties_set(props, "key//With//Double//Slashes", "value7");
    celix_properties_set(props, "object/keyThatEndsWithSlash/", "value8");
    celix_properties_set(props, "object/keyThatEndsWithDoubleSlashes//", "value9");
    celix_properties_set(props, "object/key//With//Double//Slashes", "value10");


    //When saving the properties to a string
    char* output = nullptr;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the out contains the JSON representation snippets of the properties
    EXPECT_NE(nullptr, strstr(output, R"("a":{"key":{"name":{"with":{"slashes":"value1"}}}})")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("keyThatStartsWithSlash":"value3")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value5")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value6")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("Slashes":"value7")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value8")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value9")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("Slashes":"value10")")) << "JSON: " << output;

    //And the output is a valid JSON object
    json_error_t error;
    json_t* root = json_loads(output, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;


    //And the structure for (e.g.) value10 is correct
    json_t* node = json_object_get(root, "object");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "key");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "With");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "Double");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_object(node));
    node = json_object_get(node, "Slashes");
    ASSERT_NE(nullptr, node);
    ASSERT_TRUE(json_is_string(node));
    EXPECT_STREQ("value10", json_string_value(node));

    json_decref(root);
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithKeyCollision) {
    // note this tests depends on the key iteration order for properties and
    // properties key order is based on hash order of the keys, so this test can change if the string hash map
    // implementation changes.

    //Given a properties that contains keys that will collide with an existing JSON object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1/key2/key3", "value1");
    celix_properties_set(props, "key1/key2", "value2"); //collision with object "key1/key2" -> overwrite

    //When saving the properties to a string
    char* output = nullptr;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED, &output);

    //Then the save succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And both keys are serialized (one as a flat key) (flat key name is whitebox knowledge)
    EXPECT_NE(nullptr, strstr(output, R"({"key1":{"key2":"value2"}})")) << "JSON: " << output;

    //When saving the properties to a string with the error on key collision flag
    status = celix_properties_saveToString(
        props, CELIX_PROPERTIES_ENCODE_NESTED | CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS, &output);

    //Then the save fails, because the keys collide
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithAndWithoutStrictFlagTest) {
    //Given a properties set with an empty array list
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    auto* list = celix_arrayList_createStringArray();
    celix_properties_assignArrayList(props, "key1", list);

    //When saving the properties to a string without the strict flag
    char* output = nullptr;
    auto status = celix_properties_saveToString(props, 0, &output);

    //Then the save succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    //When saving the properties to a string with the strict flag
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_STRICT, &output);

    //Then the save fails, because the empty array generates an error
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithPrettyPrintTest) {
    //Given a properties set with 2 keys
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");

    //When saving the properties to a string with pretty print
    char* output = nullptr;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_PRETTY, &output);

    //Then the save succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the output contains the JSON representation snippets of the properties with pretty print (2 indent spaces and
    // newlines)
    auto* expected = "{\n  \"key2\": \"value2\",\n  \"key1\": \"value1\"\n}";
    EXPECT_STREQ(expected, output);
}

TEST_F(PropertiesSerializationTestSuite, SaveWithInvalidStreamTest) {
    celix_autoptr(celix_properties_t) properties = celix_properties_create();
    celix_properties_set(properties, "key", "value");

    // Saving properties with invalid stream will fail
    auto status = celix_properties_save(properties, "/non-existing/no/rights/file.json", 0);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, status);
    EXPECT_EQ(1, celix_err_getErrorCount());

    auto* readStream = fopen("/dev/null", "r");
    status = celix_properties_saveToStream(properties, readStream, 0);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, status);
    EXPECT_EQ(2, celix_err_getErrorCount());
    fclose(readStream);

    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, LoadEmptyPropertiesTest) {
    //Given an empty JSON object
    const char* json = "{}";
    FILE* stream = fmemopen((void*)json, strlen(json), "r");

    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, 0, &props);
    ASSERT_EQ(CELIX_SUCCESS, status);

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
        "versionKey":"version<1.2.3.qualifier>"
    })";

    //And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, 0, &props);
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

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithArrayListsTest) {
    //Given a JSON object with array values for types string, long, double, bool and version
    const char* jsonInput = R"({
        "strArr":["value1","value2"],
        "intArr":[1,2],
        "realArr":[1.0,2.0],
        "boolArr":[true,false],
        "versionArr":["version<1.2.3.qualifier>","version<4.5.6.qualifier>"],
        "mixedRealAndIntArr1":[1,2.0,2,3.0],
        "mixedRealAndIntArr2":[1.0,2,2.0,3]
    })";

    //And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, 0, &props);
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

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithInvalidInputTest) {
    auto invalidInputs = {
        R"({)",                            // invalid JSON (caught by jansson)
        R"([])",                           // unsupported JSON (top level array not supported)
        R"(42)",                           // invalid JSON (caught by jansson)
    };
    for (auto& invalidInput: invalidInputs) {
        //Given an invalid JSON object
        FILE* stream = fmemopen((void*)invalidInput, strlen(invalidInput), "r");

        //When loading the properties from the stream
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_loadFromStream(stream, 0, &props);

        //Then loading fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stderr, "Error: ", "\n");

        fclose(stream);
    }
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithEmptyArrayTest) {
    //Given a JSON object with an empty array
    auto* inputJSON = R"({"key1":[]})";

    //When loading the properties from string
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromString2(inputJSON, 0, &props);

    //Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    //And the properties object is empty, because empty arrays are treated as unset
    EXPECT_EQ(0, celix_properties_size(props));

    //When loading the properties from string with a strict flag
    status = celix_properties_loadFromString2(inputJSON, CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS, &props);

    //Then loading fails, because the empty array generates an error
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    ASSERT_GE(celix_err_getErrorCount(), 1);
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithNestedObjectsTest) {
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

    // When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, 0, &props);
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

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithDuplicatesTest) {
    // Given a complex JSON object with duplicate keys
    const char* jsonInput = R"({
        "key":2,
        "key":3
    })";

    // When loading the properties from a string.
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromString2(jsonInput, 0, &props);

    // Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the properties object contains the last values of the jpath keys
    EXPECT_EQ(1, celix_properties_size(props));
    EXPECT_EQ(3, celix_properties_getLong(props, "key", 0));

    // When decoding the properties from the stream using a flog that does not allow duplicates
    celix_autoptr(celix_properties_t) props2 = nullptr;
    status = celix_properties_loadFromString2(jsonInput, CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES, &props2);

    // Then loading fails, because of a duplicate key
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesEscapedSlashesTest) {
    // Given a complex JSON object with collisions and duplicate keys
    // Collisions:
    // - object object1/object2 and value object1/object2
    // - value key1 in object2 in object1 and value object2/key in object1
    // - value object3/key4 and value key4 in object object3
    // Duplicate JSON keys:
    // - key3
    const char* jsonInput = R"({
        "object1": {
            "object2": {
                "key1": "value1"
            },
            "object2/key2": "value2"
        },
        "object1/object2" : "value3",
        "key3": "value4",
        "key3": "value5",
        "object3/key4": "value6",
        "object3": {
            "key4": "value7"
        }
    })";

    // When loading the properties from a string.
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromString2(jsonInput, 0, &props);

    // Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the properties object all the values for the colliding keys and a single (latest) value for the duplicate
    // keys
    EXPECT_EQ(5, celix_properties_size(props));
    EXPECT_STREQ("value1", celix_properties_getString(props, "object1/object2/key1"));
    EXPECT_STREQ("value2", celix_properties_getString(props, "object1/object2/key2"));
    EXPECT_STREQ("value3", celix_properties_getString(props, "object1/object2"));
    EXPECT_STREQ("value5", celix_properties_getString(props, "key3"));
    EXPECT_STREQ("value7", celix_properties_getString(props, "object3/key4"));

    // When decoding the properties from a string using a flag that allows duplicates
    celix_autoptr(celix_properties_t) props2 = nullptr;
    status = celix_properties_loadFromString2(jsonInput, CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES, &props2);

    // Then loading fails, because of a duplicate key
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");

    // When decoding the properties from a string using a flag that allows collisions
    celix_autoptr(celix_properties_t) props3 = nullptr;
    status = celix_properties_loadFromString2(jsonInput, CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS, &props3);

    // Then loading fails, because of a collision
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithAndWithoutStrictFlagTest) {
    auto invalidInputs = {
        R"({"mixedArr":["string", true]})", // Mixed array gives error on strict
        R"({"mixedVersionAndStringArr":["version<1.2.3.qualifier>","2.3.4"]})", // Mixed array gives error on strict
        R"({"key1":null})",                 // Null value gives error on strict
        R"({"":"value"})",                  // "" key gives error on strict
        R"({"emptyArr":[]})",               // Empty array gives error on strict
        R"({"key1":"val1", "key1":"val2"})",// Duplicate key gives error on strict
        R"({"nullArr":[null,null]})",       // Array with null values gives error on strict
    };

    for (auto& invalidInput: invalidInputs) {
        //Given an invalid JSON object
        FILE* stream = fmemopen((void*)invalidInput, strlen(invalidInput), "r");

        //When loading the properties from the stream with an empty flags
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_loadFromStream(stream, 0, &props);

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

        //When loading the properties from the stream with a strict flag
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_loadFromStream(stream, CELIX_PROPERTIES_DECODE_STRICT, &props);

        //Then decoding fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stderr, "Error: ", "\n");

        fclose(stream);
    }
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithSlashesInTheKeysTest) {
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

    // When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, 0, &props);
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

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithInvalidVersionsTest) {
    // Given a JSON object with an invalid version string (<, > not allowed in qualifier)
    const auto* jsonInput = R"( {"key":"version<1.2.3.<qualifier>>"} )";

    // When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromString2(jsonInput, 0, &props);

    // Then loading fails
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Error: ", "\n");

    // Given a JSON object with an invalid version strings, that are not recognized as versions
    jsonInput =
        R"( {"key1":"version<1.2.3", "key2":"version1.2.3>", "key3":"ver<1.2.3>}", "key4":"celix_version<1.2.3>"} )";

    // When loading the properties from the stream
    status = celix_properties_loadFromString2(jsonInput, 0, &props);

    // Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // But the values are not recognized as versions
    EXPECT_NE(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "key1"));
    EXPECT_NE(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "key2"));
    EXPECT_NE(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "key3"));
    EXPECT_NE(CELIX_PROPERTIES_VALUE_TYPE_VERSION, celix_properties_getType(props, "key4"));
}

TEST_F(PropertiesSerializationTestSuite, LoadWithInvalidStreamTest) {
    celix_properties_t* dummyProps = nullptr;

    // Loading properties with invalid stream will fail
    auto status = celix_properties_load2("non_existing_file.json", 0, &dummyProps);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, status);
    EXPECT_EQ(1, celix_err_getErrorCount());

    char* buf = nullptr;
    size_t size = 0;
    FILE* stream = open_memstream(&buf, &size); // empty stream
    status = celix_properties_loadFromStream(stream, 0, &dummyProps);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    EXPECT_EQ(2, celix_err_getErrorCount());

    fclose(stream);
    free(buf);
    celix_err_printErrors(stderr, "Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SaveAndLoadFlatProperties) {
    // Given a properties object with all possible types (but no empty arrays)
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "strKey", "strValue");
    celix_properties_setLong(props, "longKey", 42);
    celix_properties_setDouble(props, "doubleKey", 2.0);
    celix_properties_setBool(props, "boolKey", true);
    celix_properties_setVersion(props, "versionKey", celix_version_create(1, 2, 3, "qualifier"));
    auto* strArr = celix_arrayList_createStringArray();
    celix_arrayList_addString(strArr, "value1");
    celix_arrayList_addString(strArr, "value2");
    auto* longArr = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longArr, 1);
    celix_arrayList_addLong(longArr, 2);
    celix_properties_assignArrayList(props, "longArr", longArr);
    auto* doubleArr = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleArr, 1.0);
    celix_arrayList_addDouble(doubleArr, 2.0);
    celix_properties_assignArrayList(props, "doubleArr", doubleArr);
    auto* boolArr = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolArr, true);
    celix_arrayList_addBool(boolArr, false);
    celix_properties_assignArrayList(props, "boolArr", boolArr);
    auto* versionArr = celix_arrayList_createVersionArray();
    celix_arrayList_addVersion(versionArr, celix_version_create(1, 2, 3, "qualifier"));
    celix_arrayList_addVersion(versionArr, celix_version_create(4, 5, 6, "qualifier"));
    celix_properties_assignArrayList(props, "versionArr", versionArr);

    // When saving the properties to a properties_test.json file
    const char* filename = "properties_test.json";
    auto status = celix_properties_save(props, filename, 0);

    // Then saving succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // When loading the properties from the properties_test.json file
    celix_autoptr(celix_properties_t) loadedProps = nullptr;
    status = celix_properties_load2(filename, 0, &loadedProps);

    // Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the loaded properties are equal to the original properties
    EXPECT_TRUE(celix_properties_equals(props, loadedProps));
}
