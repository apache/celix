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
#include "rsa_shm_import_registration.h"
#include "rsa_shm_constants.h"
#include "RsaShmTestService.h"
#include "remote_constants.h"
#include "rsa_rpc_factory.h"
#include "endpoint_description.h"
#include "celix_log_helper.h"
#include "celix_bundle_context_ei.h"
#include "malloc_ei.h"
#include "celix_framework.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include <gtest/gtest.h>

static celix_status_t expect_RpcFacCreateProxy_ret = CELIX_SUCCESS;
static celix_status_t RpcFacCreateProxy(void *handle, const endpoint_description_t *endpointDesc, long requestSenderSvcId, long *proxySvcId) {
    (void)endpointDesc;//unused
    (void)requestSenderSvcId;//unused
    if (expect_RpcFacCreateProxy_ret != CELIX_SUCCESS) {
        return expect_RpcFacCreateProxy_ret;
    }
    celix_bundle_context_t *ctx = (celix_bundle_context_t *) handle;
    static rsa_shm_calc_service_t calcService{};
    calcService.handle = nullptr,
            calcService.add = [](void *handle, double a, double b, double *result) -> celix_status_t {
                (void) handle; //unused
                *result = a + b;
                return CELIX_SUCCESS;
            };
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_INTERFACES, RSA_SHM_CALCULATOR_SERVICE);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
    celix_properties_set(properties, OSGI_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE"," RSA_RPC_TYPE_PREFIX"mock");
    auto calcSvcId = celix_bundleContext_registerServiceAsync(ctx, &calcService, RSA_SHM_CALCULATOR_SERVICE, properties);
    EXPECT_GE(calcSvcId, 0);
    *proxySvcId = calcSvcId;
    return CELIX_SUCCESS;
}

static void RpcFacDestroyProxy(void *handle, long proxySvcId) {
    celix_bundle_context_t *ctx = (celix_bundle_context_t *) handle;
    celix_bundleContext_unregisterServiceAsync(ctx, proxySvcId, nullptr, nullptr);
    return;
}

class RsaShmImportRegUnitTestSuite : public ::testing::Test {
public:
    RsaShmImportRegUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_import_reg_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"RsaShm");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};

        static rsa_rpc_factory_t rpcFactory{};
        rpcFactory.handle = ctx.get();
        rpcFactory.createProxy = RpcFacCreateProxy;
        rpcFactory.destroyProxy = RpcFacDestroyProxy;
        rpcFactory.createEndpoint = nullptr;
        rpcFactory.destroyEndpoint = nullptr;

        celix_properties_t *rpcFacProps = celix_properties_create();
        celix_properties_set(rpcFacProps, RSA_RPC_TYPE_KEY, RSA_RPC_TYPE_PREFIX"mock");
        celix_properties_set(rpcFacProps, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_RPC_FACTORY_VERSION);
        rpcFactorySvcId = celix_bundleContext_registerServiceAsync(ctx.get(), &rpcFactory, RSA_RPC_FACTORY_NAME, rpcFacProps);
        EXPECT_GE(rpcFactorySvcId, 1);

        celix_bundleContext_waitForEvents(ctx.get());
    }

    ~RsaShmImportRegUnitTestSuite() override {
        if (rpcFactorySvcId >= 0) {
            celix_bundleContext_unregisterServiceAsync(ctx.get(), rpcFactorySvcId, nullptr, nullptr);
        }

        //reset error injection
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
    }

    endpoint_description_t *CreateEndpointDescription() {
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, RSA_SHM_CALCULATOR_SERVICE);
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE"," RSA_RPC_TYPE_PREFIX"mock");
        celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "7f7efba5-500f-4ee9-b733-68de012091da");
        celix_properties_setLong(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, 100);//Set a dummy service id
        celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
        celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, celix_bundleContext_getProperty(ctx.get(), CELIX_FRAMEWORK_UUID, ""));
        celix_properties_set(properties, RSA_SHM_SERVER_NAME_KEY, "ShmServ-dummy");
        endpoint_description_t *endpoint = nullptr;
        auto status = endpointDescription_create(properties, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, endpoint);
        return endpoint;
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    long rpcFactorySvcId{-1};
};

TEST_F(RsaShmImportRegUnitTestSuite, CreateImportRegistration) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, importRegistration);

    importRegistration_destroy(importRegistration);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, CreateImportRegistrationWithInvalidParams) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    auto status = importRegistration_create(nullptr, logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = importRegistration_create(ctx.get(), nullptr, endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = importRegistration_create(ctx.get(), logHelper.get(), nullptr, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, -1, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, CreateImportRegistrationWithNoMemory) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    celix_ei_expect_calloc((void*)&importRegistration_create, 0, nullptr);
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, FailedToCloneEndpointDescription) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    celix_ei_expect_calloc((void*)&endpointDescription_clone, 0, nullptr);
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, CreateImportRegistrationWithInvalidRpcType) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    celix_properties_set(endpoint->properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "invalid");
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, CreateImportRegistrationWithoutRpcType) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    celix_properties_unset(endpoint->properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS);
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, CreateImportRegistrationWithOverlongRpcTypeString) {
    endpoint_description_t *endpoint = CreateEndpointDescription();

    char overlongRpcType[128] = RSA_RPC_TYPE_PREFIX;
    for (int i = strlen(RSA_RPC_TYPE_PREFIX); i < 127; ++i) {
        overlongRpcType[i] = 'a';
    }
    celix_properties_set(endpoint->properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, overlongRpcType);

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, FailedToTrackRpcFactory) {
    auto* endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&importRegistration_create, 0, -1);
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);

    importRegistration_destroy(importRegistration);

    endpointDescription_destroy(endpoint);
}


TEST_F(RsaShmImportRegUnitTestSuite, RegisterMoreThanOneRpcFactory) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(ctx.get());

    //register another rpc factory
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, RSA_RPC_TYPE_KEY, RSA_RPC_TYPE_PREFIX"mock");
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_RPC_FACTORY_VERSION);
    auto svcId = celix_bundleContext_registerServiceAsync(ctx.get(), (void*)"dumb-rpc-service", RSA_RPC_FACTORY_NAME, props);
    EXPECT_GE(svcId, 1);
    celix_bundleContext_waitForEvents(ctx.get());
    celix_bundleContext_unregisterServiceAsync(ctx.get(), svcId, nullptr, nullptr);

    //destroy import registration
    importRegistration_destroy(importRegistration);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, FailedToCreateServiceProxy) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();

    expect_RpcFacCreateProxy_ret = CELIX_ENOMEM;

    import_registration_t *importRegistration = nullptr;
    long reqSenderSvcId = 123;//set dummy service id
    auto status = importRegistration_create(ctx.get(), logHelper.get(), endpoint, reqSenderSvcId, &importRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(ctx.get());

    expect_RpcFacCreateProxy_ret = CELIX_SUCCESS;//reset error injection

    //destroy import registration
    importRegistration_destroy(importRegistration);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmImportRegUnitTestSuite, GetImportedEndpointWithInvalidParams) {
    import_registration_t *importRegistration = (import_registration_t *)0x1234;//set dummy import registration
    endpoint_description_t *endpoint = nullptr;
    auto status = importRegistration_getImportedEndpoint(nullptr, &endpoint);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = importRegistration_getImportedEndpoint(importRegistration, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}