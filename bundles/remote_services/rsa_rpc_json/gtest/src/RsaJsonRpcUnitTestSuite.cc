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
#include "rsa_json_rpc_proxy_impl.h"
#include "rsa_json_rpc_endpoint_impl.h"
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
#include "celix_framework_version.h"
#include <gtest/gtest.h>
#include <cstdlib>
extern "C" {
#include "remote_interceptors_handler.h"
}

static celix_status_t SendRequest(void *handle, const endpoint_description_t *endpointDescription,
                                   celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
    (void) handle;
    (void) endpointDescription;
    (void) metadata;
    (void) request;
    response->iov_base = strdup("{}");
    response->iov_len = 2;
    return CELIX_SUCCESS;
}

class RsaJsonRpcUnitTestSuite : public ::testing::Test {
public:
    RsaJsonRpcUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_json_rpc_impl_cache");
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
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_registerServiceFactoryAsync(nullptr, 0, 0);
        celix_ei_expect_celix_version_createVersionFromString(nullptr, 0, nullptr);
        celix_ei_expect_dynFunction_createClosure(nullptr, 0, 0);
        celix_ei_expect_jsonRpc_prepareInvokeRequest(nullptr, 0, 0);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptions(nullptr, 0, 0);
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
        celix_properties_set(endpointDesc->properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
        celix_properties_set(endpointDesc->properties, CELIX_FRAMEWORK_SERVICE_NAME, RSA_RPC_JSON_TEST_SERVICE);
        celix_properties_set(endpointDesc->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "1.0.0");
        celix_properties_set(endpointDesc->properties, CELIX_RSA_ENDPOINT_ID, "8cf05b2d-421e-4c46-b55e-c3f1900b7cba");
        celix_properties_set(endpointDesc->properties, CELIX_RSA_SERVICE_IMPORTED, "true");
        endpointDesc->frameworkUUID = (char*)celix_properties_get(endpointDesc->properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, nullptr);
        endpointDesc->serviceId = svcId;
        endpointDesc->id = (char*)celix_properties_get(endpointDesc->properties, CELIX_RSA_ENDPOINT_ID, nullptr);
        endpointDesc->serviceName = strdup(RSA_RPC_JSON_TEST_SERVICE);
        return endpointDesc;
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
};

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpc) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, jsonRpc);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = nullptr;

    auto status  = rsaJsonRpc_create(nullptr, logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status  = rsaJsonRpc_create(ctx.get(), nullptr, &jsonRpc);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithENOMEM) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_calloc((void*)&rsaJsonRpc_create, 0, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithInvalidVersion) {
    rsa_json_rpc_t *jsonRpc = nullptr;

    celix_ei_expect_celix_bundle_getVersion((void*)&rsaJsonRpc_create, 1, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRsaJsonRpcWithInvalidBundleSymbolicName) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_autoptr(celix_version_t) version = celix_version_createVersionFromString("1.0.0");
    celix_ei_expect_celix_bundle_getVersion((void*)&rsaJsonRpc_create, 1, version);

    celix_ei_expect_celix_bundle_getSymbolicName((void*)&rsaJsonRpc_create, 1, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, FailedToCreateThreadMutex) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_autoptr(celix_version_t) version = celix_version_createVersionFromString("1.0.0");
    celix_ei_expect_celix_bundle_getVersion((void*)&rsaJsonRpc_create, 1, version);

    celix_ei_expect_celixThreadMutex_create((void*)&rsaJsonRpc_create, 0, CELIX_ENOMEM);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, FailedToCreateRemoteInterceptorsHandler) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    celix_ei_expect_calloc((void*)&remoteInterceptorsHandler_create, 0, nullptr);
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRpcProxy) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long proxyId = -1;
    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(-1, proxyId);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroyProxy(jsonRpc, proxyId);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateRpcProxyWithInvalidParams) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long proxyId = -1;
    status = rsaJsonRpc_createProxy(jsonRpc, nullptr, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, nullptr, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, SendRequest, nullptr, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, RpcProxyFailedToCreateProxyFactory) {
    rsa_json_rpc_t *jsonRpc = nullptr;
    auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpc);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto endpoint = CreateEndpointDescription();
    long proxyId = -1;
    celix_ei_expect_calloc((void*)&rsaJsonRpcProxy_factoryCreate, 0, nullptr);
    status = rsaJsonRpc_createProxy(jsonRpc, endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);

    rsaJsonRpc_destroy(jsonRpc);
}

