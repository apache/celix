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
#include "celix_event_admin.h"
#include "celix_event_adapter.h"
#include "celix_event_admin_service.h"
#include "celix_bundle_activator.h"
#include "celix_framework_factory.h"
#include "celix_dm_component_ei.h"
#include "celix_bundle_context_ei.h"
#include "malloc_ei.h"
#include <gtest/gtest.h>

class CelixEventAdminActTestSuite : public ::testing::Test {
public:
    CelixEventAdminActTestSuite() {
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".event_admin_act_test_cache");
        auto fwPtr = celix_frameworkFactory_createFramework(props);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](celix_framework_t* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(fw.get()), [](celix_bundle_context_t*){/*nop*/}};
    }

    ~CelixEventAdminActTestSuite() override {
        celix_ei_expect_celix_dmComponent_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_dmServiceDependency_create(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getDependencyManager(nullptr, 0, nullptr);
    }

    void TestEventAdminActivator(void (testBody)(void *act, celix_bundle_context_t *ctx)) {
        void *act{};
        auto status = celix_bundleActivator_create(ctx.get(), &act);
        ASSERT_EQ(CELIX_SUCCESS, status);

        testBody(act, ctx.get());

        status = celix_bundleActivator_destroy(act, ctx.get());
        ASSERT_EQ(CELIX_SUCCESS, status);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(CelixEventAdminActTestSuite, FailedToCreateEventAdminComponentTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_create((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(CelixEventAdminActTestSuite, FailedToCreateEventAdminTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_calloc((void*)&celix_eventAdmin_create, 0, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(CelixEventAdminActTestSuite, FailedToCreateEventHandlerDependencyTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(CelixEventAdminActTestSuite, FailedToCreateEventAdapterComponentTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmComponent_create((void*)&celix_bundleActivator_start, 1, nullptr, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(CelixEventAdminActTestSuite, FailedToCreateEventAdapterTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_calloc((void*)&celix_eventAdapter_create, 0, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(CelixEventAdminActTestSuite, FailedToCreateEventAdminDependencyForEventAdapterTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_dmServiceDependency_create((void*)&celix_bundleActivator_start, 1, nullptr, 2);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(CelixEventAdminActTestSuite, FailedToGetDependencyManagerTest) {
    TestEventAdminActivator([](void *act, celix_bundle_context_t *ctx) {
        celix_ei_expect_celix_bundleContext_getDependencyManager((void*)&celix_bundleActivator_start, 1, nullptr);
        auto status = celix_bundleActivator_start(act, ctx);
        ASSERT_EQ(CELIX_ENOMEM, status);
    });
}
