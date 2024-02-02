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
#include "calculator_service.h"

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
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, RSA_DIF_EXCEPTION_TEST_SERVICE);
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, "org.amdatu.remote.admin.http");
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
        svcInterceptorOpts.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
        svcInterceptorOpts.serviceVersion = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_VERSION;
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
        clientInterceptorOpts.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
        clientInterceptorOpts.serviceVersion = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_VERSION;
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


class RsaDfiDynamicIpServerTestSuite : public ::testing::Test {
public:
    RsaDfiDynamicIpServerTestSuite() {
        {
            auto* props = celix_properties_create();
            celix_properties_setBool(props, "CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT", true);
            celix_properties_setBool(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
            celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_dfi_server_cache");
            serverFw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(props), [](auto f) {celix_frameworkFactory_destroyFramework(f);}};
            serverCtx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(serverFw.get()), [](auto*){/*nop*/}};

            auto bundleId = celix_bundleContext_installBundle(serverCtx.get(), RSA_DFI_BUNDLE_FILE, true);
            EXPECT_TRUE(bundleId >= 0);

            bundleId = celix_bundleContext_installBundle(serverCtx.get(), CALC_BUNDLE_FILE, true);
            EXPECT_TRUE(bundleId >= 0);

            celix_bundleContext_waitForEvents(serverCtx.get());
        }

        {
            auto* props = celix_properties_create();
            celix_properties_setBool(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
            celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_dfi_client_cache");
            clientFw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(props), [](auto f) {celix_frameworkFactory_destroyFramework(f);}};
            clientCtx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(clientFw.get()), [](auto*){/*nop*/}};

            auto bundleId = celix_bundleContext_installBundle(clientCtx.get(), RSA_DFI_BUNDLE_FILE, true);
            EXPECT_TRUE(bundleId >= 0);

            bundleId = celix_bundleContext_installBundle(clientCtx.get(), TST_BUNDLE_FILE, true);
            EXPECT_TRUE(bundleId >= 0);

