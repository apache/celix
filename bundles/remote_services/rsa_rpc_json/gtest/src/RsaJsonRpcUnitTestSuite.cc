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
#include "rsa_json_rpc_impl.h"
#include "rsa_json_rpc_constants.h"
#include "rsa_request_sender_tracker.h"
#include "rsa_json_rpc_proxy_impl.h"
#include "rsa_json_rpc_endpoint_impl.h"
#include "rsa_request_sender_service.h"
#include "rsa_request_handler_service.h"
#include "RsaJsonRpcTestService.h"
#include "remote_interceptor.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "celix_log_helper.h"
#include "celix_properties.h"
#include "celix_types.h"
#include "celix_framework.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_utils.h"
#include "celix_bundle_ei.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_version_ei.h"
#include "dfi_ei.h"
#include "celix_properties_ei.h"
#include "celix_long_hash_map_ei.h"
#include <gtest/gtest.h>
#include <cstdlib>
extern "C" {
#include "remote_interceptors_handler.h"
}

class RsaJsonRpcUnitTestSuite : public ::testing::Test {
public:
    RsaJsonRpcUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_json_rpc_impl_cache");
        celix_properties_set(props, CELIX_FRAMEWORK_BUNDLE_VERSION, "1.0.0");
        celix_properties_set(props, RSA_JSON_RPC_LOG_CALLS_KEY, "true");
        celix_properties_set(props, "CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR);
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"RsaJsonRpc");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};
    }

    ~RsaJsonRpcUnitTestSuite() override {
        celix_ei_expect_celix_bundle_getSymbolicName(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundle_getManifestValue(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_registerServiceFactoryAsync(nullptr, 0, 0);
        celix_ei_expect_celix_version_createVersionFromString(nullptr, 0, nullptr);
        celix_ei_expect_dynFunction_createClosure(nullptr, 0, 0);
        celix_ei_expect_jsonRpc_prepareInvokeRequest(nullptr, 0, 0);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
    }

    endpoint_description_t *CreateEndpointDescription(long svcId = 100/*set a dummy service id*/) {
        endpoint_description_t *endpointDesc = (endpoint_description_t *)calloc(1, sizeof(endpoint_description_t));
        EXPECT_NE(endpointDesc, nullptr);
        endpointDesc->properties = celix_properties_create();
        EXPECT_TRUE(endpointDesc->properties != nullptr);
        const char *uuid = celix_bundleContext_getProperty(ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
        celix_properties_set(endpointDesc->properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
        celix_properties_set(endpointDesc->properties, CELIX_FRAMEWORK_SERVICE_NAME, RSA_RPC_JSON_TEST_SERVICE);
        celix_properties_set(endpointDesc->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "1.0.0");
        celix_properties_set(endpointDesc->properties, OSGI_RSA_ENDPOINT_ID, "8cf05b2d-421e-4c46-b55e-c3f1900b7cba");
        celix_properties_set(endpointDesc->properties, OSGI_RSA_SERVICE_IMPORTED, "true");
        endpointDesc->frameworkUUID = (char*)celix_properties_get(endpointDesc->properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, nullptr);
        endpointDesc->serviceId = svcId;
        endpointDesc->id = (char*)celix_properties_get(endpointDesc->properties, OSGI_RSA_ENDPOINT_ID, nullptr);
        endpointDesc->serviceName = strdup(RSA_RPC_JSON_TEST_SERVICE);
        return endpointDesc;
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
};

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpc) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, jsonRpc);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");

    auto status  = rsaJsonRpc_create(nullptr, logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status  = rsaJsonRpc_create(ctx.get(), nullptr, &jsonRpc);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithENOMEM) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");

    celix_ei_expect_calloc((void*)&rsaJsonRpc_create, 0, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithInvalidVersion) {
    rsa_json_rpc_t *jsonRpc = nullptr;

    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);

    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "abc");
    status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithInvalidBundleSymbolicName) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");

    celix_ei_expect_celix_bundle_getSymbolicName((void*)&rsaJsonRpc_create, 1, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, FailedToCreateThreadMutex) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");

    celix_ei_expect_celixThreadMutex_create((void*)&rsaJsonRpc_create, 0, CELIX_ENOMEM);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, FailedToCreateRemoteInterceptorsHandler) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");

    celix_ei_expect_calloc((void*)&remoteInterceptorsHandler_create, 0, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, FailedToCreateRsaRequestSenderTracker) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");

    celix_ei_expect_calloc((void*)&rsaRequestSenderTracker_create, 0, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRpcProxy) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long reqSenderSvcId = 101;//set a dummy service id
    long proxySvcId = -1;
    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, reqSenderSvcId, &proxySvcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(-1, proxySvcId);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroyProxy(jsonRpc, proxySvcId);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRpcProxyWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long reqSenderSvcId = 101;//set a dummy service id
    long proxySvcId = -1;
    status = rsaJsonRpc_createProxy(jsonRpc, nullptr, reqSenderSvcId, &proxySvcId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, -1, &proxySvcId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, reqSenderSvcId, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, RpcProxyFailedToCreateProxyFactory) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long reqSenderSvcId = 101;//set a dummy service id
    long proxySvcId = -1;
    celix_ei_expect_calloc((void*)&rsaJsonRpcProxy_factoryCreate, 0, nullptr);
    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, reqSenderSvcId, &proxySvcId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, DestroyRpcProxyWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)0x1234;//set a dummy pointer
    long proxySvcId = 101;//set a dummy service id
    rsaJsonRpc_destroyProxy(nullptr, proxySvcId);
    rsaJsonRpc_destroyProxy(jsonRpc, -1);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateEndpoint) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long requestHandlerSvcId = -1;
    status = rsaJsonRpc_createEndpoint(jsonRpc, endpoint, &requestHandlerSvcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(-1, requestHandlerSvcId);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroyEndpoint(jsonRpc, requestHandlerSvcId);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRpcEndpointWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long requestHandlerSvcId = -1;
    status = rsaJsonRpc_createEndpoint(jsonRpc, nullptr, &requestHandlerSvcId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaJsonRpc_createEndpoint(jsonRpc, endpoint, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaJsonRpc_createEndpoint(nullptr, endpoint, &requestHandlerSvcId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, RpcEndpointFailedToCreateEndpoint) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long requestHandlerSvcId = -1;
    celix_ei_expect_calloc((void*)&rsaJsonRpcEndpoint_create, 0, nullptr);
    status = rsaJsonRpc_createEndpoint(jsonRpc, endpoint, &requestHandlerSvcId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, DestroyRpcEndpointWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = (rsa_json_rpc_t *)0x1234;//set a dummy pointer
    long requestHandlerSvcId = 101;//set a dummy service id
    rsaJsonRpc_destroyEndpoint(nullptr, requestHandlerSvcId);
    rsaJsonRpc_destroyEndpoint(jsonRpc, -1);
}

static rsa_request_sender_service_t reqSenderSvc{};

class RsaJsonRpcProxyUnitTestSuite : public RsaJsonRpcUnitTestSuite {
public:
    RsaJsonRpcProxyUnitTestSuite() {
        rsa_json_rpc_t *jsonRpcPtr = nullptr;
        celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
        auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpcPtr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, jsonRpcPtr);
        celix_ei_expect_celix_bundle_getManifestValue(nullptr, 0, nullptr);//reset for next test
        jsonRpc = std::shared_ptr<rsa_json_rpc_t>{jsonRpcPtr, [](auto* r){rsaJsonRpc_destroy(r);}};

        reqSenderSvc.handle = nullptr;
        reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
            (void)handle;//unused
            (void)endpointDesc;//unused
            (void)metadata;//unused
            (void)request;//unused
            response->iov_base = strdup("{}");
            response->iov_len = 2;
            return CELIX_SUCCESS;
        };
        celix_service_registration_options_t opts{};
        opts.serviceName = RSA_REQUEST_SENDER_SERVICE_NAME;
        opts.serviceVersion = RSA_REQUEST_SENDER_SERVICE_VERSION;
        opts.svc = &reqSenderSvc;
        reqSenderSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx.get(), &opts);
        EXPECT_NE(-1, reqSenderSvcId);

    }

    ~RsaJsonRpcProxyUnitTestSuite() override {
        celix_bundleContext_unregisterServiceAsync(ctx.get(), reqSenderSvcId, nullptr, nullptr);
    }

    std::shared_ptr<rsa_json_rpc_t> jsonRpc{};
    long reqSenderSvcId = -1;
};

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToGetServiceVersionFromEndpointDescription) {
    auto endpoint = CreateEndpointDescription();
    celix_properties_unset(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION);
    long proxySvcId = -1;
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, reqSenderSvcId, &proxySvcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SUCCESS, proxySvc->test(proxySvc->handle));
    });
    EXPECT_FALSE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxySvcId);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, ServiceVersionUncompatible) {
    auto endpoint = CreateEndpointDescription();
    celix_properties_set(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "2.0.0");//It is 1.0.0 in the descriptor file of consumer
    long proxySvcId = -1;
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, reqSenderSvcId, &proxySvcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SUCCESS, proxySvc->test(proxySvc->handle));
    });
    EXPECT_FALSE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxySvcId);
}

