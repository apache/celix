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
#include "CelixEventAdapterTestSuiteBaseClass.h"
#include "celix_event_adapter.h"
#include "celix_properties_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_log_helper_ei.h"
#include "celix_threads_ei.h"
#include "malloc_ei.h"
#include "celix_event_constants.h"
#include <gtest/gtest.h>

class CelixEventAdapterErrorInjectionTestSuite : public CelixEventAdapterTestSuiteBaseClass {
public:
    CelixEventAdapterErrorInjectionTestSuite() = default;

    ~CelixEventAdapterErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_logHelper_create(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_set(nullptr, 0, 0);
        celix_ei_expect_celix_properties_setLong(nullptr, 0, 0);
        celix_ei_expect_celix_properties_setVersion(nullptr, 0, 0);
    }

    void TestEventAdapterStartStop(void (*testBody)(void)) {
        auto adapter = celix_eventAdapter_create(ctx.get());
        ASSERT_TRUE(adapter != nullptr);

        testBody();

        auto status = celix_eventAdapter_start(adapter);
        ASSERT_EQ(status, CELIX_SUCCESS);

        status = celix_eventAdapter_stop(adapter);
        ASSERT_EQ(status, CELIX_SUCCESS);
        celix_eventAdapter_destroy(adapter);
    }
};

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToCreateLogHelperTest) {
    celix_ei_expect_celix_logHelper_create((void*)&celix_eventAdapter_create, 0, nullptr);
    auto adapter = celix_eventAdapter_create(ctx.get());
    ASSERT_EQ(nullptr, adapter);
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToAllocMemoryForEventAdapterTest) {
    celix_ei_expect_calloc((void*)&celix_eventAdapter_create, 0, nullptr);
    auto adapter = celix_eventAdapter_create(ctx.get());
    ASSERT_EQ(nullptr, adapter);
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToCreateRwLockTest) {
    celix_ei_expect_celixThreadRwlock_create((void*)&celix_eventAdapter_create, 0, CELIX_ENOMEM);
    auto adapter = celix_eventAdapter_create(ctx.get());
    ASSERT_EQ(nullptr, adapter);
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToTrackServiceTest) {
    TestEventAdapterStartStop([](void){
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&celix_eventAdapter_start, 0, -1);
    });
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToTrackBundle) {
    TestEventAdapterStartStop([](void){
        celix_ei_expect_celix_bundleContext_trackBundlesWithOptionsAsync((void*)&celix_eventAdapter_start, 0, -1, 2);
    });
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToTrackFrameworkEventv) {
    TestEventAdapterStartStop([](void){
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&celix_eventAdapter_start, 0, -1, 2);
    });
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToCreateRegisteringServiceEventPorpertiesTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        (void)adapter;
        celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        auto svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", nullptr);
        ASSERT_TRUE(svcId >= 0);
        celix_bundleContext_unregisterService(ctx, svcId);

        celix_ei_expect_celix_properties_set(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
        svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", nullptr);
        ASSERT_TRUE(svcId >= 0);
        celix_bundleContext_unregisterService(ctx, svcId);

        celix_ei_expect_celix_properties_set(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 2);
        svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", nullptr);
        ASSERT_TRUE(svcId >= 0);
        celix_bundleContext_unregisterService(ctx, svcId);

        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_SERVICE_PID, "test_dump_service");
        celix_ei_expect_celix_properties_set(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 3);
        svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", props);
        ASSERT_TRUE(svcId >= 0);
        celix_bundleContext_unregisterService(ctx, svcId);

    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        EXPECT_STRNE(topic, "celix/framework/ServiceEvent/REGISTERED");
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToCreateUnregisteringServiceEventPorpertiesTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        auto svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", nullptr);
        ASSERT_TRUE(svcId >= 0);
        celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        celix_bundleContext_unregisterService(ctx, svcId);

        celix_eventAdapter_setEventAdminService(adapter, nullptr);
    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        EXPECT_STRNE(topic, "celix/framework/ServiceEvent/UNREGISTERING");
        return CELIX_SUCCESS;
    });
}


TEST_F(CelixEventAdapterErrorInjectionTestSuite, FailedToCreateBundleEventPropertiesTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        (void)adapter;
        celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        auto bundleId = celix_bundleContext_installBundle(ctx, TEST_BUNDLE, false);
        ASSERT_TRUE(bundleId >= 0);
        celix_bundleContext_waitForEvents(ctx);
        celix_bundleContext_uninstallBundle(ctx, bundleId);

        celix_ei_expect_celix_properties_setLong(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
        bundleId = celix_bundleContext_installBundle(ctx, TEST_BUNDLE, false);
        ASSERT_TRUE(bundleId >= 0);
        celix_bundleContext_waitForEvents(ctx);
        celix_bundleContext_uninstallBundle(ctx, bundleId);

        celix_ei_expect_celix_properties_set(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
        bundleId = celix_bundleContext_installBundle(ctx, TEST_BUNDLE, false);
        ASSERT_TRUE(bundleId >= 0);
        celix_bundleContext_waitForEvents(ctx);
        celix_bundleContext_uninstallBundle(ctx, bundleId);

        celix_ei_expect_celix_properties_setVersion(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
        bundleId = celix_bundleContext_installBundle(ctx, TEST_BUNDLE, false);
        ASSERT_TRUE(bundleId >= 0);
        celix_bundleContext_waitForEvents(ctx);
        celix_bundleContext_uninstallBundle(ctx, bundleId);

        celix_eventAdapter_setEventAdminService(adapter, nullptr);
    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        EXPECT_STRNE(topic, "celix/framework/BundleEvent/INSTALLED");
        return CELIX_SUCCESS;
    });
}