TEST_F(RsaJsonRpcUnitTestSuite, DestroyRpcProxyWithInvalidParams) {
    rsaJsonRpc_destroyProxy(nullptr, 101);
}

TEST_F(RsaJsonRpcUnitTestSuite, CreateEndpoint) {
    rsa_json_rpc_t *jsonRpc = nullptr;
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
    rsaJsonRpc_destroyEndpoint(nullptr, 101);
}


class RsaJsonRpcProxyUnitTestSuite : public RsaJsonRpcUnitTestSuite {
public:
    RsaJsonRpcProxyUnitTestSuite() {
        rsa_json_rpc_t *jsonRpcPtr = nullptr;
        auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpcPtr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, jsonRpcPtr);
        jsonRpc = std::shared_ptr<rsa_json_rpc_t>{jsonRpcPtr, [](auto* r){rsaJsonRpc_destroy(r);}};
    }

    ~RsaJsonRpcProxyUnitTestSuite() override {

    };

    std::shared_ptr<rsa_json_rpc_t> jsonRpc{};
};

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToGetServiceVersionFromEndpointDescription) {
    auto endpoint = CreateEndpointDescription();
    celix_properties_unset(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION);
    long proxyId = -1;
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, ServiceVersionUncompatible) {
    auto endpoint = CreateEndpointDescription();
    celix_properties_set(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "2.0.0");//It is 1.0.0 in the descriptor file of consumer
    long proxyId = -1;
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);

    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
}

