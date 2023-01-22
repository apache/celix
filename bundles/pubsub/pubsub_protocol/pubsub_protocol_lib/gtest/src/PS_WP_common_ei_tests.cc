/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */



#include <gtest/gtest.h>
#include <iostream>
#include <cstring>
#include <celix_properties_ei.h>
#include <malloc_ei.h>

#include "pubsub_wire_protocol_common.h"

class WireProtocolCommonEiTest : public ::testing::Test {
public:
    WireProtocolCommonEiTest() = default;
    ~WireProtocolCommonEiTest() override {
        celix_ei_expect_realloc(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
    };
};

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_EncodeMetadataWithNoMemoryLeft) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = celix_properties_create();
    celix_properties_set(message.metadata.metadata, "key1", "value1");

    //Scenario: No mem with no pre-allocated data
    //Given (mocked) realloc is forced to return NULL
    celix_ei_expect_realloc((void *)pubsubProtocol_encodeMetadata, 0, nullptr);

    //When I try to encode a metadata
    char *data = nullptr;
    size_t length = 0;
    size_t contentLength = 0;
    auto status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    //Then I expect a failure
    EXPECT_NE(status, CELIX_SUCCESS);

    //Scenario: No mem with some pre-allocated data
    //Given a data set with some space
    data = (char*)malloc(16);
    length = 16;

    //And (mocked) realloc is forced to return NULL
    celix_ei_expect_realloc((void *)pubsubProtocol_encodeMetadata, 0, nullptr);

    //When I try to encode a metadata
    status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);

    //Then I expect a failure
    EXPECT_NE(status, CELIX_SUCCESS);

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DecodeMetadataWithSingleEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,"); //note 1 entry
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 1);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(1, celix_properties_size(message.metadata.metadata));
    EXPECT_STREQ("value1", celix_properties_get(message.metadata.metadata, "key1", "not-found"));

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DencodeMetadataWithMultipleEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,4:key2,6:value2,6:key111,8:value111,"); //note 3 entries
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 3);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(3, celix_properties_size(message.metadata.metadata));
    EXPECT_STREQ("value1", celix_properties_get(message.metadata.metadata, "key1", "not-found"));
    EXPECT_STREQ("value2", celix_properties_get(message.metadata.metadata, "key2", "not-found"));
    EXPECT_STREQ("value111", celix_properties_get(message.metadata.metadata, "key111", "not-found"));

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DencodeMetadataWithExtraEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,4:key2,6:value2,6:key111,8:value111,"); //note 3 entries
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 2);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);
    // the 3rd entry should be ignored
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(2, celix_properties_size(message.metadata.metadata));
    EXPECT_STREQ("value1", celix_properties_get(message.metadata.metadata, "key1", "not-found"));
    EXPECT_STREQ("value2", celix_properties_get(message.metadata.metadata, "key2", "not-found"));
    EXPECT_STREQ("not-found", celix_properties_get(message.metadata.metadata, "key111", "not-found"));

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DencodeMetadataMissingEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,4:key2,6:value2,6:key111,8:value111,"); //note 3 entries
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 4);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

    EXPECT_EQ(status, CELIX_INVALID_SYNTAX);
    EXPECT_EQ(nullptr, message.metadata.metadata);

    free(data);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DecodeMetadataWithSingleEntryWithIncompleteValue) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:val,"); //note 1 entry with short value
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 1);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

    EXPECT_EQ(status, CELIX_INVALID_SYNTAX);
    EXPECT_EQ(nullptr, message.metadata.metadata);

    free(data);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DecodeMetadataTooShort) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,"); //note 1 entry
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 1);
    auto status = pubsubProtocol_decodeMetadata((void*)data, 3, &message); // not enough data for `nOfElements`

    EXPECT_EQ(status, CELIX_INVALID_SYNTAX);
    EXPECT_EQ(nullptr, message.metadata.metadata);

    free(data);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_DecodeEmptyMetadata) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    uint32_t data = 0;
    auto status = pubsubProtocol_decodeMetadata((void*)&data, 4, &message);

    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(nullptr, message.metadata.metadata);

    // incorrect `nOfElements`
    data = 4;
    status = pubsubProtocol_decodeMetadata((void*)&data, 4, &message);

    EXPECT_EQ(status, CELIX_INVALID_SYNTAX);
    EXPECT_EQ(nullptr, message.metadata.metadata);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_NotEnoughMemoryForMultipleEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,4:key2,6:value2,6:key111,8:value111,"); //note 3 entries
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 3);
    for (int i = 0; i < 6; ++i) {
        celix_ei_expect_calloc((void *)pubsubProtocol_decodeMetadata, 0, nullptr, i+1);
        auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

        EXPECT_EQ(status, CELIX_ENOMEM);
        EXPECT_EQ(nullptr, message.metadata.metadata);
    }
    free(data);
}

TEST_F(WireProtocolCommonEiTest, WireProtocolCommonTest_PropertiesAllocFailureWhenDecodingMetadata) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,"); //note 1 entry
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 1);
    celix_ei_expect_celix_properties_create((void*)pubsubProtocol_decodeMetadata, 0, nullptr);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_EQ(nullptr, message.metadata.metadata);

    free(data);
}
