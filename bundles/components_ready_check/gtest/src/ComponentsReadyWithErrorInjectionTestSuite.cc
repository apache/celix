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
#include "celix_components_ready_check.h"

#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_properties_ei.h"

class ComponentsReadyWithErrorInjectionTestSuite : public ::testing::Test {
public:
    const int USE_SERVICE_TIMEOUT_IN_MS = 250;
    const std::string frameworkReadyFilter =
            std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_FRAMEWORK_READY + ")";
    const std::string componentsReadyFilter =
            std::string{"("} + CELIX_CONDITION_ID + "=" + CELIX_CONDITION_ID_COMPONENTS_READY + ")";

    ComponentsReadyWithErrorInjectionTestSuite() = default;

    ~ComponentsReadyWithErrorInjectionTestSuite() noexcept override {
        // reset error injections
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_scheduleEvent(nullptr, 0, 0);
    }
};

TEST_F(ComponentsReadyWithErrorInjectionTestSuite, ErrorCreatingComponentsReadyCheck) {
    // Given a Celix framework
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // When an error injection for calloc is primed when called from celix_componentsReadyCheck_create
    celix_ei_expect_calloc((void*)celix_componentsReadyCheck_create, 0, nullptr);

    // Then the components ready check will cannot be created
    auto* rdy = celix_componentsReadyCheck_create(ctx->getCBundleContext());
    EXPECT_EQ(rdy, nullptr);

    // When an error injection for mutex create is primed when called from celix_componentsReadyCheck_create
    celix_ei_expect_celixThreadMutex_create((void*)celix_componentsReadyCheck_create, 0, CELIX_BUNDLE_EXCEPTION);

    // Then the components ready check will cannot be created
    rdy = celix_componentsReadyCheck_create(ctx->getCBundleContext());
    EXPECT_EQ(rdy, nullptr);

    // When an error injection for celix_bundleContext_trackServicesWithOptionsAsync is primed when called from
    // celix_componentsReadyCheck_create
    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)celix_componentsReadyCheck_create, 0, -1);

    // Then the components ready check will cannot be created
    rdy = celix_componentsReadyCheck_create(ctx->getCBundleContext());
    EXPECT_EQ(rdy, nullptr);
}

TEST_F(ComponentsReadyWithErrorInjectionTestSuite, ErrorRegisteringComponentsReadyConditionTest) {
    // Given a Celix framework
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // When an error injection for celix_properties_create is primed when called from celix_frameworkBundle_componentsCheck
    celix_ei_expect_celix_properties_create((void*)celix_componentReadyCheck_registerCondition, 0, nullptr);

    // And the components ready check is created
    auto* rdy = celix_componentsReadyCheck_create(ctx->getCBundleContext());
    EXPECT_NE(rdy, nullptr);

    // But the components.ready condition will not become available, due to an error creating properties for the service
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
            .setFilter(componentsReadyFilter)
            .setTimeout(std::chrono::milliseconds {USE_SERVICE_TIMEOUT_IN_MS})
            .build();
    EXPECT_EQ(count, 0);

    celix_componentsReadyCheck_destroy(rdy);
}

TEST_F(ComponentsReadyWithErrorInjectionTestSuite, ErrorSchedulingReadyCheckEventTest) {
    // Given a Celix framework
    auto fw = celix::createFramework();
    auto ctx = fw->getFrameworkBundleContext();

    // When an error injection for celix_bundleContext_scheduleEvent is primed when called from celix_componentReadyCheck_setFrameworkReadySvc
    celix_ei_expect_celix_bundleContext_scheduleEvent((void*)celix_componentReadyCheck_setFrameworkReadySvc, 0, -1);

    // And the components ready check is created
    auto* rdy = celix_componentsReadyCheck_create(ctx->getCBundleContext());
    EXPECT_NE(rdy, nullptr);

    // But the components.ready condition will not become available, due to an error creating a check scheduled event
    auto count = ctx->useService<celix_condition>(CELIX_CONDITION_SERVICE_NAME)
            .setFilter(componentsReadyFilter)
            .setTimeout(std::chrono::milliseconds {USE_SERVICE_TIMEOUT_IN_MS})
            .build();
    EXPECT_EQ(count, 0);

    celix_componentsReadyCheck_destroy(rdy);
}
