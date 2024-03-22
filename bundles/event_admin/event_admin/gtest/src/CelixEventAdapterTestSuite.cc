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
#include <gtest/gtest.h>

class CelixEventAdapterTestSuite : public CelixEventAdapterTestSuiteBaseClass {
public:
    CelixEventAdapterTestSuite() = default;

    ~CelixEventAdapterTestSuite() override = default;
};

TEST_F(CelixEventAdapterTestSuite, CreateEventAdapterTest) {
    auto adapter = celix_eventAdapter_create(ctx.get());
    ASSERT_TRUE(adapter != nullptr);
    celix_eventAdapter_destroy(adapter);
}

TEST_F(CelixEventAdapterTestSuite, DestroyEventAdapterWithNullptrTest) {
    celix_eventAdapter_destroy(nullptr);
}

TEST_F(CelixEventAdapterTestSuite, StartEventAdapterTest) {
    auto adapter = celix_eventAdapter_create(ctx.get());
    ASSERT_TRUE(adapter != nullptr);

    auto status = celix_eventAdapter_start(adapter);
    ASSERT_EQ(status, CELIX_SUCCESS);

    status = celix_eventAdapter_stop(adapter);
    ASSERT_EQ(status, CELIX_SUCCESS);
    celix_eventAdapter_destroy(adapter);
}

TEST_F(CelixEventAdapterTestSuite, PostServiceEventTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        (void)adapter;
        auto svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", nullptr);
        ASSERT_TRUE(svcId >= 0);
        celix_bundleContext_unregisterService(ctx, svcId);
    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        auto r = strstr(topic, "celix/framework/ServiceEvent/");
        EXPECT_TRUE(r != nullptr);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdapterTestSuite, PostServiceEventButEventAdminServiceIsNotSetTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        celix_eventAdapter_setEventAdminService(adapter, nullptr);
        auto svcId = celix_bundleContext_registerService(ctx, (void*)"dump_service", "test service", nullptr);
        ASSERT_TRUE(svcId >= 0);
        celix_bundleContext_unregisterService(ctx, svcId);
    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)topic;
        (void)props;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}


TEST_F(CelixEventAdapterTestSuite, PostBundleEventTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        (void)adapter;
        auto bundleId = celix_bundleContext_installBundle(ctx, TEST_BUNDLE, true);
        ASSERT_TRUE(bundleId >= 0);
        celix_bundleContext_uninstallBundle(ctx, bundleId);

        celix_eventAdapter_setEventAdminService(adapter, nullptr);
    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        auto r = strstr(topic, "celix/framework/BundleEvent/");
        EXPECT_TRUE(r != nullptr);
        return CELIX_SUCCESS;
    });
}


TEST_F(CelixEventAdapterTestSuite, PostBundleEventButEventAdminServiceIsNotSetTest) {
    TestEventAdapterPostEvent([](celix_bundle_context_t *ctx, celix_event_adapter_t *adapter){
        (void)adapter;
        celix_eventAdapter_setEventAdminService(adapter, nullptr);
        auto bundleId = celix_bundleContext_installBundle(ctx, TEST_BUNDLE, true);
        ASSERT_TRUE(bundleId >= 0);
        celix_bundleContext_uninstallBundle(ctx, bundleId);
    }, [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        auto r = strstr(topic, "celix/framework/BundleEvent/");
        EXPECT_TRUE(r != nullptr);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdapterTestSuite, PostFrameworkEventTest) {
    auto adapter = celix_eventAdapter_create(ctx.get());
    ASSERT_TRUE(adapter != nullptr);

    celix_event_admin_service_t eventAdminService{};
    eventAdminService.handle = adapter;
    eventAdminService.postEvent = [](void *handle, const char *topic, const celix_properties_t *props) -> celix_status_t {
        (void)handle;
        (void)props;
        (void)topic;
        return CELIX_SUCCESS;
    };
    auto status = celix_eventAdapter_setEventAdminService(adapter, &eventAdminService);
    ASSERT_EQ(status, CELIX_SUCCESS);

    status = celix_eventAdapter_start(adapter);
    ASSERT_EQ(status, CELIX_SUCCESS);

    celix_bundleContext_waitForEvents(ctx.get());

    status = celix_eventAdapter_stop(adapter);
    ASSERT_EQ(status, CELIX_SUCCESS);
    celix_eventAdapter_destroy(adapter);
}