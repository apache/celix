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
#include "rsa_shm_impl.h"
#include "rsa_shm_client.h"
#include "rsa_shm_server.h"
#include "rsa_shm_export_registration.h"
#include "rsa_shm_constants.h"
#include "RsaShmTestService.h"
#include "remote_constants.h"
#include "endpoint_description.h"
#include "celix_log_helper.h"
#include "celix_constants.h"
#include "celix_framework_factory.h"
#include "celix_errno.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "asprintf_ei.h"
#include "celix_utils_ei.h"
#include "celix_properties_ei.h"
#include "celix_log_helper_ei.h"
#include "celix_string_hash_map_ei.h"
#include <string>
#include <gtest/gtest.h>
#include <functional>

static int calculator_add(void *calculator, double a, double b, double *result) {
    (void) calculator;//unused

    *result = a + b;

    return CELIX_SUCCESS;
}

class RsaShmUnitTestSuite : public ::testing::Test {
public:
    RsaShmUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_impl_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
    }

    ~RsaShmUnitTestSuite() override {
        celix_ei_expect_celix_logHelper_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getBundleId(nullptr, 0, 0);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_bundleContext_getServiceReferences(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_trim(nullptr, 2, nullptr);
        celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
    }

    void RegisterCalculatorService() {
        static rsa_shm_calc_service_t calcService{};
        calcService.handle = nullptr,
        calcService.add = calculator_add;
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, RSA_SHM_CALCULATOR_SERVICE);
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE);
        celix_properties_set(properties, RSA_SHM_RPC_TYPE_KEY, RSA_SHM_RPC_TYPE_DEFAULT);

        calcSvcId = celix_bundleContext_registerService(ctx.get(), &calcService, RSA_SHM_CALCULATOR_SERVICE, properties);
    };

    void UnregisterCalculatorService() {
        celix_bundleContext_unregisterService(ctx.get(), calcSvcId);
    }

    endpoint_description_t *CreateEndpointDescription() {
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, RSA_SHM_CALCULATOR_SERVICE);
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE);
        celix_properties_set(properties, RSA_SHM_RPC_TYPE_KEY, RSA_SHM_RPC_TYPE_DEFAULT);
        celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "7f7efba5-500f-4ee9-b733-68de012091da");
        celix_properties_set(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, "1234");
        celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
        celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "0a068612-9f95-4f1b-bf0b-ffa6916dae12");
        celix_properties_set(properties, RSA_SHM_SERVER_NAME_KEY, "ShmServ-dummy");
        endpoint_description_t *endpoint = nullptr;
        auto status = endpointDescription_create(properties, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, status);
        return endpoint;
    }

    void TestRsaShm(const std::function<void(rsa_shm_t*)>& testBody) {
        rsa_shm_t *admin = nullptr;
        auto status = rsaShm_create(ctx.get(), &admin);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_NE(nullptr, admin);

        static celix_rsa_rpc_factory_t rpcFactory{};
        rpcFactory.handle = nullptr;
        rpcFactory.createProxy = [](void*, const endpoint_description_t*, celix_rsa_send_request_fp, void*, long *proxyId){
            *proxyId = 1;
            return CELIX_SUCCESS;
        };
        rpcFactory.destroyProxy = [](void*, long) {};
        rpcFactory.createEndpoint = [](void*, const endpoint_description_t*, long *endpointId){
            *endpointId = 1;
            return CELIX_SUCCESS;
        };
        rpcFactory.destroyEndpoint = [](void*, long) {};
        rpcFactory.handleRequest = [](void*, long, celix_properties_t*, const struct iovec*, struct iovec*){
            return CELIX_SUCCESS;
        };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_RSA_RPC_TYPE_KEY, RSA_SHM_RPC_TYPE_DEFAULT);
        status = celix_rsaShm_addRpcFactorySvc(admin, &rpcFactory, props);
        EXPECT_EQ(CELIX_SUCCESS, status);

        testBody(admin);

        status = celix_rsaShm_removeRpcFactorySvc(admin, &rpcFactory, props);
        EXPECT_EQ(CELIX_SUCCESS, status);
        rsaShm_destroy(admin);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    long calcSvcId{-1};
};

