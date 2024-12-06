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

#ifndef CELIX_CELIX_EVENT_ADMIN_TEST_SUITE_BASE_CLASS_H
#define CELIX_CELIX_EVENT_ADMIN_TEST_SUITE_BASE_CLASS_H

#include <functional>
#include <future>
#include "celix_event_admin.h"
#include "celix_event_handler_service.h"
#include "celix_event_constants.h"
#include "celix_event_remote_provider_service.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include <gtest/gtest.h>

class CelixEventAdminTestSuiteBaseClass : public ::testing::Test {
public:
    CelixEventAdminTestSuiteBaseClass() {

        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".event_admin_test_cache");
        auto fwPtr = celix_frameworkFactory_createFramework(props);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](celix_framework_t* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(fw.get()), [](celix_bundle_context_t*){/*nop*/}};
    }

    ~CelixEventAdminTestSuiteBaseClass() override = default;

    void TestEventAdmin(void (*testBody)(celix_event_admin_t *ea, celix_bundle_context_t *ctx)) {
        auto ea = celix_eventAdmin_create(ctx.get());
        EXPECT_TRUE(ea != nullptr);
        auto status = celix_eventAdmin_start(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);

        testBody(ea, ctx.get());

        status = celix_eventAdmin_stop(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_eventAdmin_destroy(ea);
    }

    void TestAddEventHandler(void (*addHandler)(void *handle, void *svc, const celix_properties_t *props), void (*removeHandler)(void *handle, void *svc, const celix_properties_t *props)) {
        auto ea = celix_eventAdmin_create(ctx.get());
        EXPECT_TRUE(ea != nullptr);
        auto status = celix_eventAdmin_start(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);

        celix_event_handler_service_t handler;
        handler.handle = nullptr;
        handler.handleEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
            (void)handle;
            (void)topic;
            (void)props;
            return CELIX_SUCCESS;
        };
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "org/celix/test");
        auto handlerSvcId = celix_bundleContext_registerService(ctx.get(), &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props);
        ASSERT_TRUE(handlerSvcId >= 0);

        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_EVENT_HANDLER_SERVICE_NAME;
        opts.filter.versionRange = CELIX_EVENT_HANDLER_SERVICE_VERSION;
        opts.callbackHandle = ea;
        opts.addWithProperties = addHandler;
        opts.removeWithProperties = removeHandler;
        long handlerTrkId = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(handlerTrkId >= 0);

        celix_bundleContext_unregisterService(ctx.get(), handlerSvcId);
        celix_bundleContext_stopTracker(ctx.get(), handlerTrkId);

        status = celix_eventAdmin_stop(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_eventAdmin_destroy(ea);
    }

    void TestSubscribeEvent(const char *topics) {
        auto ea = celix_eventAdmin_create(ctx.get());
        EXPECT_TRUE(ea != nullptr);
        auto status = celix_eventAdmin_start(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);

        celix_event_handler_service_t handler;
        handler.handle = nullptr;
        handler.handleEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
            (void)handle;
            (void)topic;
            (void)props;
            return CELIX_SUCCESS;
        };
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props, CELIX_EVENT_TOPIC, topics);

        auto handlerSvcId = celix_bundleContext_registerService(ctx.get(), &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props);
        ASSERT_TRUE(handlerSvcId >= 0);

        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_EVENT_HANDLER_SERVICE_NAME;
        opts.filter.versionRange = CELIX_EVENT_HANDLER_SERVICE_VERSION;
        opts.callbackHandle = ea;
        opts.addWithProperties = [](void *handle, void *svc, const celix_properties_t *props) {
            auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
            EXPECT_EQ(CELIX_SUCCESS, status);
        };
        opts.removeWithProperties = [](void *handle, void *svc, const celix_properties_t *props) {
            auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
            EXPECT_EQ(CELIX_SUCCESS, status);
        };
        long handlerTrkId = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(handlerTrkId >= 0);

        celix_bundleContext_unregisterService(ctx.get(), handlerSvcId);
        celix_bundleContext_stopTracker(ctx.get(), handlerTrkId);

        status = celix_eventAdmin_stop(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_eventAdmin_destroy(ea);
    }

    void TestPublishEvent(const char *handlerTopics, const char *eventFilter, const std::function<void (celix_event_admin_t*)>& testBody,
                          const std::function<celix_status_t(void*, const char*, const celix_properties_t*)>& onHandleEvent, bool asyncUnordered = false) {
        auto ea = celix_eventAdmin_create(ctx.get());
        EXPECT_TRUE(ea != nullptr);
        auto status = celix_eventAdmin_start(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);
        struct celix_handle_event_callback_data {
                const std::function<celix_status_t (void*, const char*, const celix_properties_t*)>& onHandleEvent;
                void* handle;
        } data{onHandleEvent, ea};
        celix_event_handler_service_t handler;
        handler.handle = &data;
        handler.handleEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
            auto data = static_cast<celix_handle_event_callback_data*>(handle);
            return data->onHandleEvent(data->handle, topic, props);
        };
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props, CELIX_EVENT_TOPIC, handlerTopics);
        if (eventFilter) {
            celix_properties_set(props, CELIX_EVENT_FILTER, eventFilter);
        }
        if (asyncUnordered) {
            celix_properties_set(props, CELIX_EVENT_DELIVERY, CELIX_EVENT_DELIVERY_ASYNC_UNORDERED);
        }

        auto handlerSvcId = celix_bundleContext_registerService(ctx.get(), &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props);
        ASSERT_TRUE(handlerSvcId >= 0);

        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_EVENT_HANDLER_SERVICE_NAME;
        opts.filter.versionRange = CELIX_EVENT_HANDLER_SERVICE_VERSION;
        opts.callbackHandle = ea;
        opts.addWithProperties = [](void *handle, void *svc, const celix_properties_t *props) {
            auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
            EXPECT_EQ(CELIX_SUCCESS, status);
        };
        opts.removeWithProperties = [](void *handle, void *svc, const celix_properties_t *props) {
            auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
            EXPECT_EQ(CELIX_SUCCESS, status);
        };
        long handlerTrkId = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(handlerTrkId >= 0);

        testBody(ea);

        celix_bundleContext_stopTracker(ctx.get(), handlerTrkId);
        celix_bundleContext_unregisterService(ctx.get(), handlerSvcId);

        status = celix_eventAdmin_stop(ea);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_eventAdmin_destroy(ea);
    }

    void TestPublishEventToRemote(const std::function<void(celix_event_admin_t*)>& testBody, celix_event_remote_provider_service_t* remoteProviderService) {
        TestPublishEvent("*", nullptr, [&testBody, remoteProviderService](celix_event_admin_t* ea) {
            celix_autoptr(celix_properties_t) props = celix_properties_create();
            celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
            auto status = celix_eventAdmin_addRemoteProviderService(ea, remoteProviderService, props);
            EXPECT_EQ(CELIX_SUCCESS, status);
            testBody(ea);
        }, [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS;});
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

#endif //CELIX_CELIX_EVENT_ADMIN_TEST_SUITE_BASE_CLASS_H
