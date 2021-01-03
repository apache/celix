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

extern "C" {

#include "celix_launcher.h"
#include "celix_framework_factory.h"
#include "celix_framework.h"


    static celix_framework_t *framework = nullptr;
    static celix_bundle_context_t *context = nullptr;

    static void setupFm(void) {
        int rc = 0;

        rc = celixLauncher_launch("config.properties", &framework);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        bundle_pt bundle = nullptr;
        rc = framework_getFrameworkBundle(framework, &bundle);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &context);
        ASSERT_EQ(CELIX_SUCCESS, rc);
    }

    static void teardownFm(void) {

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);

        context = nullptr;
        framework = nullptr;
    }

    static void testFramework(void) {
        //intentional empty. start/shutdown test
        printf("testing startup/shutdown single framework\n");
    }

}


class CelixFramework : public ::testing::Test {
public:
    CelixFramework() {
        setupFm();
    }

    ~CelixFramework() override {
        teardownFm();
    }
};

TEST_F(CelixFramework, testFramework) {
    testFramework();
}

TEST_F(CelixFramework, testEventQueue) {
    long eid = celix_framework_nextEventId(framework);
    EXPECT_GE(eid, 0);
    celix_framework_waitForGenericEvent(framework, eid); //event never issued so should return directly

    std::atomic<int> count{0};
    celix_framework_fireGenericEvent(framework, eid, -1L, "test", static_cast<void*>(&count), [](void* data) {
       auto *c = static_cast<std::atomic<int>*>(data);
       *c += 1;
    }, static_cast<void*>(&count), [](void* data) {
        auto *c = static_cast<std::atomic<int>*>(data);
        *c += 3;
    });

    celix_framework_waitForGenericEvent(framework, eid);
    EXPECT_EQ(4, count);
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

