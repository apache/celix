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
#include <jansson.h>

#include "celix/Properties.h"
#include "celix_err.h"
#include "celix_properties.h"
#include "celix_properties_private.h"
#include "celix_stdlib_cleanup.h"


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

        // Then saving the properties to a string succeeds, but value is not added to the JSON (because JSON does not
        // support NAN, INF and -INF)
        celix_autofree char* output;
        auto status = celix_properties_saveToString(props, 0, &output);
        ASSERT_EQ(CELIX_SUCCESS, status);
        EXPECT_STREQ("{}", output);

        //And saving the properties to a string with the flag CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF fails
        celix_err_resetErrors();
        char* output2;
        status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF, &output2);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        //And an error msg is added to celix_err
        EXPECT_EQ(1, celix_err_getErrorCount());
    }
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithArrayListsContainingNaNAndInfValueTest) {
    auto keys = {"NAN", "INF", "-INF"};
    for (const auto& key : keys) {
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_autoptr(celix_array_list_t) list = celix_arrayList_createDoubleArray();
        celix_arrayList_addDouble(list, strtod(key, nullptr));
        celix_properties_assignArrayList(props, key, celix_steal_ptr(list));

        // Then saving the properties to a string succeeds, but value is not added to the JSON (because JSON does not
        // support NAN, INF and -INF)
        celix_autofree char* output;
        auto status = celix_properties_saveToString(props, 0, &output);
        ASSERT_EQ(CELIX_SUCCESS, status);
        EXPECT_STREQ("{}", output);

        //And saving the properties to a string with the flag CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF fails
        celix_err_resetErrors();
        char* output2;
        status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_ERROR_ON_NAN_INF, &output2);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        //And an error msg is added to celix_err
        EXPECT_EQ(2, celix_err_getErrorCount());

        celix_err_resetErrors();
        char* output3;
        status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS, &output3);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
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
    celix_arrayList_assignVersion(list5, celix_version_create(1, 2, 3, "qualifier"));
    celix_arrayList_assignVersion(list5, celix_version_create(4, 5, 6, "qualifier"));
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
    celix_autofree char* output1;
    auto status = celix_properties_saveToString(props, 0, &output1);

    //Then the save went ok
    ASSERT_EQ(CELIX_SUCCESS, status);

    //And the output contains an empty JSON object, because empty arrays are treated as unset
    EXPECT_STREQ("{}", output1);

    //When saving the properties to a string with an error on  empty array flag
    char* output2;
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_ERROR_ON_EMPTY_ARRAYS, &output2);

    //Then the save fails, because the empty array generates an error
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SaveEmptyKeyTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setString(props, "", "value");

    celix_autofree char* output1;
    auto status = celix_properties_saveToString(props, 0, &output1);
    ASSERT_EQ(CELIX_SUCCESS, status);

    celix_autoptr(celix_properties_t) prop2 = nullptr;
    status = celix_properties_loadFromString2(output1, 0, &prop2);
    ASSERT_EQ(CELIX_SUCCESS, status);

    ASSERT_TRUE(celix_properties_equals(props, prop2));
}

TEST_F(PropertiesSerializationTestSuite, SaveJSONPathKeysTest) {
    //Given a properties object with jpath keys
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_set(props, "object1.key3", "value3");
    celix_properties_set(props, "object1.key4", "value4");
    celix_properties_set(props, "object2.key5", "value5");
    celix_properties_set(props, "object3.object4.key6", "value6");

    //And an in-memory stream
    celix_autofree char* output;

    //When saving the properties to the stream
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the stream contains the JSON representation snippets of the properties
    EXPECT_NE(nullptr, strstr(output, R"("key1":"value1")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("key2":"value2")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("object1":{"key3":"value3","key4":"value4"})")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("object2":{"key5":"value5"})")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("object3":{"object4":{"key6":"value6"}})")) << "JSON: " << output;

    //And the buf is a valid JSON object
    json_error_t error;
    json_auto_t* root = json_loads(output, 0, &error);
    EXPECT_NE(nullptr, root) << "Unexpected JSON error: " << error.text;
}

