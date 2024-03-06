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
#include <unistd.h>
#include <semaphore.h>

#include <gtest/gtest.h>
#include "CelixEventAdminTestSuiteBaseClass.h"
#include "celix_event_admin.h"
#include "celix_event.h"
#include "celix_event_constants.h"
#include "celix_constants.h"

static sem_t g_sem;
static bool g_eventHandledFlag = false;

static bool WaitForEventDone(int timeoutInSeconds) {
#ifdef __APPLE__
    int sleepCnt = timeoutInSeconds * 1000 / 10;
    while (g_eventHandledFlag == false && sleepCnt > 0) {
        usleep(10000);
        sleepCnt--;
    }
    return g_eventHandledFlag;
#else
    struct timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutInSeconds;
    return sem_timedwait(&g_sem, &ts) == 0;
#endif
}

static void HandleEventDone() {
    sem_post(&g_sem);
    g_eventHandledFlag = true;
}

class CelixEventAdminTestSuite : public CelixEventAdminTestSuiteBaseClass {
public:
    CelixEventAdminTestSuite() {
        sem_init(&g_sem, 0, 0);
        g_eventHandledFlag = false;
    }

    ~CelixEventAdminTestSuite() override {
        sem_destroy(&g_sem);
    }
};

TEST_F(CelixEventAdminTestSuite, CreateEventAdminTest) {
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_TRUE(ea != nullptr);
    celix_eventAdmin_destroy(ea);
}

TEST_F(CelixEventAdminTestSuite, StartStopEventAdminTest) {
    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_TRUE(ea != nullptr);

    auto status = celix_eventAdmin_start(ea);
    EXPECT_EQ(CELIX_SUCCESS, status);

    status = celix_eventAdmin_stop(ea);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_eventAdmin_destroy(ea);
}

TEST_F(CelixEventAdminTestSuite, SubscribeAllTopicTest) {
    TestSubscribeEvent("*");
}

TEST_F(CelixEventAdminTestSuite, SubscribeExactTopicTest) {
    TestSubscribeEvent("org/celix/test");
}

TEST_F(CelixEventAdminTestSuite, SubscribePrefixTopicTest) {
    TestSubscribeEvent("org/celix/*");
}

TEST_F(CelixEventAdminTestSuite, SubscribeMultipleTopicsTest) {
    TestSubscribeEvent("org/celix/test,org/celix/test2");
}

TEST_F(CelixEventAdminTestSuite, SendEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventInHandlerTest) {
    TestEventAdmin([](celix_event_admin_t *ea, celix_bundle_context_t *ctx) {
        celix_event_handler_service_t handler;
        handler.handle = ea;
        handler.handleEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
            (void)handle;
            (void)props;
            EXPECT_STREQ("org/celix/test", topic);
            auto status = celix_eventAdmin_sendEvent(handle, "org/celix/test2", nullptr);
            EXPECT_EQ(CELIX_SUCCESS, status);
            return CELIX_SUCCESS;
        };
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "org/celix/test");

        auto handlerSvcId = celix_bundleContext_registerService(ctx, &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props);
        ASSERT_TRUE(handlerSvcId >= 0);

        celix_event_handler_service_t handler2;
        handler2.handle = ea;
        handler2.handleEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
            (void)handle;
            (void)props;
            EXPECT_STREQ("org/celix/test2", topic);
            return CELIX_SUCCESS;
        };
        auto props2 = celix_properties_create();
        celix_properties_set(props2, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props2, CELIX_EVENT_TOPIC, "org/celix/test2");
        auto handlerSvcId2 = celix_bundleContext_registerService(ctx, &handler2, CELIX_EVENT_HANDLER_SERVICE_NAME, props2);
        ASSERT_TRUE(handlerSvcId2 >= 0);

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

        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);

        celix_bundleContext_unregisterService(ctx, handlerSvcId);
        celix_bundleContext_unregisterService(ctx, handlerSvcId2);
        celix_bundleContext_stopTracker(ctx, handlerTrkId);
    });

}

