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
#include "rsa_shm_export_registration.h"
#include "rsa_shm_constants.h"
#include "RsaShmTestService.h"
#include "remote_constants.h"
#include "rsa_rpc_factory.h"
#include "endpoint_description.h"
#include "rsa_request_handler_service.h"
#include "celix_log_helper.h"
#include "celix_bundle_context_ei.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_framework.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include <errno.h>
#include <gtest/gtest.h>

#define RSA_RPC_TYPE_FOR_TEST "celix.remote.admin.rpc_type.test"

static celix_status_t expect_RpcFacCreateEndpoint_ret = CELIX_SUCCESS;
static celix_status_t RpcFacCreateEndpoint(void *handle, const endpoint_description_t *endpoint, long *requestHandlerSvcId) {
    (void)endpoint; //unused
    if (expect_RpcFacCreateEndpoint_ret != CELIX_SUCCESS) {
        return expect_RpcFacCreateEndpoint_ret;
    }
    celix_bundle_context_t *ctx = static_cast<celix_bundle_context_t *>(handle); //unused
    static rsa_request_handler_service_t requestHandler{};
    requestHandler.handle = ctx;
    requestHandler.handleRequest = [](void *handle, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
        (void) handle; //unused
        (void) request; //unused
        (void) metadata; //unused
        (void) response; //unused
        return CELIX_SUCCESS;
    };
    celix_service_registration_options_t opts{};
    opts.svc = &requestHandler;
    opts.serviceName = CELIX_RSA_REQUEST_HANDLER_SERVICE_NAME;
    opts.serviceVersion = CELIX_RSA_REQUEST_HANDLER_SERVICE_VERSION;
    auto svcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    EXPECT_GE(svcId, 0);
    *requestHandlerSvcId = svcId;
    return CELIX_SUCCESS;
}

static void RpcFacDestroyEndpoint(void *handle, long requestHandlerSvcId) {
    celix_bundle_context_t *ctx = static_cast<celix_bundle_context_t *>(handle) ;
    celix_bundleContext_unregisterServiceAsync(ctx, requestHandlerSvcId, nullptr, nullptr);
    return;
}