            celix_bundleContext_waitForEvents(clientCtx.get());
        }
    }
    ~RsaDfiDynamicIpServerTestSuite() override = default;

    void TestRemoteCalculator(void (*testBody)(tst_service_t* testSvc), const char* serverIp = "127.0.0.1") {
        remote_service_admin_service_t* serverRsaSvc{nullptr};
        remote_service_admin_service_t* clientRsaSvc{nullptr};
        serverRsaTrkId = celix_bundleContext_trackService(serverCtx.get(), CELIX_RSA_REMOTE_SERVICE_ADMIN,
                                                          &serverRsaSvc, [](void* handle, void* svc){
                    auto rsaSvc = static_cast<remote_service_admin_service_t **>(handle);
                    *rsaSvc = static_cast<remote_service_admin_service_t*>(svc);
                });
        EXPECT_GE(serverRsaTrkId, 0);
        clientRsaTrkId = celix_bundleContext_trackService(clientCtx.get(), CELIX_RSA_REMOTE_SERVICE_ADMIN,
                                                          &clientRsaSvc, [](void* handle, void* svc){
                    auto rsaSvc = static_cast<remote_service_admin_service_t **>(handle);
                    *rsaSvc = static_cast<remote_service_admin_service_t*>(svc);
                });
        EXPECT_GE(clientRsaTrkId, 0);
        long calcId = celix_bundleContext_findService(serverCtx.get(), CALCULATOR_SERVICE);
        ASSERT_TRUE(calcId >= 0L);
        ASSERT_TRUE(clientRsaSvc != nullptr);
        ASSERT_TRUE(serverRsaSvc != nullptr);

        char calcIdStr[32] = {0};
        snprintf(calcIdStr, 32, "%li", calcId);
        celix_array_list_t *svcRegistrations = NULL;
        auto status = serverRsaSvc->exportService(serverRsaSvc->admin, calcIdStr, NULL, &svcRegistrations);
        ASSERT_EQ(CELIX_SUCCESS, status);
        ASSERT_EQ(1, celix_arrayList_size(svcRegistrations));
        export_registration_t *exportedReg = static_cast<export_registration_t*>(celix_arrayList_get(svcRegistrations, 0));
        export_reference_t *exportedRef = nullptr;
        status = serverRsaSvc->exportRegistration_getExportReference(exportedReg, &exportedRef);
        ASSERT_EQ(CELIX_SUCCESS, status);
        endpoint_description_t *exportedEndpoint = nullptr;
        status = serverRsaSvc->exportReference_getExportedEndpoint(exportedRef, &exportedEndpoint);
        ASSERT_EQ(CELIX_SUCCESS, status);
        free(exportedRef);
        celix_arrayList_destroy(svcRegistrations);

        celix_properties_t *importProps = celix_properties_copy(exportedEndpoint->properties);
        ASSERT_TRUE(importProps != nullptr);
        celix_properties_set(importProps, CELIX_RSA_IP_ADDRESSES, serverIp);
        endpoint_description_t *importedEndpoint = nullptr;
        status = endpointDescription_create(importProps, &importedEndpoint);
        ASSERT_EQ(CELIX_SUCCESS, status);

        import_registration_t* reg = nullptr;
        status = clientRsaSvc->importService(clientRsaSvc->admin, importedEndpoint, &reg);
        ASSERT_EQ(CELIX_SUCCESS, status);
        celix_bundleContext_waitForEvents(clientCtx.get());

        celix_service_use_options_t opts{};
        opts.filter.serviceName = TST_SERVICE_NAME;
        opts.callbackHandle = (void*)testBody;
        opts.use = [](void *handle , void *svc) {
            auto* testBody = (void (*)(tst_service_t*))handle;
            testBody(static_cast<tst_service_t*>(svc));
        };
        opts.waitTimeoutInSeconds = 5;
        auto called = celix_bundleContext_useServiceWithOptions(clientCtx.get(), &opts);
        ASSERT_TRUE(called);

        status = clientRsaSvc->importRegistration_close(clientRsaSvc->admin, reg);
        ASSERT_EQ(CELIX_SUCCESS, status);
        endpointDescription_destroy(importedEndpoint);

        status = serverRsaSvc->exportRegistration_close(serverRsaSvc->admin, exportedReg);
        ASSERT_EQ(CELIX_SUCCESS, status);

        celix_bundleContext_stopTracker(clientCtx.get(), clientRsaTrkId);
        celix_bundleContext_stopTracker(serverCtx.get(), serverRsaTrkId);
    }

    std::shared_ptr<celix_framework_t> serverFw{};
    std::shared_ptr<celix_bundle_context_t> serverCtx{};
    long serverRsaTrkId{-1};
    std::shared_ptr<celix_framework_t> clientFw{};
    std::shared_ptr<celix_bundle_context_t> clientCtx{};
    long clientRsaTrkId{-1};
};

TEST_F(RsaDfiDynamicIpServerTestSuite, RemoteCalculatorTest) {
    TestRemoteCalculator([](tst_service_t* testSvc) {
        auto ok = testSvc->testCalculator(testSvc->handle);
        ASSERT_TRUE(ok);
    });
}

TEST_F(RsaDfiDynamicIpServerTestSuite, CallingMultiTimesRemoteCalculatorTest) {
    TestRemoteCalculator([](tst_service_t* testSvc) {
        auto ok = testSvc->testCalculator(testSvc->handle);
        ASSERT_TRUE(ok);

        ok = testSvc->testCalculator(testSvc->handle);
        ASSERT_TRUE(ok);
    });
}

TEST_F(RsaDfiDynamicIpServerTestSuite, IPV6RemoteCalculatorTest) {
    TestRemoteCalculator([](tst_service_t* testSvc) {
        auto ok = testSvc->testCalculator(testSvc->handle);
        ASSERT_TRUE(ok);
    }, "::1");
}
