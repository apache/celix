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
#include <future>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include <mosquitto.h>

#include "celix_framework_utils.h"
#include "celix_constants.h"
#include "celix_log_constants.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_event_constants.h"
#include "celix_event_admin_service.h"
#include "celix_event_handler_service.h"
#include "celix_earpm_constants.h"


class CelixEarpmIntegrationTestSuite : public ::testing::Test  {
public:
    static void SetUpTestSuite() {
        mosquitto_lib_init();
        pid = fork();
        ASSERT_GE(pid, 0);
        if (pid == 0) {
            execlp("mosquitto", "mosquitto", "-c", MOSQUITTO_CONF, nullptr);
            ADD_FAILURE() << "Failed to start mosquitto";
        }
    }
    static void TearDownTestSuite() {
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        mosquitto_lib_cleanup();
    }
    CelixEarpmIntegrationTestSuite() {
        {
            auto props = celix_properties_create();
            celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
            celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".earpm_publisher");
            celix_properties_set(props, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "trace");
            celix_properties_set(props, CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF);
            publisherFw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(props),
                                                             celix_frameworkFactory_destroyFramework};
            publisherCtx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(publisherFw.get()),
                                                                   [](celix_bundle_context_t *){/*nop*/}};
            celix_framework_utils_installBundleSet(publisherFw.get(), INTEGRATED_BUNDLES, true);
        }
        {
            auto props = celix_properties_create();
            celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
            celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".earpm_subscriber");
            celix_properties_set(props, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "trace");
            celix_properties_set(props, CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF);
            subscriberFw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(props),
                                                              celix_frameworkFactory_destroyFramework};
            subscriberCtx = std::shared_ptr<celix_bundle_context_t>{
                    celix_framework_getFrameworkContext(subscriberFw.get()), [](celix_bundle_context_t *) {/*nop*/}};
            celix_framework_utils_installBundleSet(subscriberFw.get(), INTEGRATED_BUNDLES, true);
        }
    }

    ~CelixEarpmIntegrationTestSuite() override = default;

    void TestEventPublish(bool testAsyncEvent) {
        std::promise<void> receivedEventPromise;
        std::future<void> receivedEventFuture = receivedEventPromise.get_future();
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "testEvent");
        celix_event_handler_service_t handler = {
                .handle = &receivedEventPromise,
                .handleEvent = [](void* handle, const char* topic, const celix_properties_t* properties) {
                    EXPECT_STREQ("testEvent", topic);
                    EXPECT_STREQ("value", celix_properties_get(properties, "key", ""));
                    auto promise = static_cast<std::promise<void> *>(handle);
                    try {
                        promise->set_value();
                    } catch (...) {
                        //ignore
                    }
                    return CELIX_SUCCESS;
                }
        };
        celix_service_registration_options_t opts{};
        opts.svc = &handler;
        opts.serviceName = CELIX_EVENT_HANDLER_SERVICE_NAME;
        opts.serviceVersion = CELIX_EVENT_HANDLER_SERVICE_VERSION;
        opts.properties = props;
        long handlerServiceId = celix_bundleContext_registerServiceWithOptions(subscriberCtx.get(), &opts);
        ASSERT_GE(handlerServiceId, 0);

        struct use_service_callback_handle {
            bool publishAsyncEvent;
            std::future<void>& future;
        };
        struct use_service_callback_handle handle{testAsyncEvent, receivedEventFuture};
        celix_service_use_options_t useOpts{};
        useOpts.filter.serviceName = CELIX_EVENT_ADMIN_SERVICE_NAME;
        useOpts.filter.versionRange = CELIX_EVENT_ADMIN_SERVICE_USE_RANGE;
        useOpts.callbackHandle = &handle;
        useOpts.waitTimeoutInSeconds = 30;
        useOpts.use = [](void* handle, void* svc) {
            celix_event_admin_service_t *eventAdmin = static_cast<celix_event_admin_service_t *>(svc);
            celix_autoptr(celix_properties_t) props = celix_properties_create();
            celix_properties_set(props, CELIX_EVENT_TOPIC, "testEvent");
            celix_properties_set(props, "key", "value");
            celix_properties_setBool(props, CELIX_EVENT_REMOTE_ENABLE, true);
            auto callbackHandle = static_cast<struct use_service_callback_handle*>(handle);
            int tryCount = 300;
            while (tryCount-- > 0) {//wait remote handler online, and try again.
                auto status = callbackHandle->publishAsyncEvent ?
                              eventAdmin->postEvent(eventAdmin->handle, "testEvent", props) :
                              eventAdmin->sendEvent(eventAdmin->handle, "testEvent", props);
                EXPECT_EQ(CELIX_SUCCESS, status);
                auto futureStatus = callbackHandle->future.wait_for(std::chrono::milliseconds{100});
                if (futureStatus == std::future_status::ready) {
                    break;
                }
            }
            EXPECT_TRUE(tryCount >= 0);
        };
        auto found= celix_bundleContext_useServiceWithOptions(publisherCtx.get(), &useOpts);
        EXPECT_TRUE(found);

        celix_bundleContext_unregisterService(subscriberCtx.get(), handlerServiceId);
    }

    static pid_t pid;
    std::shared_ptr<celix_framework_t> publisherFw{};
    std::shared_ptr<celix_bundle_context_t> publisherCtx{};
    std::shared_ptr<celix_framework_t> subscriberFw{};
    std::shared_ptr<celix_bundle_context_t> subscriberCtx{};
};
pid_t CelixEarpmIntegrationTestSuite::pid{0};

TEST_F(CelixEarpmIntegrationTestSuite, SendEvent) {
    TestEventPublish(false);
}

TEST_F(CelixEarpmIntegrationTestSuite, PostEvent) {
    TestEventPublish(true);
}