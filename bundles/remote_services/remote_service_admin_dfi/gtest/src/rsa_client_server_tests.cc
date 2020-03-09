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

#include "gtest/gtest.h"
#include <remote_constants.h>
#include <tst_service.h>
#include "celix_api.h"

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "celix_launcher.h"
#include "framework.h"
#include "remote_service_admin.h"

    static celix_framework_t *serverFramework = NULL;
    static celix_bundle_context_t *serverContext = NULL;

    static celix_framework_t *clientFramework = NULL;
    static celix_bundle_context_t *clientContext = NULL;

    static void setupFm(void) {
        //server
        celix_properties_t *serverProps = celix_properties_load("server.properties");
        ASSERT_TRUE(serverProps != NULL);
        serverFramework = celix_frameworkFactory_createFramework(serverProps);
        ASSERT_TRUE(serverFramework != NULL);
        serverContext = celix_framework_getFrameworkContext(serverFramework);
        ASSERT_TRUE(serverContext != NULL);

        //client
        celix_properties_t *clientProperties = celix_properties_load("client.properties");
        ASSERT_TRUE(clientProperties != NULL);
        clientFramework = celix_frameworkFactory_createFramework(clientProperties);
        ASSERT_TRUE(clientFramework != NULL);
        clientContext = celix_framework_getFrameworkContext(clientFramework);
        ASSERT_TRUE(clientContext != NULL);
    }

    static void teardownFm(void) {
        celix_frameworkFactory_destroyFramework(serverFramework);
        celix_frameworkFactory_destroyFramework(clientFramework);
    }

    static void testCallback(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool ok;

        bool discovered = tst->isCalcDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        ok = tst->testCalculator(tst->handle);
        ASSERT_TRUE(ok);

        discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        ok = tst->testRemoteComplex(tst->handle);
        ASSERT_TRUE(ok);

        ok = tst->testRemoteAction(tst->handle);
        ASSERT_TRUE(ok);

        ok = tst->testRemoteNumbers(tst->handle);
        ASSERT_TRUE(ok);

        ok = tst->testRemoteString(tst->handle);
        ASSERT_TRUE(ok);

        ok = tst->testRemoteConstString(tst->handle);
        ASSERT_TRUE(ok);

        //TODO fix for apple dfi handling, see issue #91
//        ok = tst->testRemoteEnum(tst->handle);
//        ASSERT_TRUE(ok);
    };

    static void test(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = TST_SERVICE_NAME;
        opts.use = testCallback;
        opts.filter.ignoreServiceLanguage = true;
        opts.waitTimeoutInSeconds = 2;
        bool called = celix_bundleContext_useServiceWithOptions(clientContext, &opts);
        ASSERT_TRUE(called);
    }

}

class RsaDfiClientServerTests : public ::testing::Test {
public:
    RsaDfiClientServerTests() {
        setupFm();
    }
    ~RsaDfiClientServerTests() override {
        teardownFm();
    }

};


TEST_F(RsaDfiClientServerTests, Test1) {
    test();
}
