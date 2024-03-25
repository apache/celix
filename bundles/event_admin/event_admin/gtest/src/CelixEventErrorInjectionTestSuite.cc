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
#include "celix_event.h"
#include "celix_properties_ei.h"
#include "celix_utils_ei.h"
#include "malloc_ei.h"
#include <gtest/gtest.h>

class CelixEventErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixEventErrorInjectionTestSuite() = default;

    ~CelixEventErrorInjectionTestSuite() override {
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
    }
};

TEST_F(CelixEventErrorInjectionTestSuite, FailedToCopyEventPropertiesTest) {
    std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> properties{celix_properties_create(),
                                                                                        celix_properties_destroy};
    celix_properties_set(properties.get(), "key", "value");
    celix_ei_expect_celix_properties_copy((void*)&celix_event_create, 0, nullptr);
    celix_event_t *event = celix_event_create("test_topic", properties.get());
    EXPECT_TRUE(event == nullptr);
}

TEST_F(CelixEventErrorInjectionTestSuite, FailedToAllocMemoryForEventTest) {
    celix_ei_expect_calloc((void*)&celix_event_create, 0, nullptr);
    celix_event_t *event = celix_event_create(nullptr, nullptr);
    EXPECT_TRUE(event == nullptr);
}

TEST_F(CelixEventErrorInjectionTestSuite, FailedToAllocMemoryForTopicTest) {
    celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
    celix_event_t *event = celix_event_create(nullptr, nullptr);
    EXPECT_TRUE(event == nullptr);
}