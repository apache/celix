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
#include "framework.h"
#include "celix_constants.h"
#include "celix_utils.h"


class CelixFrameworkTestSuite : public ::testing::Test {
public:
    CelixFrameworkTestSuite() {
        int rc;
        celix_framework_t *fw = nullptr;
        celix_bundle_context_t *context = nullptr;

        rc = celixLauncher_launch("config.properties", &fw);
        EXPECT_EQ(CELIX_SUCCESS, rc);

        celix_bundle_t* bundle = celix_framework_getFrameworkBundle(fw);
        EXPECT_TRUE(bundle != nullptr);

        context = celix_framework_getFrameworkContext(fw);
        EXPECT_TRUE(context != nullptr);

        framework = std::shared_ptr<celix_framework_t>{fw, [](celix_framework_t* cFw) {
            celixLauncher_stop(cFw);
            celixLauncher_waitForShutdown(cFw);
            celixLauncher_destroy(cFw);
        }};
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

    //And a generic event is fired, that block the queue for 20ms
    auto callback = [](void* /*data*/) {
        std::this_thread::sleep_for(std::chrono::milliseconds{200});
    };
    celix_framework_fireGenericEvent(framework.get(), -1L, -1L, "test", nullptr, callback, nullptr, nullptr);

    //Then a wait for empty event queue for max 5ms will return a timeout
    celix_status_t status = celix_framework_waitForEmptyEventQueueFor(framework.get(), 0.005);
    EXPECT_EQ(ETIMEDOUT, status) << "Expected timeout, but got " << celix_strerror(status);

    //And a wait for empty event queue for max 1s will return success
    status = celix_framework_waitForEmptyEventQueueFor(framework.get(), 1);
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(CelixFrameworkTestSuite, AsyncInstallStartStopUpdateAndUninstallBundleTest) {
    long bndId = celix_framework_installBundleAsync(framework.get(), SIMPLE_TEST_BUNDLE1_LOCATION, false);
    EXPECT_GE(bndId, 0);
    EXPECT_TRUE(celix_framework_isBundleInstalled(framework.get(), bndId));
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_updateBundleAsync(framework.get(), bndId, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_startBundleAsync(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_TRUE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_updateBundleAsync(framework.get(), bndId, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_TRUE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_stopBundleAsync(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_updateBundleAsync(framework.get(), bndId, NULL);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_unloadBundleAsync(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_FALSE(celix_framework_isBundleInstalled(framework.get(), bndId));

    // reloaded bundle should reuse the same bundle id
    EXPECT_EQ(bndId, celix_framework_installBundleAsync(framework.get(), SIMPLE_TEST_BUNDLE1_LOCATION, false));
    EXPECT_TRUE(celix_framework_isBundleInstalled(framework.get(), bndId));
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_uninstallBundleAsync(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
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

    auto* config = celix_properties_load(INSTALL_AND_START_BUNDLES_CONFIG_PROPERTIES_FILE);
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
