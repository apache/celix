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

#include "celix_err.h"
#include "celix_properties.h"

using ::testing::MatchesRegex;

class PropertiesSerializationTestSuite : public ::testing::Test {
  public:
    PropertiesSerializationTestSuite() { celix_err_resetErrors(); }
};

TEST_F(PropertiesSerializationTestSuite, SaveEmptyPropertiesTest) {
    //Given an empty properties object
    celix_autoptr(celix_properties_t) props = celix_properties_create();

    //And an in-memory stream
    char* buf = nullptr;
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
