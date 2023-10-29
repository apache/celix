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
#include "celix_compiler.h"

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
#include "remote_interceptor.h"

#define RSA_DIF_EXCEPTION_TEST_SERVICE "exception_test_service"
typedef struct rsa_dfi_exception_test_service {
    void *handle;
    int (*func1)(void *handle);
}rsa_dfi_exception_test_service_t;

    static celix_framework_t *serverFramework = NULL;
    static celix_bundle_context_t *serverContext = NULL;

    static celix_framework_t *clientFramework = NULL;
    static celix_bundle_context_t *clientContext = NULL;

    static rsa_dfi_exception_test_service_t *exceptionTestService = NULL;
    static long exceptionTestSvcId = -1L;
    static remote_interceptor_t *serverSvcInterceptor=NULL;
    static remote_interceptor_t *clientSvcInterceptor=NULL;
    static long serverSvcInterceptorSvcId = -1L;
    static long clientSvcInterceptorSvcId = -1L;
    static bool clientInterceptorPreProxyCallRetval=true;
    static bool svcInterceptorPreExportCallRetval=true;

    static void setupFm(bool useCurlShare) {
        //server
        celix_properties_t *serverProps = celix_properties_load("server.properties");
        ASSERT_TRUE(serverProps != NULL);
        serverFramework = celix_frameworkFactory_createFramework(serverProps);
        ASSERT_TRUE(serverFramework != NULL);
        serverContext = celix_framework_getFrameworkContext(serverFramework);
        ASSERT_TRUE(serverContext != NULL);

        //client
        celix_properties_t *clientProperties = celix_properties_load("client.properties");
        celix_properties_setBool(clientProperties, "RSA_DFI_USE_CURL_SHARE_HANDLE", useCurlShare);
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

    static int rsaDfi_excepTestFunc1(void *handle CELIX_UNUSED) {
        return CELIX_CUSTOMER_ERROR_MAKE(0,1);
    }

    static void registerExceptionTestServer(void) {
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_INTERFACES, RSA_DIF_EXCEPTION_TEST_SERVICE);
        celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_CONFIGS, "org.amdatu.remote.admin.http");
        exceptionTestService = (rsa_dfi_exception_test_service_t *)calloc(1,sizeof(*exceptionTestService));
        exceptionTestService->handle = NULL;
        exceptionTestService->func1 = rsaDfi_excepTestFunc1;
        exceptionTestSvcId = celix_bundleContext_registerService(serverContext, exceptionTestService, RSA_DIF_EXCEPTION_TEST_SERVICE, properties);
    }

    static void unregisterExceptionTestServer(void) {
        celix_bundleContext_unregisterService(serverContext, exceptionTestSvcId);
        free(exceptionTestService);
    }

    static bool serverServiceInterceptor_preProxyCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {
        return true;
    }

    static void serverServiceInterceptor_postProxyCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {

    }

    static bool serverServiceInterceptor_preExportCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {
        return svcInterceptorPreExportCallRetval;
    }

    static void serverServiceInterceptor_postExportCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {

    }

    static bool clientServiceInterceptor_preProxyCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {
        return clientInterceptorPreProxyCallRetval;
    }

    static void clientServiceInterceptor_postProxyCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {

    }

    static bool clientServiceInterceptor_preExportCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {
        return true;
    }

    static void clientServiceInterceptor_postExportCall(void *, const celix_properties_t *, const char *, celix_properties_t *) {

    }

    static void registerInterceptorService(void) {
        svcInterceptorPreExportCallRetval = true;
        serverSvcInterceptor = (remote_interceptor_t *)calloc(1,sizeof(*serverSvcInterceptor));
        serverSvcInterceptor->handle = NULL;
        serverSvcInterceptor->preProxyCall = serverServiceInterceptor_preProxyCall;
        serverSvcInterceptor->postProxyCall = serverServiceInterceptor_postProxyCall;
        serverSvcInterceptor->preExportCall = serverServiceInterceptor_preExportCall;
        serverSvcInterceptor->postExportCall = serverServiceInterceptor_postExportCall;
        celix_properties_t *svcInterceptorProps = celix_properties_create();
        celix_properties_setLong(svcInterceptorProps, CELIX_FRAMEWORK_SERVICE_RANKING, 10);
        celix_service_registration_options_t svcInterceptorOpts{};
        svcInterceptorOpts.svc = serverSvcInterceptor;
        svcInterceptorOpts.serviceName = REMOTE_INTERCEPTOR_SERVICE_NAME;
        svcInterceptorOpts.serviceVersion = REMOTE_INTERCEPTOR_SERVICE_VERSION;
        svcInterceptorOpts.properties = svcInterceptorProps;
        serverSvcInterceptorSvcId = celix_bundleContext_registerServiceWithOptions(serverContext, &svcInterceptorOpts);

        clientInterceptorPreProxyCallRetval = true;
        clientSvcInterceptor = (remote_interceptor_t *)calloc(1,sizeof(*clientSvcInterceptor));
        clientSvcInterceptor->handle = NULL;
        clientSvcInterceptor->preProxyCall = clientServiceInterceptor_preProxyCall;
        clientSvcInterceptor->postProxyCall = clientServiceInterceptor_postProxyCall;
        clientSvcInterceptor->preExportCall = clientServiceInterceptor_preExportCall;
        clientSvcInterceptor->postExportCall = clientServiceInterceptor_postExportCall;
        celix_properties_t *clientInterceptorProps = celix_properties_create();
        celix_properties_setLong(clientInterceptorProps, CELIX_FRAMEWORK_SERVICE_RANKING, 10);
        celix_service_registration_options_t clientInterceptorOpts{};
        clientInterceptorOpts.svc = clientSvcInterceptor;
        clientInterceptorOpts.serviceName = REMOTE_INTERCEPTOR_SERVICE_NAME;
        clientInterceptorOpts.serviceVersion = REMOTE_INTERCEPTOR_SERVICE_VERSION;
        clientInterceptorOpts.properties = clientInterceptorProps;
        clientSvcInterceptorSvcId = celix_bundleContext_registerServiceWithOptions(clientContext, &clientInterceptorOpts);
    }

    static void unregisterInterceptorService(void) {
        celix_bundleContext_unregisterService(clientContext, clientSvcInterceptorSvcId);
        free(clientSvcInterceptor);

        celix_bundleContext_unregisterService(serverContext, serverSvcInterceptorSvcId);
        free(serverSvcInterceptor);
    }

    static void testComplex(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteComplex(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testAction(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteAction(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testNumbers(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteNumbers(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testString(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteString(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testEnum(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteEnum(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testConstString(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool discovered = tst->isRemoteExampleDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        bool ok = tst->testRemoteConstString(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testCalculator(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);

        bool ok;

        bool discovered = tst->isCalcDiscovered(tst->handle);
        ASSERT_TRUE(discovered);

        ok = tst->testCalculator(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testCreateDestroyComponentWithRemoteService(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);
        bool ok = tst->testCreateDestroyComponentWithRemoteService(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testAddRemoteServiceInRemoteService(void *handle CELIX_UNUSED, void *svc) {
        auto *tst = static_cast<tst_service_t *>(svc);
        bool ok = tst->testCreateRemoteServiceInRemoteCall(tst->handle);
        ASSERT_TRUE(ok);
    };

    static void testInterceptorPreExportCallReturnFalse(void *handle CELIX_UNUSED, void *svc) {
        svcInterceptorPreExportCallRetval = false;
        auto *tst = static_cast<tst_service_t *>(svc);

        bool ok = tst->testRemoteAction(tst->handle);
        ASSERT_FALSE(ok);
    }

    static void testInterceptorPreProxyCallReturnFalse(void *handle CELIX_UNUSED, void *svc) {
        clientInterceptorPreProxyCallRetval = false;
        auto *tst = static_cast<tst_service_t *>(svc);

        bool ok = tst->testRemoteAction(tst->handle);
        ASSERT_FALSE(ok);
    }

    static void testExceptionServiceCallback(void *handle CELIX_UNUSED, void *svc) {
        rsa_dfi_exception_test_service_t * service = (rsa_dfi_exception_test_service_t *)(svc);
        int ret = service->func1(service->handle);
        EXPECT_EQ(CELIX_CUSTOMER_ERROR_MAKE(0,1),ret);
    }

    static void testExceptionService(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = RSA_DIF_EXCEPTION_TEST_SERVICE;
        opts.use = testExceptionServiceCallback;
        opts.waitTimeoutInSeconds = 2;
        bool called = celix_bundleContext_useServiceWithOptions(clientContext, &opts);
        ASSERT_TRUE(called);
    }
}

template<typename F>
static void test(F&& f) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = TST_SERVICE_NAME;
    opts.use = f;
    opts.waitTimeoutInSeconds = 2;
    bool called = celix_bundleContext_useServiceWithOptions(clientContext, &opts);
    ASSERT_TRUE(called);
}

class RsaDfiClientServerTests : public ::testing::Test {
public:
    RsaDfiClientServerTests() {
        setupFm(false);
    }
    ~RsaDfiClientServerTests() override {
        teardownFm();
    }

};

class RsaDfiClientServerWithCurlShareTests : public ::testing::Test {
public:
    RsaDfiClientServerWithCurlShareTests() {
        setupFm(true);
    }
    ~RsaDfiClientServerWithCurlShareTests() override {
        teardownFm();
    }

};

class RsaDfiClientServerInterceptorTests : public ::testing::Test {
public:
    RsaDfiClientServerInterceptorTests() {
        setupFm(false);
        registerInterceptorService();
    }
    ~RsaDfiClientServerInterceptorTests() override {
        unregisterInterceptorService();
        teardownFm();
    }
};

class RsaDfiClientServerExceptionTests : public ::testing::Test {
public:
    RsaDfiClientServerExceptionTests() {
        setupFm(false);
        registerExceptionTestServer();
    }
    ~RsaDfiClientServerExceptionTests() override {
        unregisterExceptionTestServer();
        teardownFm();
    }
};

TEST_F(RsaDfiClientServerTests, TestRemoteCalculator) {
    test(testCalculator);
}

TEST_F(RsaDfiClientServerWithCurlShareTests, TestRemoteCalculator) {
    test(testCalculator);
}

TEST_F(RsaDfiClientServerTests, TestRemoteComplex) {
    test(testComplex);
}

TEST_F(RsaDfiClientServerWithCurlShareTests, TestRemoteComplex) {
    test(testComplex);
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

TEST_F(RsaDfiClientServerTests, CreateDestroyComponentWithRemoteService) {
    test(testCreateDestroyComponentWithRemoteService);
}

TEST_F(RsaDfiClientServerTests, AddRemoteServiceInRemoteService) {
    test(testAddRemoteServiceInRemoteService);
}

TEST_F(RsaDfiClientServerInterceptorTests,TestInterceptorPreExportCallReturnFalse) {
    test(testInterceptorPreExportCallReturnFalse);
}

TEST_F(RsaDfiClientServerInterceptorTests,TestInterceptorPreProxyCallReturnFalse) {
    test(testInterceptorPreProxyCallReturnFalse);
}

TEST_F(RsaDfiClientServerExceptionTests,TestExceptionService) {
    testExceptionService();
}
