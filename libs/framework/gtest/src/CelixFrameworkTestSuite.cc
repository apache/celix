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
#include <atomic>
#include <chrono>
#include <thread>
#include <future>

#include "celix_launcher.h"
#include "celix_framework_factory.h"
#include "celix_framework.h"
#include "framework_private.h"
#include "celix_constants.h"
#include "celix_log.h"
#include "celix_utils.h"

class CelixFrameworkTestSuite : public ::testing::Test {
  public:
    CelixFrameworkTestSuite() {
        celix_properties_t* config;
        auto status = celix_properties_load("config.properties", 0, &config);
        EXPECT_EQ(CELIX_SUCCESS, status);

        auto fw = celix_frameworkFactory_createFramework(config);
        EXPECT_TRUE(fw != nullptr);

        framework = std::shared_ptr<celix_framework_t>{
            fw, [](celix_framework_t* cFw) { celix_frameworkFactory_destroyFramework(cFw); }};
    }

    std::shared_ptr<celix_framework_t> framework{};
};

TEST_F(CelixFrameworkTestSuite, FrameworkTest) {
    //nop
}

TEST_F(CelixFrameworkTestSuite, EventQueueTest) {
    long eid = celix_framework_nextEventId(framework.get());
    EXPECT_GE(eid, 0);
    celix_framework_waitForGenericEvent(framework.get(), eid); //event never issued so should return directly

    std::atomic<int> count{0};
    celix_framework_fireGenericEvent(framework.get(), eid, -1L, "test", static_cast<void*>(&count), [](void* data) {
       auto *c = static_cast<std::atomic<int>*>(data);
       *c += 1;
    }, static_cast<void*>(&count), [](void* data) {
        auto *c = static_cast<std::atomic<int>*>(data);
        *c += 3;
    });

    celix_framework_waitForGenericEvent(framework.get(), eid);
    EXPECT_EQ(4, count);
}

TEST_F(CelixFrameworkTestSuite, TimedWaitEventQueueTest) {
    //When there is a emtpy event queue
    celix_framework_waitForEmptyEventQueue(framework.get());

    std::promise<int> p;
    std::future<int> f = p.get_future();
    //And a generic event is fired, that block the queue until timeout
    auto callback = [](void* data) {
        auto* f = static_cast<std::future<std::string>*>(data);
        f->wait();
    };
    celix_framework_fireGenericEvent(framework.get(), -1L, -1L, "test", &f, callback, nullptr, nullptr);

    //Then a wait for empty event queue for max 5ms will return a timeout
    celix_status_t status = celix_framework_waitForEmptyEventQueueFor(framework.get(), 0.005);
    EXPECT_EQ(ETIMEDOUT, status) << "Expected timeout, but got " << celix_strerror(status);

    p.set_value(1);

    //And a wait for empty event queue for max 1s will return success
    status = celix_framework_waitForEmptyEventQueueFor(framework.get(), 1);
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(CelixFrameworkTestSuite, GenericEventTimeoutPropertyTest) {
    celix_properties_t* config;
    auto status = celix_properties_load("config.properties", 0, &config);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_properties_set(config, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL" , "warning");
    celix_properties_set(config, CELIX_ALLOWED_PROCESSING_TIME_FOR_GENERIC_EVENT_IN_SECONDS, "0.010");

    framework_t* fw = celix_frameworkFactory_createFramework(config);
    ASSERT_TRUE(fw != nullptr);

    std::promise<std::string> p;
    std::future<std::string> f = p.get_future();
    celix_frameworkLogger_setLogCallback(fw->logger, &p,
        [](void* handle, celix_log_level_e, const char*, const char *, int, const char *format, va_list formatArgs) {
            auto* p = static_cast<std::promise<std::string>*>(handle);
            // format std::string from format and formatArgs
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), format, formatArgs);
            auto log = std::string(buffer);
            auto expected = "Generic event 'test' (id=" + std::to_string(100) + ")";
            if (log.find(expected) != std::string::npos) {
                try {
                    p->set_value(log);
                } catch (std::future_error& e) {
                    EXPECT_EQ(std::future_errc::promise_already_satisfied, e.code());
                }
            }
    });

    celix_framework_waitForEmptyEventQueue(fw);
    
    //And a generic event is fired, that block the queue for 20ms
    auto callback = [](void* data) {
        auto* f = static_cast<std::future<std::string>*>(data);
        f->wait();
    };
    long eventId = celix_framework_fireGenericEvent(fw, 100L, -1L, "test", &f, callback, nullptr, nullptr);
    EXPECT_EQ(100L, eventId);

    //Then waiting for the event queue will have logged errors
    celix_framework_waitForGenericEvent(fw, eventId);

    // Cleanup framework
    celix_frameworkFactory_destroyFramework(fw);
}

