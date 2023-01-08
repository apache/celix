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
#include <dlfcn.h>

#include "pubsub_wire_protocol_common.h"

class WireProtocolCommonTest : public ::testing::Test {
public:
    WireProtocolCommonTest() = default;
    ~WireProtocolCommonTest() override = default;
};

#ifdef ENABLE_MALLOC_RETURN_NULL_TESTS
/**
 * If set to true the mocked malloc will always return NULL.
 * Should be read/written using __atomic builtins.
 */
static int mallocFailAfterCalls = 0;
static int mallocCurrentCallCount = 0;

/**
 * mocked malloc to ensure testing can be done for the "malloc returns NULL" scenario
 */
extern "C" void* malloc(size_t size) {
    int count = __atomic_add_fetch(&mallocCurrentCallCount, 1, __ATOMIC_ACQ_REL);
    int target = __atomic_load_n(&mallocFailAfterCalls, __ATOMIC_ACQUIRE);
    if (target > 0 && count == target) {
        return nullptr;
    }
    static auto* orgMallocFp = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    if (orgMallocFp == nullptr) {
        perror("Cannot find malloc symbol");
        return nullptr;
    }
    return orgMallocFp(size);
}

extern "C" void* realloc(void* buf, size_t newSize) {
    int count = __atomic_add_fetch(&mallocCurrentCallCount, 1, __ATOMIC_ACQ_REL);
    int target = __atomic_load_n(&mallocFailAfterCalls, __ATOMIC_ACQUIRE);
    if (target > 0 && count == target) {
        return nullptr;
    }
    static auto* orgReallocFp = (void*(*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
    if (orgReallocFp == nullptr) {
        perror("Cannot find realloc symbol");
        return nullptr;
    }
    return orgReallocFp(buf, newSize);
}

void setupMallocFailAfterNrCalls(int nrOfCalls) {
    __atomic_store_n(&mallocFailAfterCalls, nrOfCalls, __ATOMIC_RELEASE);
    __atomic_store_n(&mallocCurrentCallCount, 0, __ATOMIC_RELEASE);
}

void disableMallocFail() {
    __atomic_store_n(&mallocFailAfterCalls, 0, __ATOMIC_RELEASE);
}
#endif

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

#ifdef ENABLE_MALLOC_RETURN_NULL_TESTS
TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_EncodeMetadataWithNoMemoryLeft) {
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = celix_properties_create();
    celix_properties_set(message.metadata.metadata, "key1", "value1");

    //Scenario: No mem with no pre-allocated data
    //Given (mocked) malloc is forced to return NULL
    setupMallocFailAfterNrCalls(1);

    //When I try to encode a metadata
    char *data = nullptr;
    size_t length = 0;
    size_t contentLength = 0;
    auto status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);
    //Then I expect a failure
    EXPECT_NE(status, CELIX_SUCCESS);

    //reset malloc
    disableMallocFail();

    //Scenario: No mem with some pre-allocated data
    //Given a data set with some space
    data = (char*)malloc(16);
    length = 16;

    //And (mocked) malloc is forced to return NULL
    setupMallocFailAfterNrCalls(1);

    //When I try to encode a metadata
    status = pubsubProtocol_encodeMetadata(&message, &data, &length, &contentLength);

    //Then I expect a failure
    EXPECT_NE(status, CELIX_SUCCESS);

    //reset malloc
    disableMallocFail();

    free(data);
    celix_properties_destroy(message.metadata.metadata);
}
#endif

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