class  RsaJsonRpcProxyUnitTestSuite2 : public RsaJsonRpcProxyUnitTestSuite {
public:
    RsaJsonRpcProxyUnitTestSuite2() {
        auto endpoint = CreateEndpointDescription();
        auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
        EXPECT_EQ(CELIX_SUCCESS, status);
        endpointDescription_destroy(endpoint);

        celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration
    }
    ~RsaJsonRpcProxyUnitTestSuite2() override {
        rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
    }
    long proxyId{-1};
};

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToCreateSendRequestMethodLock) {
    celix_autoptr(endpoint_description_t) endpoint = CreateEndpointDescription();
    long proxyId = -1L;
    celix_ei_expect_celixThreadRwlock_create((void*)&rsaJsonRpcProxy_factoryCreate, 0, CELIX_ENOMEM);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToCreateProxiesHashMap) {
    auto endpoint = CreateEndpointDescription();
    long proxyId = -1L;
    celix_ei_expect_celix_longHashMap_create((void*)&rsaJsonRpc_createProxy, 1, nullptr);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToCloneEndpointDescription) {
    auto endpoint = CreateEndpointDescription();
    long proxyId = -1L;
    celix_ei_expect_calloc((void*)&endpointDescription_clone, 0, nullptr);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToCreateProxyServiceProperties) {
    celix_autoptr(endpoint_description_t)  endpoint = CreateEndpointDescription();
    long proxyId = -1L;
    celix_ei_expect_celix_properties_copy((void*)&rsaJsonRpcProxy_factoryCreate, 1, nullptr, 2);//first:endpointDescription_clone, second:celix_rsaUtils_createServicePropertiesFromEndpointProperties
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToRegisterProxyService) {
    auto endpoint = CreateEndpointDescription();
    long proxyId = -1L;
    celix_ei_expect_celix_bundleContext_registerServiceFactoryAsync((void*)&rsaJsonRpcProxy_factoryCreate, 0, -1);
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, SendRequest, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateAllocMmeoryForProxyWhenGetService) {
    celix_ei_expect_calloc((void*)-1, 0, nullptr);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateAllocMmeoryForProxyServiceWhenGetService) {
    celix_ei_expect_calloc((void*)-1, 0, nullptr, 2);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);
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
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);
    unsetenv("CELIX_FRAMEWORK_EXTENDER_PATH");
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateVersionFromString) {
    celix_ei_expect_celix_version_createVersionFromString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, FailedToCreateFunctionClosure) {
    celix_ei_expect_dynFunction_createClosure(CELIX_EI_UNKNOWN_CALLER, 0, 1);
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void*, void*) {});
    EXPECT_FALSE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, InvokeProxyServiceWithInvalidParams) {
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, proxySvc->test(nullptr));
    });
    EXPECT_TRUE(found);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite2, InvokeProxyServiceWhenProxyIsDestroying) {
    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, this, [](void *handle, void *svc) {
        auto self = static_cast<RsaJsonRpcProxyUnitTestSuite2*>(handle);
        rsaJsonRpc_destroyProxy(self->jsonRpc.get(), self->proxyId);
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);
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

TEST_F(RsaJsonRpcProxyUnitTestSuite, FailedToSendRequest) {
    auto endpoint = CreateEndpointDescription();
    long proxyId{-1};
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, [](void*, const endpoint_description_t*, celix_properties_t*,
            const struct iovec*, struct iovec*) { return CELIX_ILLEGAL_STATE;}, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);
    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, ResponseIsNull) {
    auto endpoint = CreateEndpointDescription();
    long proxyId{-1};
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, [](void*, const endpoint_description_t*, celix_properties_t*,
                                                                     const struct iovec*, struct iovec* response) {
        response->iov_base = nullptr;//set to nullptr
        response->iov_len = 0;
        return CELIX_SUCCESS;}, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);
    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr,[](void *handle, void *svc) {
            (void) handle;//unused
            auto proxySvc = static_cast<rsa_rpc_json_test_service_t *>(svc);
            EXPECT_NE(nullptr, proxySvc);
            EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, proxySvc->test(proxySvc->handle));
        });
    EXPECT_TRUE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, ResponseIsInvalid) {
    auto endpoint = CreateEndpointDescription();
    long proxyId{-1};
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, [](void*, const endpoint_description_t*, celix_properties_t*,
                                                                     const struct iovec*, struct iovec* response) {
        response->iov_base = strdup("invalid");//set to invalid
        response->iov_len = 0;
        return CELIX_SUCCESS;}, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);
    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(CELIX_SERVICE_EXCEPTION, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
}

