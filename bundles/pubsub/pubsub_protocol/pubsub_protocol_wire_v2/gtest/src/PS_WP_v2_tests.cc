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
#include <iostream>

#include <pubsub_wire_protocol_common.h>

#include "gtest/gtest.h"

#include "pubsub_wire_v2_protocol_impl.h"
#include "celix_byteswap.h"
#include <cstring>

class WireProtocolV2Test : public ::testing::Test {
public:
    WireProtocolV2Test() = default;
    ~WireProtocolV2Test() override = default;

};


TEST_F(WireProtocolV2Test, WireProtocolV2Test_EncodeHeader_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    pubsub_protocol_message_t message;
    message.header.msgId = 1;
    message.header.seqNr = 4;
    message.header.msgMajorVersion = 0;
    message.header.msgMinorVersion = 0;
    message.header.payloadSize = 2;
    message.header.metadataSize = 3;
    message.header.payloadPartSize = 4;
    message.header.payloadOffset = 2;
    message.header.isLastSegment = 1;
    message.header.convertEndianess = 1;

    void *headerData = nullptr;
    size_t headerLength = 0;
    celix_status_t status = pubsubProtocol_wire_v2_encodeHeader(nullptr, &message, &headerData, &headerLength);

    unsigned char exp[40];
    uint32_t s = bswap_32(0xABBADEAF);
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x02000000; //envelope version
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000; //msg id
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t seq = 0x04000000; //seqnr
    memcpy(exp+12, &seq, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+16, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+20, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+24, &ms, sizeof(uint32_t));
    uint32_t pps = 0x04000000;
    memcpy(exp+28, &pps, sizeof(uint32_t));
    uint32_t ppo = 0x02000000;
    memcpy(exp+32, &ppo, sizeof(uint32_t));
    uint32_t ils = 0x01000000;
    memcpy(exp+36, &ils, sizeof(uint32_t));

    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(40, headerLength);
    for (int i = 0; i < 40; i++) {
        ASSERT_EQ(((unsigned char*) headerData)[i], exp[i]);
    }

    pubsubProtocol_wire_v2_destroy(wireprotocol);
    free(headerData);
}

TEST_F(WireProtocolV2Test, WireProtocolV2Test_DecodeHeader_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    unsigned char exp[40];
    uint32_t s = bswap_32(0xABBADEAF); //sync
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x02000000; //envelope version
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000; //msg id
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t seq = 0x08000000; //seqnr
    memcpy(exp+12, &seq, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+16, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+20, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+24, &ms, sizeof(uint32_t));
    uint32_t pps = 0x04000000;
    memcpy(exp+28, &pps, sizeof(uint32_t));
    uint32_t ppo = 0x02000000;
    memcpy(exp+32, &ppo, sizeof(uint32_t));
    uint32_t ils = 0x01000000;
    memcpy(exp+36, &ils, sizeof(uint32_t));

    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_wire_v2_decodeHeader(nullptr, exp, 40, &message);

    ASSERT_EQ(CELIX_SUCCESS, status);
    ASSERT_EQ(1, message.header.msgId);
    ASSERT_EQ(8, message.header.seqNr);
    ASSERT_EQ(0, message.header.msgMajorVersion);
    ASSERT_EQ(0, message.header.msgMinorVersion);
    ASSERT_EQ(2, message.header.payloadSize);
    ASSERT_EQ(3, message.header.metadataSize);
    ASSERT_EQ(4, message.header.payloadPartSize);
    ASSERT_EQ(2, message.header.payloadOffset);
    ASSERT_EQ(1, message.header.isLastSegment);
    ASSERT_EQ(1, message.header.convertEndianess);

    pubsubProtocol_wire_v2_destroy(wireprotocol);
}

TEST_F(WireProtocolV2Test, WireProtocolV2Test_DecodeHeader_IncorrectSync_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    unsigned char exp[40];
    uint32_t s = 0xBAABABBA;
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x01000000;
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000;
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t seq = 0x08000000;
    memcpy(exp+12, &seq, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+16, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+20, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+24, &ms, sizeof(uint32_t));

    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_wire_v2_decodeHeader(nullptr, exp, 40, &message);

    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    pubsubProtocol_wire_v2_destroy(wireprotocol);
}

TEST_F(WireProtocolV2Test, WireProtocolV2Test_DecodeHeader_IncorrectVersion_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    unsigned char exp[40];
    uint32_t s = 0xABBADEAF;
    memcpy(exp, &s, sizeof(uint32_t));
    uint32_t e = 0x02000000;
    memcpy(exp+4, &e, sizeof(uint32_t));
    uint32_t m = 0x01000000;
    memcpy(exp+8, &m, sizeof(uint32_t));
    uint32_t seq = 0x08000000;
    memcpy(exp+12, &seq, sizeof(uint32_t));
    uint32_t v = 0x00000000;
    memcpy(exp+16, &v, sizeof(uint32_t));
    uint32_t ps = 0x02000000;
    memcpy(exp+20, &ps, sizeof(uint32_t));
    uint32_t ms = 0x03000000;
    memcpy(exp+24, &ms, sizeof(uint32_t));

    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_wire_v2_decodeHeader(nullptr, exp, 40, &message);

    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    pubsubProtocol_wire_v2_destroy(wireprotocol);
}

TEST_F(WireProtocolV2Test, WireProtocolV2Test_EncodeFooter_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;

    void *footerData = nullptr;
    size_t footerLength = 0;
    celix_status_t status = pubsubProtocol_wire_v2_encodeFooter(nullptr, &message, &footerData, &footerLength);

    unsigned char exp[4];
    uint32_t s = 0xDEAFABBA;
    memcpy(exp, &s, sizeof(uint32_t));
    ASSERT_EQ(status, CELIX_SUCCESS);
    ASSERT_EQ(4, footerLength);
    for (int i = 0; i < 4; i++) {
        if (((unsigned char*) footerData)[i] != exp[i]) {
            std::cerr << "error at index " << std::to_string(i) << std::endl;
        }
        ASSERT_EQ(((unsigned char*) footerData)[i], exp[i]);
    }
    pubsubProtocol_wire_v2_destroy(wireprotocol);
    free(footerData);
}

TEST_F(WireProtocolV2Test, WireProtocolV2Test_DecodeFooter_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    unsigned char exp[4];
    uint32_t s = 0xDEAFABBA;
    memcpy(exp, &s, sizeof(uint32_t));
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    celix_status_t status = pubsubProtocol_wire_v2_decodeFooter(nullptr, exp, 4, &message);

    ASSERT_EQ(CELIX_SUCCESS, status);
    pubsubProtocol_wire_v2_destroy(wireprotocol);
}

TEST_F(WireProtocolV2Test, WireProtocolV2Test_DecodeFooter_IncorrectSync_Test) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_wire_v2_t *wireprotocol;
    pubsubProtocol_wire_v2_create(&wireprotocol);

    unsigned char exp[4];
    uint32_t s = 0xABBABAAB;
    memcpy(exp, &s, sizeof(uint32_t));
    pubsub_protocol_message_t message;

    celix_status_t status = pubsubProtocol_wire_v2_decodeFooter(nullptr, exp, 4, &message);
    ASSERT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    pubsubProtocol_wire_v2_destroy(wireprotocol);
}