class RsaShmExportRegUnitTestSuite : public ::testing::Test {
public:
    RsaShmExportRegUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_export_reg_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"RsaShm");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};

        static rsa_shm_calc_service_t calcService{};
        calcService.handle = nullptr,
        calcService.add = [](void *handle, double a, double b, double *result) -> celix_status_t {
                    (void) handle; //unused
                    *result = a + b;
                    return CELIX_SUCCESS;
                };
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, RSA_SHM_CALCULATOR_SERVICE);
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE);
        celix_properties_set(properties, RSA_SHM_RPC_TYPE_KEY, RSA_RPC_TYPE_FOR_TEST);
        calcSvcId = celix_bundleContext_registerServiceAsync(ctx.get(), &calcService, RSA_SHM_CALCULATOR_SERVICE, properties);
        EXPECT_GE(calcSvcId, 0);

        static rsa_rpc_factory_t rpcFactory{};
        rpcFactory.handle = ctx.get();
        rpcFactory.createProxy = nullptr;
        rpcFactory.destroyProxy = nullptr;
        rpcFactory.createEndpoint = RpcFacCreateEndpoint;
        rpcFactory.destroyEndpoint = RpcFacDestroyEndpoint;

        celix_properties_t *rpcFacProps = celix_properties_create();
        celix_properties_set(rpcFacProps, CELIX_RSA_RPC_TYPE_KEY, RSA_RPC_TYPE_FOR_TEST);
        celix_properties_set(rpcFacProps, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_RSA_RPC_FACTORY_VERSION);
        rpcFactorySvcId = celix_bundleContext_registerServiceAsync(ctx.get(), &rpcFactory, CELIX_RSA_RPC_FACTORY_NAME, rpcFacProps);
        EXPECT_GE(rpcFactorySvcId, 1);

        celix_bundleContext_waitForEvents(ctx.get());
    }

    ~RsaShmExportRegUnitTestSuite() override {
        if (rpcFactorySvcId >= 0) {
            celix_bundleContext_unregisterServiceAsync(ctx.get(), rpcFactorySvcId, nullptr, nullptr);
        }
        celix_bundleContext_unregisterServiceAsync(ctx.get(), calcSvcId, nullptr, nullptr);
        celix_bundleContext_waitForEvents(ctx.get());

        celix_ei_expect_bundleContext_retainServiceReference(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
    }

    endpoint_description_t *CreateEndpointDescription() {
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, RSA_SHM_CALCULATOR_SERVICE);
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE);
        celix_properties_set(properties, RSA_SHM_RPC_TYPE_KEY, RSA_RPC_TYPE_FOR_TEST);
        celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "7f7efba5-500f-4ee9-b733-68de012091da");
        celix_properties_setLong(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, calcSvcId);
        celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
        celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, celix_bundleContext_getProperty(ctx.get(), CELIX_FRAMEWORK_UUID, ""));
        celix_properties_set(properties, RSA_SHM_SERVER_NAME_KEY, "ShmServ-dummy");
        endpoint_description_t *endpoint = nullptr;
        auto status = endpointDescription_create(properties, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, endpoint);
        return endpoint;
    }

    service_reference_pt GetServiceReference() {
        celix_array_list_t *references = nullptr;
        char filter[32] = {0};
        snprintf(filter, sizeof(filter), "(%s=%ld)", (char *) CELIX_FRAMEWORK_SERVICE_ID, calcSvcId);
        auto status = bundleContext_getServiceReferences(ctx.get(), nullptr, filter, &references);
        EXPECT_EQ(CELIX_SUCCESS, status);
        service_reference_pt reference = (service_reference_pt) celix_arrayList_get(references, 0);
        EXPECT_NE(nullptr, reference);
        celix_arrayList_destroy(references);
        return reference;
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    long calcSvcId{-1};
    long rpcFactorySvcId{-1};
};