TEST_F(PropertiesSerializationTestSuite, SaveJPathKeysWithCollisionTest) {
    // note this tests depends on the key iteration order for properties and
    // properties key order is based on hash order of the keys, so this test can change if the string hash map
    // implementation changes.

    //Given a properties object with jpath keys that collide
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1.key2.key3", "value1");
    celix_properties_set(props, "key1.key2", "value2"); //collision with object "key1/key2/key3" -> overwrite
    celix_properties_set(props, "key4.key5.key6.key7", "value4");
    celix_properties_set(props, "key4.key5.key6", "value3"); //collision with field "key4/key5/key6/key7" -> overwrite

    //When saving the properties to a string
    celix_autofree char* output = nullptr;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &output);
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

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithNestedEndErrorOnCollisionsFlagsTest) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");
    celix_properties_set(props, "object1.key3", "value3");
    celix_properties_set(props, "object1.key4", "value4");
    celix_properties_set(props, "object2.key5", "value5");
    celix_properties_set(props, "object3.object4.key6", "value6");

    // And an in-memory stream
    celix_autofree char* output;

    // When saving the properties to the stream
    auto status = celix_properties_saveToString(
        props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE | CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithKeyNamesWithDotsTest) {
    //Given a properties set with key names with dots
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "a.key.name.with.dots", "value1");
    celix_properties_set(props, ".keyThatStartsWithDot", "value3");
    celix_properties_set(props, "keyThatEndsWithDot.", "value5");
    celix_properties_set(props, "keyThatEndsWithDoubleDots..", "value6");
    celix_properties_set(props, "key..With..Double..Dots", "value7");
    celix_properties_set(props, "object.keyThatEndsWithDot.", "value8");
    celix_properties_set(props, "object.keyThatEndsWithDoubleDots..", "value9");
    celix_properties_set(props, "object.key..With..Double..Dots", "value10");


    //When saving the properties to a string
    celix_autofree char* output;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &output);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the out contains the JSON representation snippets of the properties
    EXPECT_NE(nullptr, strstr(output, R"("a":{"key":{"name":{"with":{"dots":"value1"}}}})")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("keyThatStartsWithDot":"value3")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value5")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value6")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("Dots":"value7")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value8")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("":"value9")")) << "JSON: " << output;
    EXPECT_NE(nullptr, strstr(output, R"("Dots":"value10")")) << "JSON: " << output;

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
    node = json_object_get(node, "Dots");
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
    celix_properties_set(props, "key1.key2.key3", "value1");
    celix_properties_set(props, "key1.key2", "value2"); //collision with object "key1.key2" -> overwrite

    //When saving the properties to a string
    celix_autofree char* output1;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &output1);

    //Then the save succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And both keys are serialized (one as a flat key) (flat key name is whitebox knowledge)
    EXPECT_NE(nullptr, strstr(output1, R"({"key1":{"key2":"value2"}})")) << "JSON: " << output1;

    //When saving the properties to a string with the error on key collision flag
    char* output2;
    status = celix_properties_saveToString(
        props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE | CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS, &output2);

    //Then the save fails, because the keys collide
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithAndWithoutStrictFlagTest) {
    //Given a properties set with an empty array list
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    auto* list = celix_arrayList_createStringArray();
    celix_properties_assignArrayList(props, "key1", list);

    //When saving the properties to a string without the strict flag
    celix_autofree char* output;
    auto status = celix_properties_saveToString(props, 0, &output);

    //Then the save succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    //When saving the properties to a string with the strict flag
    char* output2;
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_STRICT, &output2);

    //Then the save fails, because the empty array generates an error
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    //And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SavePropertiesWithPrettyPrintTest) {
    //Given a properties set with 2 keys
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key1", "value1");
    celix_properties_set(props, "key2", "value2");

    //When saving the properties to a string with pretty print
    celix_autofree char* output;
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

    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, SaveCxxPropertiesTest) {
    // Given a C++ Properties object with 2 keys
    celix::Properties props{};
    props.set("key1", "value1");
    props.set("key2", 42);
    props.setVector("key3", std::vector<bool>{}); // empty vector

    // When saving the properties to a string
    std::string result = props.saveToString();

    // Then the result contains the JSON representation snippets of the properties
    EXPECT_NE(std::string::npos, result.find("\"key1\":\"value1\""));
    EXPECT_NE(std::string::npos, result.find("\"key2\":42"));

    // When saving the properties to a string using a flat style
    std::string result2 = props.saveToString(celix::Properties::EncodingFlags::FlatStyle);

    //The result is equals to a default save
    EXPECT_EQ(result, result2);

    // When saving the properties to a string using an errors on duplicate key flag
    EXPECT_THROW(props.saveToString(celix::Properties::EncodingFlags::Strict),
                 celix::IllegalArgumentException);

    // When saving the properties to a string using combined flags
    EXPECT_THROW(props.saveToString(
                     celix::Properties::EncodingFlags::Pretty | celix::Properties::EncodingFlags::ErrorOnEmptyArrays |
                     celix::Properties::EncodingFlags::ErrorOnCollisions |
                     celix::Properties::EncodingFlags::ErrorOnNanInf | celix::Properties::EncodingFlags::NestedStyle),
                 celix::IllegalArgumentException);

    // When saving the properties to an invalid filename location
    EXPECT_THROW(props.save("/non-existing/no/rights/file.json"),
                     celix::IOException);
}