class  RsaJsonRpcProxyUnitTestSuite2 : public RsaJsonRpcProxyUnitTestSuite {
public:
    RsaJsonRpcProxyUnitTestSuite2() {
        auto endpoint = CreateEndpointDescription();
        auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, reqSenderSvcId, &proxySvcId);
        EXPECT_EQ(CELIX_SUCCESS, status);
        endpointDescription_destroy(endpoint);

        celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration
    }
    ~RsaJsonRpcProxyUnitTestSuite2() override {
        rsaJsonRpc_destroyProxy(jsonRpc.get(), proxySvcId);
    }
    long proxySvcId{-1};
};

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToCreateProxiesHashMap) {
    auto endpoint = CreateEndpointDescription();
    long svcId = -1L;
    celix_ei_expect_celix_longHashMap_create((void*)&rsaJsonRpc_createProxy, 1, nullptr);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, reqSenderSvcId, &svcId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToCloneEndpointDescription) {
    auto endpoint = CreateEndpointDescription();
    long svcId = -1L;
    celix_ei_expect_calloc((void*)&endpointDescription_clone, 0, nullptr);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, reqSenderSvcId, &svcId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToRegisterProxyService) {
    auto endpoint = CreateEndpointDescription();
    long svcId = -1L;
    celix_ei_expect_celix_bundleContext_registerServiceFactoryAsync((void*)&rsaJsonRpcProxy_factoryCreate, 0, -1);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, reqSenderSvcId, &svcId);
    EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, CallProxyService) {
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SUCCESS, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToFindInterfaceDescriptor) {
    setenv("CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR"/non-exist", true);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SUCCESS, proxySvc->test(proxySvc->handle));
    });
    EXPECT_FALSE(found);
    unsetenv("CELIX_FRAMEWORK_EXTENDER_PATH");
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateVersionFromString) {
    celix_ei_expect_celix_version_createVersionFromString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SUCCESS, proxySvc->test(proxySvc->handle));
    });
    EXPECT_FALSE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateFunctionClosure) {
    celix_ei_expect_dynFunction_createClosure(CELIX_EI_UNKNOWN_CALLER, 0, 1);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SERVICE_EXCEPTION, proxySvc->test(proxySvc->handle));
    });
    EXPECT_FALSE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToPrepareInvokeRequest) {
    celix_ei_expect_jsonRpc_prepareInvokeRequest(CELIX_EI_UNKNOWN_CALLER, 0, 1);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SERVICE_EXCEPTION, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateMetadata) {
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        EXPECT_EQ(CELIX_ENOMEM, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToSendRequest) {
    reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
        (void)handle;//unused
        (void)endpointDesc;//unused
        (void)metadata;//unused
        (void)request;//unused
        (void)response;//unused
        return CELIX_ILLEGAL_STATE;
    };

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, ResponseIsNull) {
    reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc,
                                  celix_properties_t *metadata, const struct iovec *request,
                                  struct iovec *response) -> celix_status_t {
        (void) handle;//unused
        (void) endpointDesc;//unused
        (void) metadata;//unused
        (void) request;//unused
        response->iov_base = nullptr;//set to nullptr
        response->iov_len = 0;
        return CELIX_SUCCESS;
    };
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr,[](void *handle, void *svc) {
            (void) handle;//unused
            auto proxySvc = static_cast<rsa_rpc_json_test_service_t *>(svc);
            EXPECT_NE(nullptr, proxySvc);
            EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, proxySvc->test(proxySvc->handle));
        });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, ResponseIsInvalid) {
    reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
        (void)handle;//unused
        (void)endpointDesc;//unused
        (void)metadata;//unused
        (void)request;//unused
        response->iov_base = nullptr;//set to nullptr
        response->iov_len = 0;
        return CELIX_SUCCESS;
    };
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);

    //response is invalid json
    reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
        (void)handle;//unused
        (void)endpointDesc;//unused
        (void)metadata;//unused
        (void)request;//unused
        response->iov_base = strdup("invalid");//set to invalid
        response->iov_len = 0;
        return CELIX_SUCCESS;
    };
    found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SERVICE_EXCEPTION, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, RemoteServiceReturnsError) {
    //response is null
    reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
        (void)handle;//unused
        (void)endpointDesc;//unused
        (void)metadata;//unused
        (void)request;//unused
        response->iov_base = strdup("{\"e\":70003}");//set error code
        response->iov_len = strlen((const char*)response->iov_base);
        return CELIX_SUCCESS;
    };
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(70003, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, ServiceInvocationIsIntercepted) {
    static remote_interceptor_t interceptor{};
    interceptor.preProxyCall = [](void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) -> bool {
                (void)handle;//unused
                (void)svcProperties;//unused
                (void)functionName;//unused
                (void)metadata;//unused
                return false;
            };
    interceptor.postProxyCall = [](void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
                (void)handle;//unused
                (void)svcProperties;//unused
                (void)functionName;//unused
                (void)metadata;//unused
                return;
            };

    celix_service_registration_options_t opts{};
    opts.serviceName = REMOTE_INTERCEPTOR_SERVICE_NAME;
    opts.serviceVersion = REMOTE_INTERCEPTOR_SERVICE_VERSION;
    opts.svc = &interceptor;
    auto interceptorSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx.get(), &opts);
    celix_bundleContext_waitForAsyncRegistration(ctx.get(), interceptorSvcId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_INTERCEPTOR_EXCEPTION, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);

    celix_bundleContext_unregisterServiceAsync(ctx.get(), interceptorSvcId, nullptr, nullptr);
}