TEST_F(CelixFrameworkTestSuite, AsyncInstallStartStopUpdateAndUninstallBundleTest) {
    long bndId = celix_framework_installBundleAsync(framework.get(), SIMPLE_TEST_BUNDLE1_LOCATION, false);
    EXPECT_GE(bndId, 0);
    EXPECT_TRUE(celix_framework_isBundleInstalled(framework.get(), bndId));
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_updateBundleAsync(framework.get(), bndId, nullptr);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_startBundleAsync(framework.get(), bndId);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_TRUE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_updateBundleAsync(framework.get(), bndId, nullptr);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_TRUE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_stopBundleAsync(framework.get(), bndId);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_updateBundleAsync(framework.get(), bndId, nullptr);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_unloadBundleAsync(framework.get(), bndId);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_FALSE(celix_framework_isBundleInstalled(framework.get(), bndId));

    // reloaded bundle should reuse the same bundle id
    EXPECT_EQ(bndId, celix_framework_installBundleAsync(framework.get(), SIMPLE_TEST_BUNDLE1_LOCATION, false));
    EXPECT_TRUE(celix_framework_isBundleInstalled(framework.get(), bndId));
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_uninstallBundleAsync(framework.get(), bndId);
    celix_framework_waitForBundleLifecycleHandlers(framework.get());
    EXPECT_FALSE(celix_framework_isBundleInstalled(framework.get(), bndId));
}

TEST_F(CelixFrameworkTestSuite, ListBundlesTest) {
    auto list = celix_framework_listBundles(framework.get());
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
    list = celix_framework_listInstalledBundles(framework.get());
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);

    long bndId = celix_framework_installBundle(framework.get(), SIMPLE_TEST_BUNDLE1_LOCATION, false);
    EXPECT_GT(bndId, 0);

    list = celix_framework_listBundles(framework.get());
    EXPECT_EQ(0, celix_arrayList_size(list)); //installed, but not started
    celix_arrayList_destroy(list);
    list = celix_framework_listInstalledBundles(framework.get());
    EXPECT_EQ(1, celix_arrayList_size(list));
    celix_arrayList_destroy(list);

    celix_framework_startBundle(framework.get(), bndId);

    list = celix_framework_listBundles(framework.get());
    EXPECT_EQ(1, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
    list = celix_framework_listInstalledBundles(framework.get());
    EXPECT_EQ(1, celix_arrayList_size(list));
    celix_arrayList_destroy(list);

    celix_framework_stopBundle(framework.get(), bndId);

    list = celix_framework_listBundles(framework.get());
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
    list = celix_framework_listInstalledBundles(framework.get());
    EXPECT_EQ(1, celix_arrayList_size(list)); //stopped, but still installed
    celix_arrayList_destroy(list);

    celix_framework_uninstallBundle(framework.get(), bndId);

    list = celix_framework_listBundles(framework.get());
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
    list = celix_framework_listInstalledBundles(framework.get());
    EXPECT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);
}

TEST_F(CelixFrameworkTestSuite, UseBundlesAndUseBundleTest) {
    celix_framework_t* fw = framework.get();

    long bndId1 = celix_framework_installBundle(fw, SIMPLE_TEST_BUNDLE1_LOCATION, true);
    EXPECT_GT(bndId1, CELIX_FRAMEWORK_BUNDLE_ID);
    long bndId2 = celix_framework_installBundle(fw, SIMPLE_TEST_BUNDLE2_LOCATION, false);
    EXPECT_GT(bndId2, CELIX_FRAMEWORK_BUNDLE_ID);

    int callBackCount = 0;
    auto callback = [](void* handle, const celix_bundle_t* bnd) {
        auto* ct = static_cast<int*>(handle);
        *ct += 1;
        EXPECT_GE(celix_bundle_getId(bnd), CELIX_FRAMEWORK_BUNDLE_ID);
    };

    size_t useCount = celix_framework_useBundles(fw, true, &callBackCount, callback);
    EXPECT_EQ(3, useCount); //2 bundles + framework

    auto nop = [](void* /*handle*/, const celix_bundle_t* /*bnd*/) {};

    //test use framework bundle
    bool called = celix_framework_useBundle(fw, true, CELIX_FRAMEWORK_BUNDLE_ID, nullptr, nop);
    EXPECT_TRUE(called);

    //test use active bundle
    called = celix_framework_useBundle(fw, true, bndId1, nullptr, nop);
    EXPECT_TRUE(called);
    called = celix_framework_useBundle(fw, false, bndId1, nullptr, nop);
    EXPECT_TRUE(called);

    //test use inactive bundle
    called = celix_framework_useBundle(fw, true, bndId2, nullptr, nop);
    EXPECT_FALSE(called); //note bnd2 is not active
    called = celix_framework_useBundle(fw, false, bndId2, nullptr, nop);
    EXPECT_TRUE(called);
}

