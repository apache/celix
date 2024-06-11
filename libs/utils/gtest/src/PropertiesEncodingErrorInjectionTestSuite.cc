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

#include "celix_properties.h"
#include "celix/Properties.h"

#include "celix_err.h"
#include "celix_array_list_ei.h"
#include "celix_utils_ei.h"
#include "celix_version_ei.h"
#include "jansson_ei.h"
#include "malloc_ei.h"
#include "stdio_ei.h"

class PropertiesEncodingErrorInjectionTestSuite : public ::testing::Test {
  public:
    PropertiesEncodingErrorInjectionTestSuite() = default;

    ~PropertiesEncodingErrorInjectionTestSuite() override {
        celix_ei_expect_json_object(nullptr, 0, nullptr);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_json_object_set_new(nullptr, 0, -1);
        celix_ei_expect_json_sprintf(nullptr, 0, nullptr);
        celix_ei_expect_celix_version_toString(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_addString(nullptr, 0, CELIX_SUCCESS);
        celix_err_resetErrors();
    }
};

TEST_F(PropertiesEncodingErrorInjectionTestSuite, SaveErrorTest) {
    //Given a dummy properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key", "value");

    //When an error injected is prepared for json_object() from saveToStream
    celix_ei_expect_json_object((void*)celix_properties_saveToStream, 0, nullptr);

    //And a dummy stream is created
    FILE* stream = fopen("/dev/null", "w");

    //When I call celix_properties_saveToStream
    celix_status_t status = celix_properties_saveToStream(props, stream, 0);

    //Then I expect an error
    EXPECT_EQ(CELIX_ENOMEM, status);
    fclose(stream);

    //When an error injected is prepared for open_memstream()n from save
    celix_ei_expect_open_memstream((void*)celix_properties_saveToString, 0, nullptr);

    //When I call celix_properties_saveToString
    char* out;
    status = celix_properties_saveToString(props, 0, &out);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    //And I expect 2 error messages in celix_err
    EXPECT_EQ(2, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, FcloseErrorWhenSaveTest) {
    //Given a dummy properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key", "value");

    //When an error injected is prepared for fclose() from save
    celix_ei_expect_fclose((void*)celix_properties_save, 0, -1);

    //And I call celix_properties_save
    auto status = celix_properties_save(props, "somefile.json", 0);

    //Then I expect an error
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION, status);

    //And I expect 1 error message in celix_err
    EXPECT_EQ(1, celix_err_getErrorCount());
}


TEST_F(PropertiesEncodingErrorInjectionTestSuite, EncodeErrorTest) {
    // Given a dummy properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key.with.slash", "value");
    celix_properties_set(props, "key-with-out-slash", "value");

    // When an error injected is prepared for celix_utils_writeOrCreateString() from celix_properties_saveToString
    celix_ei_expect_celix_utils_writeOrCreateString((void*)celix_properties_saveToString, 2, nullptr);

    // And I call celix_properties_saveToString using NESTED encoding (whitebox-knowledge)
    char* out;
    auto status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // When an error injected is prepared for json_object() from celix_properties_saveToString
    celix_ei_expect_json_object((void*)celix_properties_saveToString, 2, nullptr);

    // And I call celix_properties_saveToString using NESTED encoding (whitebox-knowledge)
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // When an error injected is prepared for json_object_set_new() from celix_properties_saveToString
    celix_ei_expect_json_object_set_new((void*)celix_properties_saveToString, 2, -1);

    // And I call celix_properties_saveToString using NESTED encoding (whitebox-knowledge)
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // When an error injected is prepared for json_string() from celix_properties_saveToString
    celix_ei_expect_json_string((void*)celix_properties_saveToString, 3, nullptr);

    // And I call celix_properties_saveToString using NESTED encoding (whitebox-knowledge)
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_NESTED_STYLE, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // When an error injected is prepared for json_object_set_new() from celix_properties_saveToString
    celix_ei_expect_json_object_set_new((void*)celix_properties_saveToString, 3, -1);

    // And I call celix_properties_saveToString using FLAT encoding (whitebox-knowledge)
    status = celix_properties_saveToString(props, CELIX_PROPERTIES_ENCODE_FLAT_STYLE, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // And I expect 5 error message in celix_err
    EXPECT_EQ(5, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, EncodeArrayErrorTest) {
    // Given a dummy properties object with an array
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    auto* arr = celix_arrayList_createStringArray();
    celix_arrayList_addString(arr, "value1");
    celix_arrayList_addString(arr, "value2");
    celix_properties_assignArrayList(props, "key", arr);

    // When an error injected is prepared for json_array() from celix_properties_saveToString
    celix_ei_expect_json_array((void*)celix_properties_saveToString, 4, nullptr);

    // And I call celix_properties_saveToString
    char* out;
    auto status = celix_properties_saveToString(props, 0, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);


    //When an error injected is prepared for json_array_append_new() from loadFromString2
    celix_ei_expect_json_array_append_new((void*)celix_properties_saveToString, 4, -1);

    //And I call celix_properties_saveToString
    status = celix_properties_saveToString(props, 0, &out);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    //When an error injected is prepared for json_string() from loadFromString2
    celix_ei_expect_json_string((void*)celix_properties_saveToString, 5, nullptr);

    //And I call celix_properties_saveToString
    status = celix_properties_saveToString(props, 0, &out);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // And I expect 3 error message in celix_err
    EXPECT_EQ(4, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, EncodeVersionErrorTest) {
    // Given a dummy properties object with a version
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    auto* version = celix_version_create(1, 2, 3, "qualifier");
    celix_properties_assignVersion(props, "key", version);

    // When an error injected is prepared for json_sprintf() from celix_properties_saveToString
    celix_ei_expect_json_sprintf((void*)celix_properties_saveToString, 4, nullptr);

    // And I call celix_properties_saveToString
    char* out;
    auto status = celix_properties_saveToString(props, 0, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // When an error injected is prepared for celix_version_toString() from celix_properties_saveToString
    celix_ei_expect_celix_version_toString((void*)celix_properties_saveToString, 4, nullptr);

    // And I call celix_properties_saveToString
    status = celix_properties_saveToString(props, 0, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // And I expect 2 error message in celix_err
    EXPECT_EQ(2, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, EncodeDumpfErrorTest) {
    // Given a dummy properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, "key", "value");

    // When an error injected is prepared for json_dumpf() from celix_properties_saveToString
    celix_ei_expect_json_dumpf((void*)celix_properties_saveToStream, 0, -1);

    // And I call celix_properties_saveToString
    char* out;
    auto status = celix_properties_saveToString(props, 0, &out);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // And I expect 1 error message in celix_err
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, LoadErrorTest) {
    //Given a dummy json string
    const char* json = R"({"key":"value"})";

    //When an error injected is prepared for fmemopen() from loadFromString2
    celix_ei_expect_fmemopen((void*)celix_properties_loadFromString, 0, nullptr);

    //When I call celix_properties_loadFromString
    celix_properties_t* props;
    auto status = celix_properties_loadFromString(json, 0, &props);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    //And I expect 1 error message in celix_err
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, DecodeErrorTest) {
    //Given a dummy json string
    const char* json = R"({"key":"value", "object": {"key":"value"}})";

    //When an error injected is prepared for celix_properties_create()->malloc() from celix_properties_loadFromString2
    celix_ei_expect_malloc((void*)celix_properties_loadFromString, 3, nullptr);

    //When I call celix_properties_loadFromString
    celix_properties_t* props;
    auto status = celix_properties_loadFromString(json, 0, &props);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    //When an error injected is prepared for celix_utils_writeOrCreateString() from celix_properties_loadFromString2
    celix_ei_expect_celix_utils_writeOrCreateString((void*)celix_properties_loadFromString, 3, nullptr);

    //When I call celix_properties_loadFromString
    status = celix_properties_loadFromString(json, 0, &props);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    //And I expect 2 error message in celix_err
    EXPECT_EQ(2, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, DecodeArrayErrorTest) {
    //Given a dummy json string
    const char* json = R"({"key":["value1", "value2"]})";

    // When an error injected is prepared for celix_arrayList_createWithOptions() from celix_properties_loadFromString2
    celix_ei_expect_celix_arrayList_createWithOptions((void*)celix_properties_loadFromString, 4, nullptr);

    //When I call celix_properties_loadFromString
    celix_properties_t* props;
    auto status = celix_properties_loadFromString(json, 0, &props);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // When an error injected is prepared for celix_arrayList_addString() from celix_properties_loadFromString2
    celix_ei_expect_celix_arrayList_addString((void*)celix_properties_loadFromString, 4, ENOMEM);

    //When I call celix_properties_loadFromString
    status = celix_properties_loadFromString(json, 0, &props);

    //Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // And I expect 0 error message in celix_err. Note because errors are injected for celix_array_list_t, celix_err is
    // not used
    EXPECT_EQ(0, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, DecodeVersionErrorTest) {
    // Given a dummy json version string
    const char* json = R"({"key":"version<1.2.3.qualifier>"})";

    // When an error injected is prepared for celix_utils_writeOrCreateString() from celix_properties_loadFromString2
    celix_ei_expect_celix_utils_writeOrCreateString((void*)celix_properties_loadFromString, 4, nullptr);

    // And I call celix_properties_loadFromString
    celix_properties_t* props;
    auto status = celix_properties_loadFromString(json, 0, &props);

    // Then I expect an error
    EXPECT_EQ(ENOMEM, status);

    // And I expect 1 error message in celix_err
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_printErrors(stderr, "Test Error: ", "\n");
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, SaveCxxPropertiesErrorTest) {
    //Given a dummy Properties object
    celix::Properties props{};
    props.set("key", "value");

    //When an error injected is prepared for json_object() from saveToStream
    celix_ei_expect_json_object((void*)celix_properties_saveToStream, 0, nullptr);

    //Then saving to file throws a bad alloc exception
    EXPECT_THROW(props.save("somefile.json"), std::bad_alloc);

    //When an error injected is prepared for json_object() from saveToStream
    celix_ei_expect_json_object((void*)celix_properties_saveToStream, 0, nullptr);

    //Then saving to string throws a bad alloc exception
    EXPECT_THROW(props.saveToString(), std::bad_alloc);
}

TEST_F(PropertiesEncodingErrorInjectionTestSuite, LoadCxxPropertiesErrorTest) {
        //Given a dummy json string
        const char* json = R"({"key":"value"})";

        //When an error injected is prepared for malloc() from celix_properties_create
        celix_ei_expect_malloc((void*)celix_properties_create, 0, nullptr);

        //Then loading from string throws a bad alloc exception
        EXPECT_THROW(celix::Properties::loadFromString(json), std::bad_alloc);

        //When an error injected is prepared for malloc() from celix_properties_create
        celix_ei_expect_malloc((void*)celix_properties_create, 0, nullptr);

        //And an empty json file exists
        FILE* file = fopen("empty.json", "w");
        fprintf(file, "{}");
        fclose(file);

        //Then loading from file throws a bad alloc exception
        EXPECT_THROW(celix::Properties::load2("empty.json"), std::bad_alloc);
}