TEST_F(RsaShmUnitTestSuite, CreateAndDestroyRsaShm) {
    rsa_shm_t *admin = nullptr;
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, admin);
    rsaShm_destroy(admin);
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateLogHelper) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_celix_logHelper_create((void*)&rsaShm_create, 0, nullptr);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToAllocMemoryForRsaShm) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_calloc((void*)&rsaShm_create, 0, nullptr);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateRpcFactoriesMap) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_celix_stringHashMap_create((void*)&rsaShm_create, 0, nullptr);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateServicesMutex) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_celixThreadMutex_create((void*)&rsaShm_create, 0, CELIX_ENOMEM);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);

    celix_ei_expect_celixThreadMutex_create((void*)&rsaShm_create, 0, CELIX_ENOMEM, 2);
    status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateClientManager) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_malloc((void*)&rsaShmClientManager_create, 0, nullptr);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToGetFrameworkUuid) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_celix_bundleContext_getProperty((void*)&rsaShm_create, 0, nullptr);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToGetBundleId) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_celix_bundleContext_getBundleId((void*)&rsaShm_create, 0, -1);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToGenerateShmServerName) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_asprintf((void*)&rsaShm_create, 0, -1);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateShmServer) {
    rsa_shm_t *admin = nullptr;
    celix_ei_expect_calloc((void*)&rsaShmServer_create, 0, nullptr);
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmUnitTestSuite, AddRpcFactoryService) {
    rsa_shm_t *admin = nullptr;
    auto status = rsaShm_create(ctx.get(), &admin);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, admin);

    static celix_rsa_rpc_factory_t rpcFactory{};
    rpcFactory.handle = nullptr;
    rpcFactory.createProxy = [](void*, const endpoint_description_t*, celix_rsa_send_request_fp, void*, long *proxyId){
        *proxyId = 1;
        return CELIX_SUCCESS;
    };
    rpcFactory.destroyProxy = [](void*, long) {};
    rpcFactory.createEndpoint = [](void*, const endpoint_description_t*, long *endpointId){
        *endpointId = 1;
        return CELIX_SUCCESS;
    };
    rpcFactory.destroyEndpoint = [](void*, long) {};
    rpcFactory.handleRequest = [](void*, long, celix_properties_t*, const struct iovec*, struct iovec*){
        return CELIX_SUCCESS;
    };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_set(props, CELIX_RSA_RPC_TYPE_KEY, RSA_SHM_RPC_TYPE_DEFAULT);
    status = celix_rsaShm_addRpcFactorySvc(admin, &rpcFactory, props);
    EXPECT_EQ(CELIX_SUCCESS, status);
    status = celix_rsaShm_removeRpcFactorySvc(admin, &rpcFactory, props);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_stringHashMap_put((void*)&celix_rsaShm_addRpcFactorySvc, 0, ENOMEM);
    status = celix_rsaShm_addRpcFactorySvc(admin, &rpcFactory, props);
    EXPECT_EQ(ENOMEM, status);

    celix_properties_unset(props, CELIX_RSA_RPC_TYPE_KEY);
    status = celix_rsaShm_addRpcFactorySvc(admin, &rpcFactory, props);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    status = celix_rsaShm_removeRpcFactorySvc(admin, &rpcFactory, props);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    rsaShm_destroy(admin);
}

