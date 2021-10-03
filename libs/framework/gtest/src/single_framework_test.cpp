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

#include "celix_launcher.h"
#include "celix_framework_factory.h"
#include "celix_framework.h"
#include "framework.h"


class CelixFramework : public ::testing::Test {
public:
    CelixFramework() {
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

TEST_F(CelixFramework, testFramework) {
    //nop
}

TEST_F(CelixFramework, testEventQueue) {
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

TEST_F(CelixFramework, testAsyncInstallStartStopAndUninstallBundle) {
    long bndId = celix_framework_installBundleAsync(framework.get(), SIMPLE_TEST_BUNDLE1_LOCATION, false);
    EXPECT_GE(bndId, 0);
    EXPECT_TRUE(celix_framework_isBundleInstalled(framework.get(), bndId));
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_startBundle(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_TRUE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_stopBundle(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_FALSE(celix_framework_isBundleActive(framework.get(), bndId));

    celix_framework_uninstallBundleAsync(framework.get(), bndId);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    EXPECT_FALSE(celix_framework_isBundleInstalled(framework.get(), bndId));
}

class FrameworkFactory : public ::testing::Test {
public:
    FrameworkFactory() = default;
    ~FrameworkFactory() override = default;
};


TEST_F(FrameworkFactory, testFactoryCreate) {
    framework_t* fw = celix_frameworkFactory_createFramework(nullptr);
    ASSERT_TRUE(fw != nullptr);
    celix_frameworkFactory_destroyFramework(fw);
}

TEST_F(FrameworkFactory, testFactoryCreateAndToManyStartAndStops) {
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

TEST_F(FrameworkFactory, recreateFramework) {
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

TEST_F(FrameworkFactory, restartFramework) {
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