class RsaJsonRpcEndPointUnitTestSuite : public RsaJsonRpcUnitTestSuite {
public:
    RsaJsonRpcEndPointUnitTestSuite() {
        rsa_json_rpc_t *jsonRpcPtr = nullptr;
        celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
        auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpcPtr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, jsonRpcPtr);
        celix_ei_expect_celix_bundle_getManifestValue(nullptr, 0, nullptr);//reset for next test
        jsonRpc = std::shared_ptr<rsa_json_rpc_t>{jsonRpcPtr, [](auto* r){rsaJsonRpc_destroy(r);}};

        static rsa_rpc_json_test_service_t testSvc{};
        testSvc.handle = nullptr;
        testSvc.test = [](void *handle) -> int {
            (void)handle;//unused
            return CELIX_SUCCESS;
        };
        celix_service_registration_options_t opts{};
        opts.serviceName = RSA_RPC_JSON_TEST_SERVICE;
        opts.serviceVersion = RSA_RPC_JSON_TEST_SERVICE_VERSION;
        opts.svc = &testSvc;
        rpcTestSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx.get(), &opts);
        EXPECT_NE(-1, rpcTestSvcId);

    }

    ~RsaJsonRpcEndPointUnitTestSuite() override {
        celix_bundleContext_unregisterServiceAsync(ctx.get(), rpcTestSvcId, nullptr, nullptr);
    }

    unsigned int GenerateSerialProtoId() {//The same as rsaJsonRpc_generateSerialProtoId
        const char *bundleSymName = celix_bundle_getSymbolicName(celix_bundleContext_getBundle(ctx.get()));
        return celix_utils_stringHash(bundleSymName) + 1;
    }

    std::shared_ptr<rsa_json_rpc_t> jsonRpc{};
    long rpcTestSvcId = -1;
};

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToCreateEndpointLock) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);

    celix_ei_expect_celixThreadRwlock_create((void*)&rsaJsonRpcEndpoint_create, 0, CELIX_ENOMEM);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToCloneEndpointDescription) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_ei_expect_calloc((void *)&endpointDescription_clone, 0, nullptr);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToTrackEndpointService) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);

    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&rsaJsonRpcEndpoint_create, 0, -1);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToRegisterRequestHandler) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);

    celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync((void*)&rsaJsonRpcEndpoint_create, 0, -1);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, UseRequestHandler) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};
        EXPECT_EQ(CELIX_SUCCESS, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));
        free(reply.iov_base);
    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToFindInterfaceDescriptor) {
    setenv("CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR"/non-exist", true);
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};
        EXPECT_EQ(CELIX_ILLEGAL_STATE, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));
        free(reply.iov_base);
    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
    unsetenv("CELIX_FRAMEWORK_EXTENDER_PATH");
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToGetServiceVersionFromInterfaceDescriptor) {
    celix_ei_expect_dynInterface_getVersionString(CELIX_EI_UNKNOWN_CALLER, 0, 1);

    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};
        EXPECT_EQ(CELIX_ILLEGAL_STATE, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));
        free(reply.iov_base);
    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, ServiceVersionMismatched) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_properties_set(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "2.0.0");//Its 1.0.0 in the interface descriptor
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};
        EXPECT_EQ(CELIX_ILLEGAL_STATE, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));
        free(reply.iov_base);
    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, EndpointLostServiceVersion) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_properties_unset(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};
        EXPECT_EQ(CELIX_ILLEGAL_STATE, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));
        free(reply.iov_base);
    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, UseRequestHandlerWithInvalidParams) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};

        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(nullptr, metadata, &request, &reply));

        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(reqHandler->handle, nullptr, &request, &reply));

        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(reqHandler->handle, metadata, nullptr, &reply));

        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(reqHandler->handle, metadata, &request, nullptr));

        request.iov_base =  (char *)"{\"a\": []}";// lost method node
        request.iov_len = strlen((char*)request.iov_base);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));

        request.iov_base =  (char *)"invalid";
        request.iov_len = strlen((char*)request.iov_base);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));

        celix_properties_setLong(metadata, "SerialProtocolId", 1);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));

    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, ServiceInvocationIsIntercepted) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_properties_unset(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    static remote_interceptor_t interceptor{};
    interceptor.preExportCall = [](void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) -> bool {
        (void)handle;//unused
        (void)svcProperties;//unused
        (void)functionName;//unused
        (void)metadata;//unused
        return false;
    };
    interceptor.postExportCall = [](void *handle, const celix_properties_t *svcProperties, const char *functionName, celix_properties_t *metadata) {
        (void)handle;//unused
        (void)svcProperties;//unused
        (void)functionName;//unused
        (void)metadata;//unused
        return;
    };
    celix_service_registration_options_t opts{};
    opts.serviceName = REMOTE_INTERCEPTOR_SERVICE_NAME;
    opts.serviceVersion = REMOTE_INTERCEPTOR_SERVICE_VERSION;
    opts.svc = &interceptor;
    auto interceptorSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx.get(), &opts);
    celix_bundleContext_waitForAsyncRegistration(ctx.get(), interceptorSvcId);

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    auto found = celix_bundleContext_useService(ctx.get(), RSA_REQUEST_HANDLER_SERVICE_NAME, metadata, [](void *handle, void *svc) {
        celix_properties_t *metadata = static_cast< celix_properties_t *>(handle);//unused
        auto reqHandler = static_cast<rsa_request_handler_service_t*>(svc);
        EXPECT_NE(nullptr, reqHandler);
        struct iovec request{};
        request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
        request.iov_len = strlen((char*)request.iov_base);
        struct iovec reply{nullptr,0};
        EXPECT_EQ(CELIX_INTERCEPTOR_EXCEPTION, reqHandler->handleRequest(reqHandler->handle, metadata, &request, &reply));
        free(reply.iov_base);
    });
    EXPECT_TRUE(found);

    celix_properties_destroy(metadata);


    celix_bundleContext_unregisterServiceAsync(ctx.get(), interceptorSvcId, nullptr, nullptr);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), svcId);
    endpointDescription_destroy(endpoint);
}