TEST_F(RsaShmUnitTestSuite, ExportService) {
    TestRsaShm([this](rsa_shm_t* admin) {

        RegisterCalculatorService();

        celix_array_list_t *regs = nullptr;
        celix_properties_t *prop = celix_properties_create();
        celix_properties_set(prop, "AdditionKey", "AdditionValue");
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), prop, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_GE(1, celix_arrayList_size(regs));
        for (int i = 0; i < celix_arrayList_size(regs); i++) {
            export_registration_t *reg = static_cast<export_registration_t *>(celix_arrayList_get(regs, i));
            export_reference_t *ref = nullptr;
            status = exportRegistration_getExportReference(reg, &ref);
            EXPECT_EQ(CELIX_SUCCESS, status);
            endpoint_description_t *endpoint = nullptr;
            status = exportReference_getExportedEndpoint(ref, &endpoint);
            EXPECT_EQ(CELIX_SUCCESS, status);
            EXPECT_STREQ("AdditionValue", celix_properties_get(endpoint->properties, "AdditionKey", nullptr));
            EXPECT_EQ(nullptr,
                      celix_properties_get(endpoint->properties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, nullptr));
            EXPECT_STREQ(RSA_SHM_CALCULATOR_SERVICE,
                         celix_properties_get(endpoint->properties, CELIX_FRAMEWORK_SERVICE_NAME, nullptr));
            EXPECT_STREQ("true", celix_properties_get(endpoint->properties, CELIX_RSA_SERVICE_IMPORTED, nullptr));
            EXPECT_EQ(calcSvcId, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_ENDPOINT_SERVICE_ID, -1));
            free(ref);

            status = rsaShm_removeExportedService(admin, reg);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        celix_arrayList_destroy(regs);
        celix_properties_destroy(prop);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ExportServiceWithObjectClass) {
    TestRsaShm([this](rsa_shm_t* admin){
        RegisterCalculatorService();

        celix_properties_t *prop = celix_properties_create();
        celix_properties_set(prop, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, "*");
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), prop, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_GE(1, celix_arrayList_size(regs));
        for(int i = 0; i < celix_arrayList_size(regs); i++) {
            export_registration_t *reg= static_cast<export_registration_t *>(celix_arrayList_get(regs, i));
            status = rsaShm_removeExportedService(admin, reg);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        celix_arrayList_destroy(regs);
        celix_properties_destroy(prop);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateExportedServiceProperties) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_array_list_t *regs = nullptr;
        celix_ei_expect_celix_properties_create((void *) &rsaShm_exportService, 0, nullptr);
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_ENOMEM, status);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ExportedInterfaceNotMatchObjectClass) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_properties_t *prop = celix_properties_create();
        celix_properties_set(prop, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, "unmatched-interface");
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), prop, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);
        celix_properties_destroy(prop);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ExportServiceWithUnmatchedConfigType) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_properties_t *prop = celix_properties_create();
        celix_properties_set(prop, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, "unmatched-config-type");
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), prop, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);
        celix_properties_destroy(prop);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ExportServiceWithUnmatchedRpcType) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_properties_t *prop = celix_properties_create();
        celix_properties_set(prop, RSA_SHM_RPC_TYPE_KEY, "unmatched-rpc-type");
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), prop, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);
        celix_properties_destroy(prop);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ServiceLostExportedInterfaceProperty) {
    TestRsaShm([this](rsa_shm_t* admin) {
        static rsa_shm_calc_service_t calcService{
                .handle = nullptr,
                .add = calculator_add,
        };
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE);
        calcSvcId = celix_bundleContext_registerService(ctx.get(), &calcService, RSA_SHM_CALCULATOR_SERVICE,
                                                        properties);

        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

        UnregisterCalculatorService();

    });
}