class FrameworkFactoryTestSuite : public ::testing::Test {
public:
    FrameworkFactoryTestSuite() = default;
    ~FrameworkFactoryTestSuite() override = default;
};


TEST_F(FrameworkFactoryTestSuite, FactoryCreateTest) {
    framework_t* fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    celix_frameworkFactory_destroyFramework(fw);
}

TEST_F(FrameworkFactoryTestSuite, ConfigureFrameworkUUIDTest) {
    celix_properties_t* config = celix_properties_create();
    celix_properties_set(config, CELIX_FRAMEWORK_UUID, "test-framework");
    framework_t* fw = celix_frameworkFactory_createFramework(config);
    ASSERT_TRUE(fw != nullptr);
    EXPECT_STREQ("test-framework", celix_framework_getUUID(fw));
    celix_frameworkFactory_destroyFramework(fw);
}

TEST_F(FrameworkFactoryTestSuite, FactoryCreateAndToManyStartAndStopsTest) {
    framework_t* fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);

    framework_start(fw); //should already be done by frameworkFactory_newFramework();
    framework_start(fw);
    framework_start(fw);
    framework_start(fw);

    framework_stop(fw); //will also be implicitly done by framework_destroy
    framework_stop(fw);
    framework_stop(fw);
    framework_stop(fw);

    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw); //note stop, wait and then destroy is needed .. combine ?
}

TEST_F(FrameworkFactoryTestSuite, RecreateFrameworkTest) {
    framework_t* fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);

    fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);

    fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);

    fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);

    fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);
}

TEST_F(FrameworkFactoryTestSuite, RestartFrameworkTest) {
    framework_t* fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    framework_stop(fw);
    framework_waitForStop(fw);

    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);

    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);

    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);

    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);

    framework_start(fw);
    framework_stop(fw);
    framework_waitForStop(fw);

    framework_destroy(fw);
}


TEST_F(FrameworkFactoryTestSuite, LaunchFrameworkWithConfigTest) {
    /* Rule: When a Celix framework is started with a config for auto starting bundles and installing bundles,
     * the specified bundles will be installed and - if needed - started.
     */

    celix_properties_t* config = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, celix_properties_load(INSTALL_AND_START_BUNDLES_CONFIG_PROPERTIES_FILE, 0, &config));
    ASSERT_TRUE(config != nullptr);

    framework_t* fw = celix_frameworkFactory_createFramework(config);
    ASSERT_TRUE(fw != nullptr);

    auto* startedBundleIds = celix_framework_listBundles(fw);
    auto* installedBundleIds = celix_framework_listInstalledBundles(fw);

    /*
     * 3 started: simple_test_bundle1, simple_test_bundle2 and simple_test_bundle3
     */
    EXPECT_EQ(celix_arrayList_size(startedBundleIds), 3);

    /*
     * 3 started: simple_test_bundle1, simple_test_bundle2 and simple_test_bundle3
     * 2 installed: simple_test_bundle4 and simple_test_bundle5
     */
    EXPECT_EQ(celix_arrayList_size(installedBundleIds), 5);

    celix_arrayList_destroy(startedBundleIds);
    celix_arrayList_destroy(installedBundleIds);

    framework_stop(fw);
    framework_waitForStop(fw);
    framework_destroy(fw);
}

TEST_F(FrameworkFactoryTestSuite, BundleWithErrMessageTest) {
    // Given a framework
    auto* fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);

    // When installing a bundle with an activator that pushes an error message to celix err and returns an error
    // during stop if there is still a message in the celix err queue.
    long bndId = celix_framework_installBundle(fw, CELIX_ERR_TEST_BUNDLE, true);

    // Then the bundle installs
    EXPECT_GT(bndId, CELIX_FRAMEWORK_BUNDLE_ID);

    // And the bundle stops without an error return code, because the celix err message count is 0 -> error is printed
    // by the framework
    bool stopped = celix_framework_stopBundle(fw, bndId);
    EXPECT_TRUE(stopped);
}
