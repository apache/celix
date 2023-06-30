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

#include "celix/Constants.h"
#include "celix/FrameworkFactory.h"
#include "celix_condition.h"

class FrameworkConditionsTestSuite : public ::testing::Test {
  public:
    const int USE_SERVICE_TIMEOUT_IN_MS = 500;

    FrameworkConditionsTestSuite() = default;
};

TEST_F(FrameworkConditionsTestSuite, ConditionTrueAndFrameworkReadyTest) {
    // Given a Celix framework (with conditions enabled (default))
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // Then the condition service with id "true" is available
    auto filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_TRUE + ")";
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME).setFilter(filter).build();
    EXPECT_EQ(1, count);

    // And the condition service with id "framework.error" will not become available
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_ERROR + ")";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);

    // But the condition service with id "framework.ready" will become available
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);
}

TEST_F(FrameworkConditionsTestSuite, ConditionTrueAndFrameworkErrorTest) {
    // Given a Celix framework which is configured to start an invalid bundle
    auto fw = celix::createFramework({{celix::AUTO_START_0, "non-existing-bundle.zip"}});
    auto ctx = fw->getFrameworkBundleContext();

    // Then the condition service with id "true" is available
    auto filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_TRUE + ")";
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME).setFilter(filter).build();
    EXPECT_EQ(1, count);

    // And the condition service with id "framework.ready" does not become available (framework startup error)
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);

    // But the condition service with id "framework.error" will become available
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_ERROR + ")";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);
}

TEST_F(FrameworkConditionsTestSuite, FrameworkReadyRegisteredLastTest) {
    // Given a Celix framework which is configured to start a bundle with a condition test service
    auto fw = celix::createFramework({{celix::AUTO_START_0, COND_TEST_BUNDLE_LOC}});
    auto ctx = fw->getFrameworkBundleContext();

    // Then the condition service with id "true" is available
    auto filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_TRUE + ")";
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME).setFilter(filter).build();
    EXPECT_EQ(1, count);

    // And the condition service with id "framework.error" will not become available
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_ERROR + ")";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);

    // But the condition service with id "framework.ready" will become available
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);

    // And the condition service with id "test" is available
    filter = std::string{"("} + CELIX_CONDITION_ID + "=test)";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(filter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);

    // And the service.id of the framework.ready condition is higher than the service.id of the test condition
    //(white-box test, framework.ready condition is registered last)
    filter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";
    long readySvcId = ctx->findServiceWithName(CELIX_CONDITION_SERVICE_NAME, filter);
    filter = std::string{"("} + CELIX_CONDITION_ID + "=test)";
    long testySvcId = ctx->findServiceWithName(CELIX_CONDITION_SERVICE_NAME, filter);
    EXPECT_GT(readySvcId, testySvcId);
}