TEST_F(RsaShmExportRegUnitTestSuite, CreateExportRegistration) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, exportRegistration);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);

    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, CreateExportRegistrationWithInvalidEndpoint) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    export_registration_t *exportRegistration = nullptr;

    auto status = exportRegistration_create(nullptr, logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = exportRegistration_create(ctx.get(), nullptr, reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = exportRegistration_create(ctx.get(), logHelper.get(), nullptr, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = exportRegistration_create(ctx.get(), logHelper.get(), reference, nullptr, &exportRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, CreateExportRegistrationWithOutRpcType) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_properties_unset(endpoint->properties, RSA_SHM_RPC_TYPE_KEY);

    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, CreateExportRegistrationWithGettingServiceReferenceFails) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_ei_expect_bundleContext_retainServiceReference((void*)&exportRegistration_create, 0, CELIX_ILLEGAL_ARGUMENT);

    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, CreateExportRegistrationWithNoMemory) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_ei_expect_calloc((void*)&exportRegistration_create, 0, nullptr);
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ENOMEM, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, FailedToCloneEndpointDescription) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_ei_expect_calloc((void*)&endpointDescription_clone, 0, nullptr);
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ENOMEM, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, CreateExportRegistrationWithTrackingRpcFactoryFails) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&exportRegistration_create, 0, -1);
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, FailedToCreateReqHandlerSvcEntry) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_ei_expect_calloc((void*)&exportRegistration_create, 1, nullptr, 2);
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ENOMEM, status);

    celix_ei_expect_celixThreadRwlock_create((void*)&exportRegistration_create, 1, ENOMEM);
    status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_ENOMEM, status);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaShmExportRegUnitTestSuite, RegisterMoreThanOneRpcFactory) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);
    celix_bundleContext_waitForEvents(ctx.get());

    //register another rpc factory
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, CELIX_RSA_RPC_TYPE_KEY, RSA_RPC_TYPE_FOR_TEST);
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, CELIX_RSA_RPC_FACTORY_VERSION);
    auto svcId = celix_bundleContext_registerServiceAsync(ctx.get(), (void*)"dumb-rpc-service", CELIX_RSA_RPC_FACTORY_NAME, props);
    EXPECT_GE(svcId, 1);
    celix_bundleContext_waitForEvents(ctx.get());
    celix_bundleContext_unregisterServiceAsync(ctx.get(), svcId, nullptr, nullptr);

    //Release export registration
    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, RpcFactoryFailedToCreateEndpoint) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    expect_RpcFacCreateEndpoint_ret = CELIX_SERVICE_EXCEPTION;
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());

    expect_RpcFacCreateEndpoint_ret = CELIX_SUCCESS;//reset error injection

    //Release export registration
    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, RpcFactoryFailedToTrackRequestHandlerService) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(CELIX_EI_UNKNOWN_CALLER, 0, -1, 2);
    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());

    //Release export registration
    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, ExportRegistrationCall) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());

    struct iovec request = {.iov_base = (void*)"request", .iov_len = 7};
    struct iovec response{};
    status = exportRegistration_call(exportRegistration, nullptr, &request, &response);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //Release export registration
    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, ExportRegistrationCallWithInvalidParams) {
    struct iovec request = {.iov_base = (void*)"request", .iov_len = 7};
    struct iovec response{};
    auto status = exportRegistration_call(nullptr, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaShmExportRegUnitTestSuite, ExportRegistrationCallWithNoRequestHandler) {
    //Create export registration
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    //remove request handler service
    celix_bundleContext_unregisterServiceAsync(ctx.get(), rpcFactorySvcId, nullptr, nullptr);
    rpcFactorySvcId = -1;

    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());

    struct iovec request = {.iov_base = (void*)"request", .iov_len = 7};
    struct iovec response{};
    status = exportRegistration_call(exportRegistration, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    //Release export registration
    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, GetExportReference) {
    endpoint_description_t *endpoint = CreateEndpointDescription();
    service_reference_pt reference = GetServiceReference();

    export_registration_t *exportRegistration = nullptr;
    auto status = exportRegistration_create(ctx.get(), logHelper.get(), reference, endpoint, &exportRegistration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, exportRegistration);

    export_reference_t *exportReference = nullptr;
    status = exportRegistration_getExportReference(exportRegistration, &exportReference);
    EXPECT_EQ(CELIX_SUCCESS, status);

    service_reference_pt ref = nullptr;
    status = exportReference_getExportedService(exportReference, &ref);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_EQ(reference, ref);

    free(exportReference);

    bundleContext_ungetServiceReference(ctx.get(), reference);
    endpointDescription_destroy(endpoint);

    exportRegistration_release(exportRegistration);
}

TEST_F(RsaShmExportRegUnitTestSuite, GetExportReferenceWithInvalidParams) {
    export_registration_t *registration = (export_registration_t *)0x1234;
    export_reference_t *exportReference = nullptr;
    auto status = exportRegistration_getExportReference(registration, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    status = exportRegistration_getExportReference(nullptr, &exportReference);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaShmExportRegUnitTestSuite, GetExportedEndpointWithInvalidParams) {
    export_reference_t *exportReference = (export_reference_t *)0x1234;
    endpoint_description_t *endpoint = nullptr;
    auto status = exportReference_getExportedEndpoint(exportReference, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    status = exportReference_getExportedEndpoint(nullptr, &endpoint);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaShmExportRegUnitTestSuite, GetExportedServiceWithInvalidParams) {
    export_reference_t *exportReference = (export_reference_t *)0x1234;
    service_reference_pt reference = nullptr;
    auto status = exportReference_getExportedService(exportReference, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    status = exportReference_getExportedService(nullptr, &reference);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}