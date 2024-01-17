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
#include "CelixEventAdminTestSuiteBaseClass.h"
#include "celix_event_admin.h"
#include "celix_event.h"
#include "celix_event_handler_service.h"
#include "celix_event_constants.h"
#include "celix_log_helper_ei.h"
#include "celix_array_list_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_threads_ei.h"
#include "celix_utils_ei.h"
#include "malloc_ei.h"
#include <gtest/gtest.h>


class CelixEventAdminErrorInjectionTestSuite : public CelixEventAdminTestSuiteBaseClass {
public:
    CelixEventAdminErrorInjectionTestSuite() = default;

    ~CelixEventAdminErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_logHelper_create(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadCondition_init(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_addLong(nullptr, 0, 0);
        celix_ei_expect_celix_elapsedtime(nullptr, 0, 0);
    }
};


TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToStartEventAdminTest) {
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_TRUE(ea != nullptr);
    celix_ei_expect_celixThread_create((void*)&celix_eventAdmin_start, 0, CELIX_ENOMEM);

    auto status = celix_eventAdmin_start(ea);
    EXPECT_EQ(CELIX_ENOMEM, status);

    celix_eventAdmin_destroy(ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToStartEventAdmin2Test) {
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_TRUE(ea != nullptr);
    celix_ei_expect_celixThread_create((void*)&celix_eventAdmin_start, 0, CELIX_ENOMEM, 2);

    auto status = celix_eventAdmin_start(ea);
    EXPECT_EQ(CELIX_ENOMEM, status);

    celix_eventAdmin_destroy(ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToAllocMemoryEventAdminTest) {
    celix_ei_expect_calloc((void*)&celix_eventAdmin_create, 0, nullptr);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateLogHelperForEventAdminTest) {
    celix_ei_expect_celix_logHelper_create((void*)&celix_eventAdmin_create, 0, nullptr);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateLockForEventAdminTest) {
    celix_ei_expect_celixThreadRwlock_create((void*)&celix_eventAdmin_create, 0, CELIX_ENOMEM);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateChannelMatchingAllEventsForEventAdminTest) {
    celix_ei_expect_celix_arrayList_create((void*)&celix_eventAdmin_create, 0, nullptr);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateChannelsMatchingTopicForEventAdminTest) {
    celix_ei_expect_celix_stringHashMap_create((void*)&celix_eventAdmin_create, 0, nullptr);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateChannelsMatchingPrefixTopicForEventAdminTest) {
    celix_ei_expect_celix_stringHashMap_create((void*)&celix_eventAdmin_create, 0, nullptr, 2);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateEventHandlersMapForEventAdminTest) {
    celix_ei_expect_celix_longHashMap_create((void*)&celix_eventAdmin_create, 0, nullptr);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateMutexForEventAdminTest) {
    celix_ei_expect_celixThreadMutex_create((void*)&celix_eventAdmin_create, 0, CELIX_ENOMEM);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateCondForEventAdminTest) {
    celix_ei_expect_celixThreadCondition_init((void*)&celix_eventAdmin_create, 0, CELIX_ENOMEM);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateAsyncEventQueueForEventAdminTest) {
    celix_ei_expect_celix_arrayList_create((void*)&celix_eventAdmin_create, 0, nullptr, 2);
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_EQ(nullptr, ea);
}


TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToAllocMemoryForEventHandlerTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        celix_ei_expect_calloc((void*)&celix_eventAdmin_addEventHandlerWithProperties, 0, nullptr);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToDupTopicsWhenAddingEventHandlerTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        celix_ei_expect_celix_utils_strdup((void*)&celix_eventAdmin_addEventHandlerWithProperties, 0, nullptr);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateTopicsMapWhenAddingEventHandlerTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        celix_ei_expect_celix_stringHashMap_create((void*)&celix_eventAdmin_addEventHandlerWithProperties, 0, nullptr);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToPutTopicToMapWhenAddingEventHandlerTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        celix_ei_expect_celix_stringHashMap_put((void*)&celix_eventAdmin_addEventHandlerWithProperties, 0, CELIX_ENOMEM);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToPutHandlerToMapWhenAddingEventHandlerTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        celix_ei_expect_celix_longHashMap_put((void*)&celix_eventAdmin_addEventHandlerWithProperties, 0, CELIX_ENOMEM);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, props);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToSubscribeAllTopicTest) {
    celix_ei_expect_celix_arrayList_addLong((void*)&celix_eventAdmin_addEventHandlerWithProperties, 1, CELIX_ENOMEM);
    TestSubscribeEvent("*");
}


TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToSubscribeTopicTest) {
    celix_ei_expect_calloc((void*)&celix_eventAdmin_addEventHandlerWithProperties, 2, nullptr);
    TestSubscribeEvent("org/celix/test");

    celix_ei_expect_celix_arrayList_create((void*)&celix_eventAdmin_addEventHandlerWithProperties, 2, nullptr);
    TestSubscribeEvent("org/celix/test");

    celix_ei_expect_celix_arrayList_addLong((void*)&celix_eventAdmin_addEventHandlerWithProperties, 2, CELIX_ENOMEM);
    TestSubscribeEvent("org/celix/test");

    celix_ei_expect_celix_stringHashMap_put((void*)&celix_eventAdmin_addEventHandlerWithProperties, 2, CELIX_ENOMEM);
    TestSubscribeEvent("org/celix/test");
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToSubscribeExistedTopicTest) {
    TestEventAdmin([](celix_event_admin_t *ea, celix_bundle_context_t *ctx) {
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

        auto handlerSvcId = celix_bundleContext_registerService(ctx, &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props);
        ASSERT_TRUE(handlerSvcId >= 0);

        auto props2 = celix_properties_create();
        celix_properties_set(props2, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props2, CELIX_EVENT_TOPIC, "org/celix/test");
        auto handlerSvcId2 = celix_bundleContext_registerService(ctx, &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props2);
        ASSERT_TRUE(handlerSvcId2 >= 0);

        celix_ei_expect_celix_arrayList_addLong((void*)&celix_eventAdmin_addEventHandlerWithProperties, 2, CELIX_ENOMEM, 2);

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
        long handlerTrkId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
        EXPECT_TRUE(handlerTrkId >= 0);

        celix_bundleContext_unregisterService(ctx, handlerSvcId);
        celix_bundleContext_unregisterService(ctx, handlerSvcId2);
        celix_bundleContext_stopTracker(ctx, handlerTrkId);
    });
}


TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCreateEventHandlerSetsWhenSendingEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        celix_ei_expect_celix_longHashMap_create((void*)&celix_eventAdmin_sendEvent, 1, nullptr);
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToCollectHandlersWhenSendingEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        celix_ei_expect_celix_longHashMap_put((void*)&celix_eventAdmin_sendEvent, 2, CELIX_ENOMEM);
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, SendEventToBlacklistHandlerTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        celix_ei_expect_celix_elapsedtime((void*)&celix_eventAdmin_sendEvent, 3, 61);
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = celix_eventAdmin_sendEvent(ea, "org/celix/test1", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STRNE("org/celix/test1", topic);
        return CELIX_SUCCESS;
    });
}


TEST_F(CelixEventAdminErrorInjectionTestSuite, FailedToPostEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        celix_ei_expect_calloc((void*)&celix_eventAdmin_postEvent, 2, nullptr);
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ENOMEM, status);

        celix_ei_expect_calloc((void*)&celix_event_create, 0, nullptr);
        status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ENOMEM, status);

        celix_ei_expect_celix_arrayList_add((void*)&celix_eventAdmin_postEvent, 2, CELIX_ENOMEM);
        status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ENOMEM, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminErrorInjectionTestSuite, PostEventToBlacklistHandlerTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        celix_ei_expect_celix_elapsedtime(CELIX_EI_UNKNOWN_CALLER, 0, 61);
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = celix_eventAdmin_postEvent(ea, "org/celix/test1", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STRNE("org/celix/test1", topic);
        return CELIX_SUCCESS;
    });
}