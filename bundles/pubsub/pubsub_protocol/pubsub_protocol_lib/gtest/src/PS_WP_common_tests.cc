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

#include "pubsub_wire_protocol_common.h"

class WireProtocolCommonTest : public ::testing::Test {
public:
    WireProtocolCommonTest() = default;
    ~WireProtocolCommonTest() = default;
};

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_EncodeMetadataWithSingleEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = celix_properties_create();
    celix_properties_set(message.metadata.metadata, "key1", "value1");
    size_t neededNetstringLength = 16;  //4:key1,6:value1, (16 chars)

    char *data = nullptr;
    size_t length = 0;
    size_t contentLength = 0;
    celix_status_t status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    EXPECT_TRUE(status == CELIX_SUCCESS);
    EXPECT_EQ(contentLength, neededNetstringLength + 4);

    //note first 4 bytes are an int with the nr of metadata entries (6);
    uint32_t nrOfMetadataEntries;
    pubsubProtocol_readInt((unsigned char*)data, 0, message.header.convertEndianess, &nrOfMetadataEntries);
    EXPECT_EQ(1, nrOfMetadataEntries);
    EXPECT_GE(length, neededNetstringLength + 4 /*sizeof(nrOfMetadataEntries)*/);

    //create null terminated string from netstring entries in the data buffer (- first 4 bytes + null terminating entry)
    char checkStr[32];
    ASSERT_GE(sizeof(checkStr), neededNetstringLength);
    strncpy(checkStr, data+4, neededNetstringLength);
    checkStr[neededNetstringLength] = '\0';
    EXPECT_STREQ("4:key1,6:value1,", checkStr);

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_EncodeMetadataWithMultipleEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = celix_properties_create();
    celix_properties_set(message.metadata.metadata, "key1", "value1");      //4:key1,6:value1, (16 chars)
    celix_properties_set(message.metadata.metadata, "key2", "value2");      //4:key2,6:value2, (16 chars)
    celix_properties_set(message.metadata.metadata, "key3", "value3");      //4:key3,6:value3, (16 chars)
    celix_properties_set(message.metadata.metadata, "key4", "value4");      //4:key4,6:value4, (16 chars)
    celix_properties_set(message.metadata.metadata, "key5", "value5");      //4:key5,6:value5, (16 chars)
    celix_properties_set(message.metadata.metadata, "key111", "value111");  //6:key111,8:value111, (20 chars)
    size_t neededNetstringLength = 100;                                     //Total: 5 * 16 + 20 = 100


    char *data = nullptr;
    size_t length = 0;
    size_t contentLength = 0;
    celix_status_t status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    EXPECT_TRUE(status == CELIX_SUCCESS);
    EXPECT_EQ(contentLength, neededNetstringLength + 4);

    //note first 4 bytes are a int with the nr of metadata entries (6);
    uint32_t nrOfMetadataEntries;
    pubsubProtocol_readInt((unsigned char*)data, 0, message.header.convertEndianess, &nrOfMetadataEntries);
    EXPECT_EQ(6, nrOfMetadataEntries);


    //create null terminated string from netstring entries in the data buffer (- first 4 bytes + null terminating entry)
    char checkStr[128];
    ASSERT_GE(sizeof(checkStr), neededNetstringLength);
    strncpy(checkStr, data+4, neededNetstringLength);
    checkStr[neededNetstringLength] = '\0';

    //NOTE because celix properties can reorder entries (hashmap), check if the expected str parts are in the data.
    EXPECT_NE(nullptr, strstr(checkStr, "4:key1,6:value1,")); //entry 1
    EXPECT_NE(nullptr, strstr(checkStr, "4:key2,6:value2,")); //entry 2
    EXPECT_NE(nullptr, strstr(checkStr, "4:key3,6:value3,")); //entry 3
    EXPECT_NE(nullptr, strstr(checkStr, "4:key4,6:value4,")); //entry 4
    EXPECT_NE(nullptr, strstr(checkStr, "4:key5,6:value5,")); //entry 5
    EXPECT_NE(nullptr, strstr(checkStr, "6:key111,8:value111,")); //entry 6 (with value 111)

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_EncodeMetadataWithNullOrEmptyArguments) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    char *data = nullptr;
    size_t length = 0;
    size_t contentLength = 0;
    celix_status_t status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);

    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(contentLength, 4);

    //note first 4 bytes are a int with the nr of metadata entries (0);
    uint32_t nrOfMetadataEntries;
    pubsubProtocol_readInt((unsigned char*)data, 0, message.header.convertEndianess, &nrOfMetadataEntries);
    EXPECT_EQ(0, nrOfMetadataEntries);
    free(data);

    data = nullptr;
    length = 0;
    message.metadata.metadata = celix_properties_create(); //note empty

    status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(contentLength, 4);

    //note first 4 bytes are a int with the nr of metadata entries (0);
    pubsubProtocol_readInt((unsigned char*)data, 0, message.header.convertEndianess, &nrOfMetadataEntries);
    EXPECT_EQ(0, nrOfMetadataEntries);
    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_EncodeWithExistinBufferWhichAlsoNeedsSomeRealloc) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = nullptr;

    char *data = (char*)malloc(2);
    size_t length = 2;
    size_t contentLength = 0;
    celix_status_t status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_GT(length, 2);

    message.metadata.metadata = celix_properties_create();
    char key[32];
    //create a lot of metadata entries
    for (int i = 0; i < 1000; ++i) {
        snprintf(key, sizeof(key), "key%i", i);
        celix_properties_set(message.metadata.metadata, key, "abcdefghijklmnop");
    }
    //note reusing data
    status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    EXPECT_EQ(status, CELIX_SUCCESS);

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DecodeMetadataWithSingleEntries) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DencodeMetadataWithMultipleEntries) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DencodeMetadataWithExtraEntries) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DencodeMetadataMissingEntries) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DecodeMetadataWithSingleEntryWithIncompleteValue) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DecodeMetadataTooShort) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DecodeEmptyMetadata) {
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

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_DencodeMetadataWithDuplicateEntries) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 1;
    message.metadata.metadata = nullptr;

    char* data = strdup("ABCD4:key1,6:value1,4:key1,6:value2,6:key111,8:value111,"); //note 3 entries with duplicate key1
    auto len = strlen(data);
    pubsubProtocol_writeInt((unsigned char*)data, 0, message.header.convertEndianess, 3);
    auto status = pubsubProtocol_decodeMetadata((void*)data, len, &message);

    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(2, celix_properties_size(message.metadata.metadata));
    EXPECT_STREQ("value2", celix_properties_get(message.metadata.metadata, "key1", "not-found"));
    EXPECT_STREQ("value111", celix_properties_get(message.metadata.metadata, "key111", "not-found"));

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}
