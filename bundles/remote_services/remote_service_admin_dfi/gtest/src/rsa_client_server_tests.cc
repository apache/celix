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
#include <tst_service.h>
#include <remote_custom_serialization_service.h>
#include <calculator_service.h>
#include "celix_api.h"
#include "../../../examples/remote_example_api/include/remote_example.h"

extern "C" {
#include <cstdlib>
#include <cstring>

#include "celix_properties.h"
#include "framework.h"
#include "remote_service_admin.h"

    static celix_framework_t *serverFramework = nullptr;
    static celix_bundle_context_t *serverContext = nullptr;

    static celix_framework_t *clientFramework = nullptr;
    static celix_bundle_context_t *clientContext = nullptr;

    static void setupFm() {
        //server
        celix_properties_t *serverProps = celix_properties_load("server.properties");
        ASSERT_TRUE(serverProps != nullptr);
        serverFramework = celix_frameworkFactory_createFramework(serverProps);
        ASSERT_TRUE(serverFramework != nullptr);
        serverContext = celix_framework_getFrameworkContext(serverFramework);
        ASSERT_TRUE(serverContext != nullptr);

        //client
        celix_properties_t *clientProperties = celix_properties_load("client.properties");
        ASSERT_TRUE(clientProperties != nullptr);
        clientFramework = celix_frameworkFactory_createFramework(clientProperties);
        ASSERT_TRUE(clientFramework != nullptr);
        clientContext = celix_framework_getFrameworkContext(clientFramework);
        ASSERT_TRUE(clientContext != nullptr);
    }

    static void teardownFm() {
        celix_frameworkFactory_destroyFramework(serverFramework);
        celix_frameworkFactory_destroyFramework(clientFramework);
    }

    static void testComplex(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteComplex(tst->handle);
        ASSERT_TRUE(ok);
    }

    bool customDeleteCalled;
    static void testComplexCustomSerialization(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        customDeleteCalled = false;
        bool ok = tst->testRemoteComplex(tst->handle);
        ASSERT_TRUE(ok);
        ASSERT_TRUE(customDeleteCalled);
    }

    static void testAction(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteAction(tst->handle);
        ASSERT_TRUE(ok);
    }

    static void testNumbers(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteNumbers(tst->handle);
        ASSERT_TRUE(ok);
    }

    static void testString(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteString(tst->handle);
        ASSERT_TRUE(ok);
    }

    static void testEnum(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteEnum(tst->handle);
        ASSERT_TRUE(ok);
    }

    static void testConstString(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteConstString(tst->handle);
        ASSERT_TRUE(ok);
    }

    static void testCalculator(void *handle __attribute__((unused)), void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool ok;

        bool discovered = tst->isCalcDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        ok = tst->testCalculator(tst->handle);
        ASSERT_TRUE(ok);
    }

}

template<typename F>
static void test(F&& f) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = TST_SERVICE_NAME;
    opts.use = f;
    opts.filter.ignoreServiceLanguage = true;
    opts.waitTimeoutInSeconds = 2;
    bool called = celix_bundleContext_useServiceWithOptions(clientContext, &opts);
    ASSERT_TRUE(called);
}

static std::pair<rsa_custom_serialization_service*, long> setupCustomSerializationService() {
    long targetServiceId = -1;
    celix_service_use_options_t useOpts{};
    useOpts.callbackHandle = &targetServiceId;
    useOpts.useWithProperties = [](void *handle, void *, const celix_properties_t *props){
        long *targetServiceId = (long*)handle;
        *targetServiceId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);
    };
    useOpts.filter.serviceName = REMOTE_EXAMPLE_NAME;
    useOpts.filter.ignoreServiceLanguage = true;
    auto called = celix_bundleContext_useServiceWithOptions(serverContext, &useOpts);
    if(!called || targetServiceId < 0) {
        throw std::runtime_error("!called || targetServiceId < 0");
    }
    printf("custom delete target service id %li\n", targetServiceId);

    auto *customSvc = new rsa_custom_serialization_service;
    customSvc->deleteType = [](const char *, void* instance) -> celix_status_t {
        auto *output = (complex_output_example*)instance;
        free(output->name);
        free(output);
        customDeleteCalled = true;
        return CELIX_SUCCESS;
    };

    celix_properties_t *props = celix_properties_create();
    celix_properties_setLong(props, RSA_SERVICE_TARGETED_ID, targetServiceId);

    celix_service_registration_options_t opts{};
    opts.serviceName = RSA_CUSTOM_SERIALIZATION_SERVICE_NAME;
    opts.svc = customSvc;
    opts.properties = props;

    long customSvcId = celix_bundleContext_registerServiceWithOptions(serverContext, &opts);
    if(customSvcId < 0) {
        throw std::runtime_error("customSvcId < 0");
    }
    return {customSvc, customSvcId};
}

static void stopCustomSerializationService(std::pair<rsa_custom_serialization_service*, long> setupVars) {
    celix_bundleContext_unregisterService(serverContext, setupVars.second);
    delete setupVars.first;
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


TEST_F(RsaDfiClientServerTests, TestRemoteCalculator) {
    test(testCalculator);
}

TEST_F(RsaDfiClientServerTests, TestRemoteComplex) {
    test(testComplex);
}

TEST_F(RsaDfiClientServerTests, TestRemoteComplexCustomSerialization) {
    auto setupVars = setupCustomSerializationService();
    test(testComplexCustomSerialization);
    stopCustomSerializationService(setupVars);
}

TEST_F(RsaDfiClientServerTests, TestRemoteNumbers) {
    test(testNumbers);
}

TEST_F(RsaDfiClientServerTests, TestRemoteString) {
    test(testString);
}

TEST_F(RsaDfiClientServerTests, TestRemoteConstString) {
    test(testConstString);
}

TEST_F(RsaDfiClientServerTests, TestRemoteEnum) {
    test(testEnum);
}

TEST_F(RsaDfiClientServerTests, TestRemoteAction) {
    test(testAction);
}
