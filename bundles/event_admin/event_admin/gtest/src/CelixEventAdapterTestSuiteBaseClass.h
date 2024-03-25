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

#ifndef CELIX_CELIX_EVENT_ADAPTER_TEST_SUITE_BASE_CLASS_H
#define CELIX_CELIX_EVENT_ADAPTER_TEST_SUITE_BASE_CLASS_H

#include "celix_event_adapter.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include <gtest/gtest.h>

class CelixEventAdapterTestSuiteBaseClass : public ::testing::Test {
public:
    CelixEventAdapterTestSuiteBaseClass() {
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".event_adapter_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) { celix_frameworkFactory_destroyFramework(f); }};
        ctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(fw.get()), [](auto*){/*nop*/}};
    }

    ~CelixEventAdapterTestSuiteBaseClass() override = default;

    void TestEventAdapterPostEvent(void (*testBody)(celix_bundle_context_t *ctx, celix_event_adapter_t *adapter), celix_status_t (*OnPostEvent)(void *handle, const char *topic, const celix_properties_t *props)) {
        auto adapter = celix_eventAdapter_create(ctx.get());
        ASSERT_TRUE(adapter != nullptr);

        auto status = celix_eventAdapter_start(adapter);
        ASSERT_EQ(status, CELIX_SUCCESS);

        celix_bundleContext_waitForEvents(ctx.get());

        celix_event_admin_service_t eventAdminService{};
        eventAdminService.handle = adapter;
        eventAdminService.postEvent = OnPostEvent;
        status = celix_eventAdapter_setEventAdminService(adapter, &eventAdminService);
        ASSERT_EQ(status, CELIX_SUCCESS);

        testBody(ctx.get(), adapter);

        status = celix_eventAdapter_stop(adapter);
        ASSERT_EQ(status, CELIX_SUCCESS);
        celix_eventAdapter_destroy(adapter);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};


#endif //CELIX_CELIX_EVENT_ADAPTER_TEST_SUITE_BASE_CLASS_H