TEST_F(PropertiesSerializationTestSuite, LoadEmptyPropertiesTest) {
    //Given an empty JSON object
    const char* json = "{}";

    //When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromString2(json, 0, &props);
    ASSERT_EQ(CELIX_SUCCESS, status);

    //Then the properties object is empty
    EXPECT_EQ(0, celix_properties_size(props));
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
        EXPECT_NE(CELIX_SUCCESS, status);

        //And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_printErrors(stderr, "Test Error: ", "\n");

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
    celix_properties_t* props2;
    status = celix_properties_loadFromString2(inputJSON, CELIX_PROPERTIES_DECODE_ERROR_ON_EMPTY_ARRAYS, &props2);

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
    EXPECT_STREQ("value3", celix_properties_getString(props, "object1.key3"));
    EXPECT_EQ(true, celix_properties_getBool(props, "object1.key4", false));
    EXPECT_DOUBLE_EQ(5., celix_properties_getDouble(props, "object2.key5", 0.0));
    EXPECT_EQ(6, celix_properties_getLong(props, "object3.object4.key6", 0));
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
    celix_properties_t* props2;
    status = celix_properties_loadFromString2(jsonInput, CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES, &props2);

    // Then loading fails, because of a duplicate key
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesEscapedDotsTest) {
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
            "object2.key2": "value2"
        },
        "object1.object2" : "value3",
        "key3": "value4",
        "key3": "value5",
        "object3.key4": "value6",
        "object3": {
            "key4": "value7"
        }
    })";

    // When loading the properties from a string.
    celix_autoptr(celix_properties_t) props;
    auto status = celix_properties_loadFromString2(jsonInput, 0, &props);

    // Then loading succeeds
    ASSERT_EQ(CELIX_SUCCESS, status);

    // And the properties object all the values for the colliding keys and a single (latest) value for the duplicate
    // keys
    EXPECT_EQ(5, celix_properties_size(props));
    EXPECT_STREQ("value1", celix_properties_getString(props, "object1.object2.key1"));
    EXPECT_STREQ("value2", celix_properties_getString(props, "object1.object2.key2"));
    EXPECT_STREQ("value3", celix_properties_getString(props, "object1.object2"));
    EXPECT_STREQ("value5", celix_properties_getString(props, "key3"));
    EXPECT_STREQ("value7", celix_properties_getString(props, "object3.key4"));

    // When decoding the properties from a string using a flag that allows duplicates
    celix_properties_t* props2;
    status = celix_properties_loadFromString2(jsonInput, CELIX_PROPERTIES_DECODE_ERROR_ON_DUPLICATES, &props2);

    // Then loading fails, because of a duplicate key
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Test Error: ", "\n");

    // When decoding the properties from a string using a flag that allows collisions
    celix_properties_t* props3;
    status = celix_properties_loadFromString2(jsonInput, CELIX_PROPERTIES_DECODE_ERROR_ON_COLLISIONS, &props3);

    // Then loading fails, because of a collision
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    // And at least one error message is added to celix_err
    EXPECT_GE(celix_err_getErrorCount(), 1);
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithAndWithoutStrictFlagTest) {
    auto invalidInputs = {
        R"({"mixedArr":["string", true]})", // Mixed array gives error on strict
        R"({"mixedVersionAndStringArr":["version<1.2.3.qualifier>","2.3.4"]})", // Mixed array gives error on strict
        R"({"key1":null})",                 // Null value gives error on strict
        R"({"":"value"})",                  // "" key gives error on strict
        R"({"emptyArr":[]})",               // Empty array gives error on strict
        R"({"key1":"val1", "key1":"val2"})"// Duplicate key gives error on strict
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
        celix_err_printErrors(stderr, "Test Error: ", "\n");

        fclose(stream);
    }
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithUnsupportedArrayTypesTest) {
    auto invalidArrays = {
        R"({"objArray":[{"obj1": true}, {"obj2": true}]})", // Array with objects not supported.
        R"({"arrayArray":[[1,2], [2,4]]})",                  // Array with array not supported.
        R"({"nullArr":[null,null]})"       // Array with null values gives error on strict
    };

    // Decode with no strict flag, will ignore the unsupported arrays
    for (auto& invalidArray : invalidArrays) {
        // When loading the properties from the string
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_loadFromString2(invalidArray, 0, &props);

        // Then decoding succeeds, because strict is disabled
        ASSERT_EQ(CELIX_SUCCESS, status);
        EXPECT_GE(celix_err_getErrorCount(), 0);

        // But the properties size is 0, because the all invalid inputs are ignored
        EXPECT_EQ(0, celix_properties_size(props));
    }

    // Decode with strict flag, will fail on unsupported arrays
    for (auto& invalidArray : invalidArrays) {
        // When loading the properties from the string
        celix_autoptr(celix_properties_t) props = nullptr;
        auto status = celix_properties_loadFromString2(invalidArray, CELIX_PROPERTIES_DECODE_STRICT, &props);

        // Then decoding fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        // And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        celix_err_resetErrors();

        // When loading the properties from the CELIX_PROPERTIES_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS flag
        celix_properties_t* props2;
        status = celix_properties_loadFromString2(
            invalidArray, CELIX_PROPERTIES_DECODE_ERROR_ON_UNSUPPORTED_ARRAYS, &props2);

        // Then decoding fails
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        // And at least one error message is added to celix_err
        EXPECT_GE(celix_err_getErrorCount(), 1);
        //celix_err_resetErrors();
        celix_err_printErrors(stderr, "Test Error: ", "\n");
    }
}