TEST_F(CelixEventAdminTestSuite, SendEventWithPropertiesTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_properties_destroy(props);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        EXPECT_STREQ("org/celix/test", topic);
        EXPECT_STREQ("value", celix_properties_get(props, "key", nullptr));
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventWithFilterTest) {
    TestPublishEvent("org/celix/test", "(key=value)", [](celix_event_admin_t *ea) {
        auto props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_properties_destroy(props);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        EXPECT_STREQ("org/celix/test", topic);
        EXPECT_STREQ("value", celix_properties_get(props, "key", nullptr));
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventWithFilterNoMatchTest) {
    TestPublishEvent("org/celix/test", "(key=value)", [](celix_event_admin_t *ea) {
        auto props = celix_properties_create();
        celix_properties_set(props, "key", "value2");
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_properties_destroy(props);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)topic;
        (void)props;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventWithPrefixTopicHandlerTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventWithPrefixTopicHandlerTest2) {
    TestPublishEvent("org/*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventWithPrefixTopicHandlerNoMatchTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix2/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)topic;
        (void)props;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, SendEventWithMatchingAllTopicHandlerTest) {
    TestPublishEvent("*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_sendEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, HandlerPostEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        static int count = 0;
        if (count == 0) {
            count += 1;
            auto status = celix_eventAdmin_postEvent(handle, "org/celix/test", nullptr);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithPropertiesTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_properties_destroy(props);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        EXPECT_STREQ("org/celix/test", topic);
        EXPECT_STREQ("value", celix_properties_get(props, "key", nullptr));
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithFilterTest) {
    TestPublishEvent("org/celix/test", "(key=value)", [](celix_event_admin_t *ea) {
        auto props = celix_properties_create();
        celix_properties_set(props, "key", "value");
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_properties_destroy(props);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        EXPECT_STREQ("org/celix/test", topic);
        EXPECT_STREQ("value", celix_properties_get(props, "key", nullptr));
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithFilterNoMatchTest) {
    TestPublishEvent("org/celix/test", "(key=value)", [](celix_event_admin_t *ea) {
        auto props = celix_properties_create();
        celix_properties_set(props, "key", "value2");
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_properties_destroy(props);
        usleep(10000);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)topic;
        (void)props;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithPrefixTopicHandlerTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithPrefixTopicHandlerTest2) {
    TestPublishEvent("org/*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithPrefixTopicHandlerNoMatchTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, "org/celix2/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        usleep(10000);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)topic;
        (void)props;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithMatchingAllTopicHandlerTest) {
    TestPublishEvent("*", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostMutilEventTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        for (int i = 0; i < 10; ++i) {
            auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        EXPECT_STREQ("org/celix/test", topic);
        HandleEventDone();
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostMutilEventToUnorderedHandlerTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        for (int i = 0; i < 10; ++i) {
            auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        auto eventDone = WaitForEventDone(30);
        EXPECT_TRUE(eventDone);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        usleep(10000);
        EXPECT_STREQ("org/celix/test", topic);
        HandleEventDone();
        return CELIX_SUCCESS;
    }, true);
}

TEST_F(CelixEventAdminTestSuite, MutilpleEventHandlerSubscribeMutilpleTopicTest) {
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

        auto props3 = celix_properties_create();
        celix_properties_set(props3, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
        celix_properties_set(props3, CELIX_EVENT_TOPIC, "org/celix/test2");
        auto handlerSvcId3 = celix_bundleContext_registerService(ctx, &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props3);
        ASSERT_TRUE(handlerSvcId3 >= 0);

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
        celix_bundleContext_unregisterService(ctx, handlerSvcId3);
        celix_bundleContext_stopTracker(ctx, handlerTrkId);
    });
}

TEST_F(CelixEventAdminTestSuite, EventHandlerNoTopicTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> propsCopy{celix_properties_copy(props), celix_properties_destroy};
        celix_properties_unset(propsCopy.get(), CELIX_EVENT_TOPIC);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, propsCopy.get());
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> propsCopy{celix_properties_copy(props), celix_properties_destroy};
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, propsCopy.get());
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(CelixEventAdminTestSuite, EventHandlerNoServiceIdTest) {
    TestAddEventHandler([](void *handle, void *svc, const celix_properties_t *props) {
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> propsCopy{celix_properties_copy(props), celix_properties_destroy};
        celix_properties_unset(propsCopy.get(), CELIX_FRAMEWORK_SERVICE_ID);
        auto status = celix_eventAdmin_addEventHandlerWithProperties(handle, svc, propsCopy.get());
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }, [](void *handle, void *svc, const celix_properties_t *props) {
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> propsCopy{celix_properties_copy(props), celix_properties_destroy};
        celix_properties_unset(propsCopy.get(), CELIX_FRAMEWORK_SERVICE_ID);
        auto status = celix_eventAdmin_removeEventHandlerWithProperties(handle, svc, propsCopy.get());
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    });
}

TEST_F(CelixEventAdminTestSuite, PrefixTopicTooLongTest) {
    TestSubscribeEvent("org/celix/topic-too-long----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------/*");
}

TEST_F(CelixEventAdminTestSuite, SendEventWithInvalidArgumentsTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_sendEvent(ea, nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        status = celix_eventAdmin_sendEvent(nullptr, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, EventTopicTooLongForPrefixTopicHandlerTest) {
    TestPublishEvent("org/celix/*", nullptr, [](celix_event_admin_t *ea) {
        const char *topic = "org/celix/topic-too-long----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
        auto status = celix_eventAdmin_sendEvent(ea, topic, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, PostEventWithInvalidArgumentsTest) {
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        auto status = celix_eventAdmin_postEvent(ea, nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        status = celix_eventAdmin_postEvent(nullptr, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        ADD_FAILURE() << "Should not be called";
        return CELIX_SUCCESS;
    });
}

static bool g_blockingHandlerCalled = false;
TEST_F(CelixEventAdminTestSuite, AsyncEventQueueFullTest) {
    g_blockingHandlerCalled = true;
    TestPublishEvent("org/celix/test", nullptr, [](celix_event_admin_t *ea) {
        for (int i = 0; i < 512 + 1/*handling event*/; ++i) {
            auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        usleep(30000);
        auto status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, status);
        g_blockingHandlerCalled = false;
    }, [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)props;
        (void)topic;
        while (g_blockingHandlerCalled) usleep(1000);
        return CELIX_SUCCESS;
    });
}

TEST_F(CelixEventAdminTestSuite, RemoveEventHandlerAfterEventAdminStopTest) {
    g_blockingHandlerCalled = true;
    celix_event_handler_service_t handler;
    handler.handle = nullptr;
    handler.handleEvent = [](void *handle, const char *topic, const celix_properties_t *props) {
        (void)handle;
        (void)topic;
        (void)props;
        while (g_blockingHandlerCalled) usleep(1000);
        return CELIX_SUCCESS;
    };
    auto props = celix_properties_create();
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_EVENT_HANDLER_SERVICE_VERSION);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "org/celix/test");

    auto handlerSvcId = celix_bundleContext_registerService(ctx.get(), &handler, CELIX_EVENT_HANDLER_SERVICE_NAME, props);
    ASSERT_TRUE(handlerSvcId >= 0);

    auto ea = celix_eventAdmin_create(ctx.get());
    EXPECT_TRUE(ea != nullptr);
    auto status = celix_eventAdmin_start(ea);
    EXPECT_EQ(CELIX_SUCCESS, status);

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

    for (int i = 0; i < 100; ++i) {
        status = celix_eventAdmin_postEvent(ea, "org/celix/test", nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }

    g_blockingHandlerCalled = false;
    status = celix_eventAdmin_stop(ea);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_unregisterService(ctx.get(), handlerSvcId);
    celix_bundleContext_stopTracker(ctx.get(), handlerTrkId);

    celix_eventAdmin_destroy(ea);


}