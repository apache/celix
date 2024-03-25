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
#include <gtest/gtest.h>

class CelixEventTestSuite : public ::testing::Test {
public:
    CelixEventTestSuite() = default;

    ~CelixEventTestSuite() override = default;
};

TEST_F(CelixEventTestSuite, CreateEventTest) {
    std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> properties{celix_properties_create(), celix_properties_destroy};
    celix_properties_set(properties.get(), "key", "value");
    celix_event_t *event = celix_event_create("test_topic", properties.get());
    EXPECT_TRUE(event != nullptr);
    EXPECT_STREQ("test_topic", celix_event_getTopic(event));
    EXPECT_STREQ("value", celix_properties_get(celix_event_getProperties(event), "key", nullptr));
    celix_event_release(event);
}

TEST_F(CelixEventTestSuite, CreateEventWithoutPropertiesTest) {
    celix_event_t *event = celix_event_create("test_topic", nullptr);
    EXPECT_TRUE(event != nullptr);
    EXPECT_STREQ("test_topic", celix_event_getTopic(event));
    EXPECT_TRUE(celix_event_getProperties(event) == nullptr);
    celix_event_release(event);
}