TEST_F(PropertiesSerializationTestSuite, LoadPropertiesWithDotsInTheKeysTest) {
    // Given a complex JSON object
    const char* jsonInput = R"({
        ".": "value1",
        "keyThatEndsWithDots.": "value2",
        "key..With..Double..Dots": "value3",
        "object": {
            ".": "value4",
            "keyThatEndsWithDot.": "value5",
            "key..With..Double..Dot": "value6"
        }
    })";

    // And a stream with the JSON object
    FILE* stream = fmemopen((void*)jsonInput, strlen(jsonInput), "r");

    // When loading the properties from the stream
    celix_autoptr(celix_properties_t) props = nullptr;
    auto status = celix_properties_loadFromStream(stream, 0, &props);
    celix_err_printErrors(stderr, "Test Error: ", "\n");
    ASSERT_EQ(CELIX_SUCCESS, status);

    // Then the properties object contains the nested objects
    EXPECT_EQ(6, celix_properties_size(props));
    EXPECT_STREQ("value1", celix_properties_getString(props, "."));
    EXPECT_STREQ("value2", celix_properties_getString(props, "keyThatEndsWithDots."));
    EXPECT_STREQ("value3", celix_properties_getString(props, "key..With..Double..Dots"));
    EXPECT_STREQ("value4", celix_properties_getString(props, "object.."));
    EXPECT_STREQ("value5", celix_properties_getString(props, "object.keyThatEndsWithDot."));
    EXPECT_STREQ("value6", celix_properties_getString(props, "object.key..With..Double..Dot"));
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
    celix_err_printErrors(stderr, "Test Error: ", "\n");

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
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesSerializationTestSuite, LoadCxxPropertiesTest) {
    // Given a JSON object
    auto jsonInput = R"({"key1":"value1","key2":42,"key2":43})"; // note duplicate key3

    // When loading the properties from the JSON object
    auto props = celix::Properties::loadFromString(jsonInput);

    // Then the properties object contains the values
    EXPECT_EQ(2, props.size());
    EXPECT_STREQ("value1", props.get("key1").c_str());
    EXPECT_EQ(43, props.getAsLong("key2", -1));

    // When loading the properties from the JSON object with a strict flag
    EXPECT_THROW(celix::Properties::loadFromString(jsonInput, celix::Properties::DecodeFlags::Strict),
                 celix::IllegalArgumentException);

    // When loading the properties from the JSON object with a flag combined
    EXPECT_THROW(
        celix::Properties::loadFromString(
            jsonInput,
            celix::Properties::DecodeFlags::ErrorOnCollisions | celix::Properties::DecodeFlags::ErrorOnDuplicates |
                celix::Properties::DecodeFlags::ErrorOnEmptyArrays | celix::Properties::DecodeFlags::ErrorOnEmptyKeys |
                celix::Properties::DecodeFlags::ErrorOnUnsupportedArrays |
                celix::Properties::DecodeFlags::ErrorOnNullValues | celix::Properties::DecodeFlags::ErrorOnNullValues),
        celix::IllegalArgumentException);

    EXPECT_THROW(celix::Properties::load2("non_existing_file.json"), celix::IOException);
}