TEST_F(RsaShmUnitTestSuite, FailedTrimmedExportedInterfaces) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_array_list_t *regs = nullptr;
        celix_ei_expect_celix_utils_trim((void *) &rsaShm_exportService, 0, nullptr);
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_ENOMEM, status);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, CreateEndpointFailed1) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_ei_expect_celix_bundleContext_getProperty((void *) &rsaShm_exportService, 1, nullptr);
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, CreateEndpointFailed2) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        //Failed to get rpc type
        celix_ei_expect_celix_utils_strdup((void *) &rsaShm_exportService, 2, nullptr);
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);

        //Failed to dup default rpc type
        celix_ei_expect_celix_utils_trim((void *) &rsaShm_exportService, 2, nullptr);
        celix_ei_expect_celix_utils_strdup((void *) &rsaShm_exportService, 2, nullptr, 2);
        status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, CreateEndpointFailed3) {
    TestRsaShm([this](rsa_shm_t* admin) {

        RegisterCalculatorService();

        celix_ei_expect_celix_utils_strdup((void *) &endpointDescription_create, 0, nullptr);
        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(0, celix_arrayList_size(regs));
        celix_arrayList_destroy(regs);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ExportServiceFailed) {
    TestRsaShm([this](rsa_shm_t* admin) {
        celix_array_list_t *regs = nullptr;

        auto status = rsaShm_exportService(admin, nullptr, nullptr, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        std::string svcId = std::to_string(calcSvcId);

        status = rsaShm_exportService(admin, const_cast<char *>(svcId.c_str()), nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_exportService(nullptr, const_cast<char *>(svcId.c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(-1).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

        celix_ei_expect_bundleContext_getServiceReferences((void *) rsaShm_exportService, 0, CELIX_SERVICE_EXCEPTION);
        status = rsaShm_exportService(admin, const_cast<char *>(svcId.c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);

    });
}

TEST_F(RsaShmUnitTestSuite, RemoveExportServiceFailed1) {
    TestRsaShm([](rsa_shm_t* admin) {
        auto status = rsaShm_removeExportedService(admin, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_removeExportedService(nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    });
}

TEST_F(RsaShmUnitTestSuite, RemoveExportServiceFailed2) {
    TestRsaShm([this](rsa_shm_t* admin) {
        RegisterCalculatorService();

        celix_array_list_t *regs = nullptr;
        auto status = rsaShm_exportService(admin, const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        for (int i = 0; i < celix_arrayList_size(regs); i++) {
            export_registration_t *reg = static_cast<export_registration_t *>(celix_arrayList_get(regs, i));
            celix_ei_expect_calloc((void *) &rsaShm_removeExportedService, 1, nullptr);
            status = rsaShm_removeExportedService(admin, reg);
            EXPECT_EQ(CELIX_ENOMEM, status);
            status = rsaShm_removeExportedService(admin, reg);
            EXPECT_EQ(CELIX_SUCCESS, status);
        }
        celix_arrayList_destroy(regs);

        UnregisterCalculatorService();
    });
}

TEST_F(RsaShmUnitTestSuite, ImportService) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);

        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = rsaShm_removeImportedService(admin, regs);
        EXPECT_EQ(CELIX_SUCCESS, status);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, ImportServiceWithUnmatchedConfigType) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);
        celix_properties_set(endpoint->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "unmatched-config-type");


        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(nullptr, regs);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, EndpointLostConfigType) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);
        celix_properties_unset(endpoint->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS);

        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);
        EXPECT_EQ(nullptr, regs);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, EndpointLostShmServerName) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);
        celix_properties_unset(endpoint->properties, RSA_SHM_SERVER_NAME_KEY);

        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        EXPECT_EQ(nullptr, regs);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateShmClientForImportedService) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);

        celix_ei_expect_celixThreadMutex_create((void *) &rsaShmClientManager_createOrAttachClient, 1, CELIX_ENOMEM);

        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_ENOMEM, status);
        EXPECT_EQ(nullptr, regs);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, FailedToCreateImportRegistration) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);

        celix_ei_expect_calloc((void *)&importRegistration_create, 0, nullptr);
        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(ENOMEM, status);
        EXPECT_EQ(nullptr, regs);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, EndpointLostRpcType) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);
        celix_properties_unset(endpoint->properties, RSA_SHM_RPC_TYPE_KEY);

        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        EXPECT_EQ(nullptr, regs);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, ImportServiceWithInvalidParameters) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        auto status = rsaShm_importService(admin, nullptr, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_importService(admin, nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_importService(nullptr, nullptr, &regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);

        status = rsaShm_importService(admin, endpoint, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, RemoveImportedServiceWithInvalidParameters) {
    TestRsaShm([this](rsa_shm_t* admin) {
        import_registration_t *regs = nullptr;

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);

        auto status = rsaShm_importService(admin, endpoint, &regs);
        EXPECT_EQ(CELIX_SUCCESS, status);

        status = rsaShm_removeImportedService(admin, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_removeImportedService(nullptr, regs);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_removeImportedService(admin, regs);
        EXPECT_EQ(CELIX_SUCCESS, status);

        endpointDescription_destroy(endpoint);
    });
}

TEST_F(RsaShmUnitTestSuite, CallRsaShmSendWithInvalidParameters) {
    TestRsaShm([this](rsa_shm_t* admin) {

        endpoint_description_t *endpoint = CreateEndpointDescription();
        EXPECT_NE(nullptr, endpoint);

        auto status = rsaShm_send(nullptr, nullptr, nullptr, nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_send(admin, nullptr, nullptr, nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        status = rsaShm_send(admin, endpoint, nullptr, nullptr, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
        char data[] = "{ }";
        const struct iovec request = {data, strlen(data)};
        status = rsaShm_send(admin, endpoint, nullptr, &request, nullptr);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        struct iovec response = {nullptr, 0};
        status = rsaShm_send(admin, endpoint, nullptr, nullptr, &response);
        EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

        celix_properties_unset(endpoint->properties, RSA_SHM_SERVER_NAME_KEY);
        status = rsaShm_send(admin, endpoint, nullptr, &request, &response);
        EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);

        endpointDescription_destroy(endpoint);
    });
}

class RsaShmRpcTestSuite : public ::testing::Test {
public:
    RsaShmRpcTestSuite() {
        auto* clientProps = celix_properties_create();
        celix_properties_set(clientProps, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(clientProps, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_client_cache");
        celix_properties_set(clientProps, "CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR);
        auto* clientFwPtr = celix_frameworkFactory_createFramework(clientProps);
        auto* clientCtxPtr = celix_framework_getFrameworkContext(clientFwPtr);
        clientFw = std::shared_ptr<celix_framework_t>{clientFwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        clientCtx = std::shared_ptr<celix_bundle_context_t>{clientCtxPtr, [](auto*){/*nop*/}};
        auto bundleId = celix_bundleContext_installBundle(clientCtx.get(), RSA_JSON_RPC_BUNDLE, true);
        EXPECT_TRUE(bundleId >= 0);
        rsa_shm_t *clientAdminPtr = nullptr;
        auto status = rsaShm_create(clientCtx.get(), &clientAdminPtr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        clientAdmin = std::shared_ptr<rsa_shm_t>{clientAdminPtr, [](auto* a) {rsaShm_destroy(a);}};
        celix_service_tracking_options_t rpcFacTrkopts{};
        rpcFacTrkopts.filter.serviceName = CELIX_RSA_RPC_FACTORY_NAME;
        rpcFacTrkopts.filter.versionRange = CELIX_RSA_RPC_FACTORY_USE_RANGE;
        rpcFacTrkopts.callbackHandle = clientAdminPtr;
        rpcFacTrkopts.addWithProperties = [](void* handle, void* svc, const celix_properties_t* props) {
            (void)celix_rsaShm_addRpcFactorySvc(handle, svc, props);
        };
        rpcFacTrkopts.removeWithProperties = [](void* handle, void* svc, const celix_properties_t* props) {
            (void)celix_rsaShm_removeRpcFactorySvc(handle, svc, props);
        };
        clientRpcFacTrackId = celix_bundleContext_trackServicesWithOptionsAsync(clientCtx.get(), &rpcFacTrkopts);
        EXPECT_TRUE(clientRpcFacTrackId > 0);
        celix_bundleContext_waitForEvents(clientCtx.get());//Wait for all services to be ready

        auto* serverProps = celix_properties_create();
        celix_properties_set(serverProps, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(serverProps, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_server_cache");
        celix_properties_set(serverProps, "CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR);
        auto* serverFwPtr = celix_frameworkFactory_createFramework(serverProps);
        auto* serverCtxPtr = celix_framework_getFrameworkContext(serverFwPtr);
        serverFw = std::shared_ptr<celix_framework_t>{serverFwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        serverCtx = std::shared_ptr<celix_bundle_context_t>{serverCtxPtr, [](auto*){/*nop*/}};
        bundleId = celix_bundleContext_installBundle(serverCtx.get(), RSA_JSON_RPC_BUNDLE, true);
        EXPECT_TRUE(bundleId >= 0);
        rsa_shm_t *serverAdminPtr = nullptr;
        status = rsaShm_create(serverCtx.get(), &serverAdminPtr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        serverAdmin = std::shared_ptr<rsa_shm_t>{serverAdminPtr, [](auto* a) {rsaShm_destroy(a);}};
        static rsa_shm_calc_service_t calcService{
                .handle = nullptr,
                .add = calculator_add,
        };
        celix_properties_t *properties = celix_properties_create();
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_INTERFACES, RSA_SHM_CALCULATOR_SERVICE);
        celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, RSA_SHM_CALCULATOR_SERVICE_VERSION);
        celix_properties_set(properties, CELIX_RSA_SERVICE_EXPORTED_CONFIGS, RSA_SHM_CALCULATOR_CONFIGURATION_TYPE ",celix.remote.admin.rpc_type.json");
        calcSvcId = celix_bundleContext_registerService(serverCtx.get(), &calcService, RSA_SHM_CALCULATOR_SERVICE, properties);
        EXPECT_TRUE(calcSvcId >= 0);
        rpcFacTrkopts.callbackHandle = serverAdminPtr;
        serverRpcFacTrackId = celix_bundleContext_trackServicesWithOptionsAsync(serverCtx.get(), &rpcFacTrkopts);
        EXPECT_TRUE(serverRpcFacTrackId > 0);
        celix_bundleContext_waitForEvents(serverCtx.get());//Wait for all services to be ready
    }

    ~RsaShmRpcTestSuite() override {
        celix_bundleContext_stopTracker(serverCtx.get(), serverRpcFacTrackId);
        celix_bundleContext_unregisterService(serverCtx.get(), calcSvcId);
        celix_bundleContext_stopTracker(clientCtx.get(), clientRpcFacTrackId);
    }

    std::shared_ptr<celix_framework_t> clientFw{};
    std::shared_ptr<celix_bundle_context_t> clientCtx{};
    std::shared_ptr<rsa_shm_t> clientAdmin{};
    long clientRpcFacTrackId{-1};
    std::shared_ptr<celix_framework_t> serverFw{};
    std::shared_ptr<celix_bundle_context_t> serverCtx{};
    std::shared_ptr<rsa_shm_t> serverAdmin{};
    long calcSvcId{-1};
    long serverRpcFacTrackId{-1};
};


TEST_F(RsaShmRpcTestSuite, CallRemoteService) {
    //Export service
    celix_array_list_t *exportedRegs = nullptr;
    auto status = rsaShm_exportService(serverAdmin.get(), const_cast<char *>(std::to_string(calcSvcId).c_str()), nullptr, &exportedRegs);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_GE(1, celix_arrayList_size(exportedRegs));
    export_registration_t *exportReg= static_cast<export_registration_t *>(celix_arrayList_get(exportedRegs, 0));
    export_reference_t *ref = nullptr;
    status = exportRegistration_getExportReference(exportReg, &ref);
    EXPECT_EQ(CELIX_SUCCESS, status);
    endpoint_description_t *endpoint = nullptr;
    status = exportReference_getExportedEndpoint(ref, &endpoint);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(serverCtx.get());//Wait for all services to be ready

    //Import service
    import_registration_t *importReg = nullptr;
    status = rsaShm_importService(clientAdmin.get(), endpoint, &importReg);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(clientCtx.get());//Wait for all services to be ready

    //Use service
    celix_service_use_options_t opts{};
    opts.filter.serviceName = RSA_SHM_CALCULATOR_SERVICE;
    opts.callbackHandle = this;
    opts.use = [](void *handle, void *svc) {
        (void)handle;//unused
        auto *calc = static_cast<rsa_shm_calc_service_t *>(svc);
        double result;
        EXPECT_EQ(CELIX_SUCCESS, calc->add(calc->handle, 1, 2, &result));
        EXPECT_EQ(3.0, result);
    };
    auto found = celix_bundleContext_useServiceWithOptions(clientCtx.get(), &opts);
    EXPECT_TRUE(found);

    //Remove service
    free(ref);
    for(int i = 0; i < celix_arrayList_size(exportedRegs); i++) {
        export_registration_t *r= static_cast<export_registration_t *>(celix_arrayList_get(exportedRegs, i));
        status = rsaShm_removeExportedService(serverAdmin.get(), r);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }
    celix_arrayList_destroy(exportedRegs);

    //Use service again
    opts.filter.serviceName = RSA_SHM_CALCULATOR_SERVICE;
    opts.callbackHandle = this;
    opts.use = [](void *handle, void *svc) {
        (void)handle;//unused
        auto *calc = static_cast<rsa_shm_calc_service_t *>(svc);
        double result;
        EXPECT_NE(CELIX_SUCCESS, calc->add(calc->handle, 1, 2, &result));
    };
    found = celix_bundleContext_useServiceWithOptions(clientCtx.get(), &opts);
    EXPECT_TRUE(found);

    //Remove service proxy
    status = rsaShm_removeImportedService(clientAdmin.get(), importReg);
    EXPECT_EQ(CELIX_SUCCESS, status);
}