TEST_F(RsaJsonRpcProxyUnitTestSuite, RemoteServiceReturnsError) {
    auto endpoint = CreateEndpointDescription();
    long proxyId{-1};
    auto status = rsaJsonRpc_createProxy(jsonRpc.get(), endpoint, [](void*, const endpoint_description_t*, celix_properties_t*,
                                                                     const struct iovec*, struct iovec* response) {
        response->iov_base = strdup("{\"e\":70003}");//set error code
        response->iov_len = strlen((const char*)response->iov_base);
        return CELIX_SUCCESS;}, nullptr, &proxyId);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpointDescription_destroy(endpoint);
    celix_bundleContext_waitForEvents(ctx.get());//wait for proxy service registration

    auto found = celix_bundleContext_useService(ctx.get(), RSA_RPC_JSON_TEST_SERVICE, nullptr, [](void *handle, void *svc) {
        (void)handle;//unused
        auto proxySvc = static_cast<rsa_rpc_json_test_service_t*>(svc);
        EXPECT_NE(nullptr, proxySvc);
        EXPECT_EQ(70003, proxySvc->test(proxySvc->handle));
    });
    EXPECT_TRUE(found);

    rsaJsonRpc_destroyProxy(jsonRpc.get(), proxyId);
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
    opts.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
    opts.serviceVersion = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_VERSION;
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
        auto status  = rsaJsonRpc_create(ctx.get(), logHelper.get(), &jsonRpcPtr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, jsonRpcPtr);
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
        return celix_utils_stringHash(bundleSymName) + CELIX_FRAMEWORK_VERSION_MAJOR;
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

    celix_ei_expect_celix_bundleContext_trackServicesWithOptions((void*)&rsaJsonRpcEndpoint_create, 0, -1);
    long svcId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &svcId);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, HandleRequest) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};
    EXPECT_EQ(CELIX_SUCCESS, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));
    free(reply.iov_base);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, FailedToFindInterfaceDescriptor) {
    setenv("CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR"/non-exist", true);
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};
    EXPECT_EQ(CELIX_ILLEGAL_STATE, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));
    free(reply.iov_base);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
    unsetenv("CELIX_FRAMEWORK_EXTENDER_PATH");
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, ServiceVersionMismatched) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_properties_set(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "2.0.0");//Its 1.0.0 in the interface descriptor
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};
    EXPECT_EQ(CELIX_ILLEGAL_STATE, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));
    free(reply.iov_base);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, EndpointLostServiceVersion) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_properties_unset(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION);
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};
    EXPECT_EQ(CELIX_ILLEGAL_STATE, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));
    free(reply.iov_base);

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, UseRequestHandlerWithInvalidParams) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(nullptr, epId, metadata, &request, &reply));

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, nullptr, &request, &reply));

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, nullptr, &reply));

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, nullptr));

    EXPECT_EQ(CELIX_ILLEGAL_STATE, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), 10000/*non-existing epId*/, metadata, &request, &reply));

    request.iov_base =  (char *)"{\"a\": []}";// lost method node
    request.iov_len = strlen((char*)request.iov_base);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));

    request.iov_base =  (char *)"invalid";
    request.iov_len = strlen((char*)request.iov_base);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));

    celix_properties_setLong(metadata, "SerialProtocolId", 1);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, ServiceInvocationIsIntercepted) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    celix_properties_unset(endpoint->properties, CELIX_FRAMEWORK_SERVICE_VERSION);
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
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
    opts.serviceName = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_NAME;
    opts.serviceVersion = CELIX_RSA_REMOTE_INTERCEPTOR_SERVICE_VERSION;
    opts.svc = &interceptor;
    auto interceptorSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx.get(), &opts);
    celix_bundleContext_waitForAsyncRegistration(ctx.get(), interceptorSvcId);

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};
    EXPECT_EQ(CELIX_INTERCEPTOR_EXCEPTION, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));
    free(reply.iov_base);

    celix_properties_destroy(metadata);


    celix_bundleContext_unregisterServiceAsync(ctx.get(), interceptorSvcId, nullptr, nullptr);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
}

TEST_F(RsaJsonRpcEndPointUnitTestSuite, JsonRpcCallFailed) {
    auto endpoint = CreateEndpointDescription(rpcTestSvcId);
    long epId = -1L;
    auto status = rsaJsonRpc_createEndpoint(jsonRpc.get(), endpoint, &epId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_bundleContext_waitForEvents(ctx.get());//wait for async endpoint creation

    unsigned int serialProtoId = GenerateSerialProtoId();
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    celix_ei_expect_jsonRpc_call(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    struct iovec request{};
    request.iov_base =  (char *)"{\n    \"m\": \"test\",\n    \"a\": []\n}";
    request.iov_len = strlen((char*)request.iov_base);
    struct iovec reply{nullptr,0};
    EXPECT_EQ(CELIX_SERVICE_EXCEPTION, celix_rsaJsonRpc_handleRequest(jsonRpc.get(), epId, metadata, &request, &reply));

    celix_properties_destroy(metadata);

    rsaJsonRpc_destroyEndpoint(jsonRpc.get(), epId);
    endpointDescription_destroy(endpoint);
}