TEST_F(PropertiesSerializationTestSuite, SaveAndLoadFlatProperties) {
    // Given a properties object with all possible types (but no empty arrays)
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    celix_properties_set(props, "single/strKey", "strValue");
    celix_properties_setLong(props, "single/longKey", 42);
    celix_properties_setDouble(props, "single/doubleKey", 2.0);
    celix_properties_setBool(props, "single/boolKey", true);
    celix_properties_assignVersion(props, "single/versionKey", celix_version_create(1, 2, 3, "qualifier"));

    celix_array_list_t* strArr = celix_arrayList_createStringArray();
    celix_arrayList_addString(strArr, "value1");
    celix_arrayList_addString(strArr, "value2");
    celix_properties_assignArrayList(props, "array/stringArr", strArr);

    celix_array_list_t* longArr = celix_arrayList_createLongArray();
    celix_arrayList_addLong(longArr, 1);
    celix_arrayList_addLong(longArr, 2);
    celix_properties_assignArrayList(props, "array/longArr", longArr);

    celix_array_list_t* doubleArr = celix_arrayList_createDoubleArray();
    celix_arrayList_addDouble(doubleArr, 1.0);
    celix_arrayList_addDouble(doubleArr, 2.0);
    celix_properties_assignArrayList(props, "array/doubleArr", doubleArr);

    celix_array_list_t* boolArr = celix_arrayList_createBoolArray();
    celix_arrayList_addBool(boolArr, true);
    celix_arrayList_addBool(boolArr, false);
    celix_properties_assignArrayList(props, "array/boolArr", boolArr);

    celix_array_list_t* versionArr = celix_arrayList_createVersionArray();
    celix_arrayList_assignVersion(versionArr, celix_version_create(1, 2, 3, "qualifier"));
    celix_arrayList_assignVersion(versionArr, celix_version_create(4, 5, 6, "qualifier"));
    celix_properties_assignArrayList(props, "array/versionArr", versionArr);

    // When saving the properties to a properties_test.json file
    const char* filename = "properties_test.json";
    auto status = celix_properties_save(props, filename, CELIX_PROPERTIES_ENCODE_PRETTY);

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

TEST_F(PropertiesSerializationTestSuite, SaveAndLoadCxxProperties) {
    //Given a filename
    std::string filename = "properties_test.json";

    //And a Properties object with 1 key
    celix::Properties props{};
    props.set("key1", "value1");

    //When saving the properties to the filename
    props.save(filename);

    //And reloading the properties from the filename
    auto props2 = celix::Properties::load2(filename);

    //Then the reloaded properties are equal to the original properties
    EXPECT_TRUE(props == props2);
}

TEST_F(PropertiesSerializationTestSuite, JsonErrorToCelixStatusTest) {
    EXPECT_EQ(CELIX_ILLEGAL_STATE, celix_properties_jsonErrorToStatus(json_error_unknown));

    EXPECT_EQ(ENOMEM, celix_properties_jsonErrorToStatus(json_error_out_of_memory));
    EXPECT_EQ(ENOMEM, celix_properties_jsonErrorToStatus(json_error_stack_overflow));

    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, celix_properties_jsonErrorToStatus(json_error_cannot_open_file));

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_invalid_argument));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_invalid_argument));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_premature_end_of_input));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_end_of_input_expected));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_invalid_syntax));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_invalid_format));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_wrong_type));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_null_character));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_null_value));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_null_byte_in_key));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_duplicate_key));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_numeric_overflow));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_item_not_found));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_properties_jsonErrorToStatus(json_error_index_out_of_range));
}

TEST_F(PropertiesSerializationTestSuite, KeyCollision) {
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    // pick keys such that key1 appears before key2 when iterating over the properties
    celix_properties_set(props, "a.b.haha.arbifdadfsfa", "value1");
    celix_properties_set(props, "a.b.haha", "value2");

    celix_autofree char* output = nullptr;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE | CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS,
                                                    &output);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    celix_autoptr(celix_properties_t) props2 = celix_properties_create();
    // pick keys such that key1 appears before key2 when iterating over the properties
    celix_properties_set(props2, "a.b.c", "value1");
    celix_properties_set(props2, "a.b.c.d", "value2");
    status = celix_properties_saveToString(props2, CELIX_PROPERTIES_ENCODE_NESTED_STYLE | CELIX_PROPERTIES_ENCODE_ERROR_ON_COLLISIONS,
                                           &output);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    status = celix_properties_saveToString(props2, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &output);
    // "a.b.c.d" is silently discarded
    EXPECT_STREQ(R"({"a":{"b":{"c":"value1"}}})", output);
    std::cout << output << std::endl;
    EXPECT_EQ(CELIX_SUCCESS, status);
}
