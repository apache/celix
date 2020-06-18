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

#include <sstream>
#include <arpa/inet.h>

#include <pubsub_wire_protocol_common.h>

#include "gtest/gtest.h"

#include "pubsub_wire_protocol_impl.h"
#include <cstring>

class WireProtocolV1Test : public ::testing::Test {
public:
    WireProtocolV1Test() = default;
    ~WireProtocolV1Test() override = default;

};


TEST_F(WireProtocolV1Test, WireProtocolV1Test_EncodeHeader_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    pubsub_protocol_message_t message;
    message.header.msgId = 1;
    message.header.msgMajorVersion = 0;
    message.header.msgMinorVersion = 0;
    message.header.payloadSize = 2;
    message.header.metadataSize = 3;

    void *headerData = nullptr;
    size_t headerLength = 0;
    celix_status_t status = pubsubProtocol_encodeHeader(nullptr, &message, &headerData, &headerLength);

    unsigned char exp[24];
    uint32_t s = 0xABBABAAB;
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x01000000;
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000;
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+12, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+16, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+20, &ms, sizeof(uint32_t));

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(24, headerLength);
    for (int i = 0; i < 24; i++) {
        ASSERT_EQ(((unsigned char*) headerData)[i], exp[i]);
    }

    pubsubProtocol_destroy(wireprotocol);
    free(headerData);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_DecodeHeader_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[24];
    uint32_t s = 0xABBABAAB;
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x01000000;
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000;
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+12, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+16, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+20, &ms, sizeof(uint32_t));

    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_decodeHeader(nullptr, exp, 24, &message);

    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_EQ(1, message.header.msgId);
    ASSERT_EQ(0, message.header.msgMajorVersion);
    ASSERT_EQ(0, message.header.msgMinorVersion);
    ASSERT_EQ(2, message.header.payloadSize);
    ASSERT_EQ(3, message.header.metadataSize);

    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_WireProtocolV1Test_DecodeHeader_IncorrectSync_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[24];
    uint32_t s = 0xBAABABBA;
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x01000000;
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000;
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+12, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+16, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+20, &ms, sizeof(uint32_t));

    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_decodeHeader(nullptr, exp, 24, &message);

    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_WireProtocolV1Test_DecodeHeader_IncorrectVersion_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[24];
    uint32_t s = 0xABBABAAB;
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x02000000;
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000;
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+12, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+16, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+20, &ms, sizeof(uint32_t));

    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_decodeHeader(nullptr, exp, 24, &message);

    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_EncodeMetadata_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    pubsub_protocol_message_t message;

    message.metadata.metadata = celix_properties_create();
    celix_properties_set(message.metadata.metadata, "a", "b");

    void *data = nullptr;
    size_t length = 0;
    celix_status_t status = pubsubProtocol_v1_encodeMetadata(nullptr, &message, &data, &length);

    unsigned char exp[12];
    uint32_t s = htonl(1);
    memcpy(exp, &s, sizeof(uint32_t));
    memcpy(exp + 4, "1:a,1:b,", 8);

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(12, length);
    for (int i = 0; i < 12; i++) {
        ASSERT_EQ(((unsigned char*) data)[i], exp[i]);
    }

    celix_properties_destroy(message.metadata.metadata);
    free(data);
    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_DecodeMetadata_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[12];
    uint32_t s = htonl(1);
    memcpy(exp, &s, sizeof(uint32_t));
    memcpy(exp + 4, "1:a,1:b,", 8);

    pubsub_protocol_message_t message;
    celix_status_t status = pubsubProtocol_v1_decodeMetadata(nullptr, exp, 12, &message);

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(1, celix_properties_size(message.metadata.metadata));
    const char * value = celix_properties_get(message.metadata.metadata, "a", nullptr);
    ASSERT_STREQ("b", value);

    celix_properties_destroy(message.metadata.metadata);

    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_DecodeMetadata_EmptyKey_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[11];
    uint32_t s = htonl(1);
    memcpy(exp, &s, sizeof(uint32_t));
    memcpy(exp + 4, "0:,1:b,", 7);

    pubsub_protocol_message_t message;
    celix_status_t status = pubsubProtocol_v1_decodeMetadata(nullptr, exp, 11, &message);

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(1, celix_properties_size(message.metadata.metadata));
    const char * value = celix_properties_get(message.metadata.metadata, "", nullptr);
    ASSERT_STREQ("b", value);

    celix_properties_destroy(message.metadata.metadata);
    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_DecodeMetadata_SpecialChars_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[15];
    uint32_t s = htonl(1);
    memcpy(exp, &s, sizeof(uint32_t));
    memcpy(exp + 4, "4:a,:l,1:b,", 11);

    pubsub_protocol_message_t message;
    celix_status_t status = pubsubProtocol_v1_decodeMetadata(nullptr, &exp, 15, &message);

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(1, celix_properties_size(message.metadata.metadata));
    const char * value = celix_properties_get(message.metadata.metadata, "a,:l", nullptr);
    ASSERT_STREQ("b", value);

    celix_properties_destroy(message.metadata.metadata);
    pubsubProtocol_destroy(wireprotocol);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_EncodeFooter_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    pubsub_protocol_message_t message;

    void *footerData = nullptr;
    size_t footerLength = 0;
    celix_status_t status = pubsubProtocol_encodeFooter(nullptr, &message, &footerData, &footerLength);

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(0, footerLength);
    ASSERT_EQ(nullptr, footerData);
    pubsubProtocol_destroy(wireprotocol);
    free(footerData);
}

TEST_F(WireProtocolV1Test, WireProtocolV1Test_DecodeFooter_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v1_t *wireprotocol;
    pubsubProtocol_create(&wireprotocol);

    unsigned char exp[4];
    uint32_t s = 0xBAABABBA;
    memcpy(exp, &s, sizeof(uint32_t));
    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_decodeFooter(nullptr, exp, 4, &message);
    ASSERT_EQ(CELIX_SUCCESS, status);
    pubsubProtocol_destroy(wireprotocol);
}
