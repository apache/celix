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

#include "celix/FrameworkFactory.h"
#include "celix/FrameworkUtils.h"
#include "celix_condition.h"
#include "celix_components_ready_constants.h"

class ComponentsReadyTestSuite : public ::testing::Test {
public:
    const int USE_SERVICE_TIMEOUT_IN_MS = 2000;  // TODO improve test time to a lower value
    const std::string componentsReadyFilter =
            std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_COMPONENTS_READY + ")";

    ComponentsReadyTestSuite() = default;
};

TEST_F(ComponentsReadyTestSuite, ComponentsReadyTest) {
    // Given a Celix framework
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // When the components ready check bundle is installed
    celix::installBundleSet(*fw, COMPONENTS_READY_CHECK_BUNDLE_SET);

    // Then the condition service with id "components.ready" will become available
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
            .setFilter(componentsReadyFilter)
            .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
            .build();
    EXPECT_EQ(1, count);
}

TEST_F(ComponentsReadyTestSuite, ComponentsReadyWithActiveComponentTest) {
    // Given a Celix framework
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // When a test bundle with an active-able component is installed
    celix::installBundleSet(*fw, ACTIVE_CMP_TEST_BUNDLE_SET);

    // And the components ready bundle check is installed
    celix::installBundleSet(*fw, COMPONENTS_READY_CHECK_BUNDLE_SET);

    // Then the "components.ready" condition will become available
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
            .setFilter(componentsReadyFilter)
            .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
            .build();
    EXPECT_EQ(1, count);
}


TEST_F(ComponentsReadyTestSuite, ComponentsReadyWithInactiveComponentTest) {
    // Given a Celix framework
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // When a test bundle with a not active-able component is installed
    celix::installBundleSet(*fw, INACTIVE_CMP_TEST_BUNDLE_SET);

    // WAnd the components ready check bundle is installed
    celix::installBundleSet(*fw, COMPONENTS_READY_CHECK_BUNDLE_SET);

    // Then the "components.ready" condition will not become available, because the test bundle contains a component
    // which cannot become active
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
            .setFilter(componentsReadyFilter)
            .setTimeout(std::chrono::milliseconds{USE_SERVICE_TIMEOUT_IN_MS})
            .build();
    EXPECT_EQ(0, count);
}
