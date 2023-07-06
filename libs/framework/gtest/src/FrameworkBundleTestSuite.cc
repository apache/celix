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

class FrameworkBundleTestSuite : public ::testing::Test {
  public:
    const int USE_SERVICE_TIMEOUT_IN_MS = 500;
    const std::string trueFilter = std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_TRUE + ")";
    const std::string frameworkReadyFilter =
        std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";
    const std::string frameworkErrorFilter =
        std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_ERROR + ")";
    const std::string componentsReadyFilter =
        std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_COMPONENTS_READY + ")";

    FrameworkBundleTestSuite() = default;
};

TEST_F(FrameworkBundleTestSuite, ConditionTrueFrameworkReadyAndComponentsReadyTest) {
    // Given a Celix framework (with conditions enabled (default))
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // Then the condition service with id "true" is available
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME).setFilter(trueFilter).build();
    EXPECT_EQ(1, count);

    // And the condition service with id "framework.error" will not become available
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(frameworkErrorFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);

    // But the condition service with id "framework.ready" will become available
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(frameworkReadyFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);
    
    // And the condition service with id "components.ready" will become available
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(componentsReadyFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);
}

TEST_F(FrameworkBundleTestSuite, ConditionTrueAndFrameworkErrorTest) {
    // Given a Celix framework which is configured to start an invalid bundle
    auto fw = celix::createFramework({{celix::AUTO_START_0, "non-existing-bundle.zip"}});
    auto ctx = fw->getFrameworkBundleContext();

    // Then the condition service with id "true" is available
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME).setFilter(trueFilter).build();
    EXPECT_EQ(1, count);

    // And the condition service with id "framework.ready" does not become available (framework startup error)
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(frameworkReadyFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);

    // But the condition service with id "framework.error" will become available
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(frameworkErrorFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);
}

TEST_F(FrameworkBundleTestSuite, FrameworkReadyRegisteredLaterTest) {
    // Given a Celix framework which is configured to start a bundle with a condition test service
    auto fw = celix::createFramework({{celix::AUTO_START_0, COND_TEST_BUNDLE_LOC}});
    auto ctx = fw->getFrameworkBundleContext();

    // Then the condition service with id "true" is available
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME).setFilter(trueFilter).build();
    EXPECT_EQ(1, count);

    // And the condition service with id "framework.error" will not become available
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(frameworkErrorFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);

    // But the condition service with id "framework.ready" will become available
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(frameworkReadyFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);

    // And the condition service with id "test" is available
    auto testFilter = std::string{"("} + CELIX_CONDITION_ID + "=test)";
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(testFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(1, count);

    // And the service.id of the framework.ready condition is higher than the service.id of the test condition
    //(white-box test, framework.ready condition is registered last)
    long readySvcId = ctx->findServiceWithName(CELIX_CONDITION_SERVICE_NAME, frameworkReadyFilter);
    long testySvcId = ctx->findServiceWithName(CELIX_CONDITION_SERVICE_NAME, testFilter);
    EXPECT_GT(readySvcId, testySvcId);

    // And the "components.ready" condition is not available, because the test bundle contains a component which will
    // not become active
    count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
                .setFilter(componentsReadyFilter)
                .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
                .build();
    EXPECT_EQ(0, count);
}

