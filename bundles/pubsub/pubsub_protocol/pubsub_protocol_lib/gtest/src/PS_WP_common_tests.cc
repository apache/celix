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

#include <iostream>

#include <pubsub_wire_protocol_common.h>

#include "gtest/gtest.h"

#include <cstring>
#include <chrono>

class WireProtocolCommonTest : public ::testing::Test {
public:
    WireProtocolCommonTest() = default;
    ~WireProtocolCommonTest() override = default;
};

TEST_F(WireProtocolCommonTest, WireProtocolCommonTest_EncodeMetadata_Benchmark) { // NOLINT(cert-err58-cpp)
    pubsub_protocol_message_t message;
    message.header.convertEndianess = 0;
    message.metadata.metadata = celix_properties_create();
    celix_properties_set(message.metadata.metadata, "key1", "value1");
    celix_properties_set(message.metadata.metadata, "key2", "value2");
    celix_properties_set(message.metadata.metadata, "key3", "value3");
    celix_properties_set(message.metadata.metadata, "key4", "value4");
    celix_properties_set(message.metadata.metadata, "key5", "value5");

    void *data = nullptr;
    size_t length = 0;
    auto start = std::chrono::system_clock::now();
    for(int i = 0; i < 100000; i++) {
        celix_status_t status = pubsubProtocol_encodeMetadata(&message, &data, &length);
        ASSERT_TRUE(status == CELIX_SUCCESS);
    }
    auto end = std::chrono::system_clock::now();
    free(data);

    std::cout << "WireProtocolCommonTest_EncodeMetadata_Benchmark took " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " µs\n";
    celix_properties_destroy(message.metadata.metadata);
}