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
#include "TopologyManagerTestSuiteBaseClass.h"
#include "celix_compiler.h"
#include "celix_errno.h"
#include "malloc_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_array_list_ei.h"
#include "celix_properties_ei.h"
#include "celix_utils_ei.h"
#include "celix_threads_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_filter_ei.h"
#include "asprintf_ei.h"

class TopologyManagerCreatingErrorInjectionTestSuite : public ::testing::Test {
public:
    TopologyManagerCreatingErrorInjectionTestSuite() {
        auto config = celix_properties_create();
        celix_properties_set(config, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(config, CELIX_FRAMEWORK_CACHE_DIR, ".tm_unit_test_cache");
        fw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(config),
                                                [](auto f) { celix_frameworkFactory_destroyFramework(f); }};
        ctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(fw.get()),
                                                      [](auto) {/*nop*/}};
        logHelper = std::shared_ptr<celix_log_helper_t>{celix_logHelper_create(ctx.get(), "tm_unit_test"),
                                                        [](auto l) { celix_logHelper_destroy(l); }};
    }

    ~TopologyManagerCreatingErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_putLong(nullptr, 1, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
};

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, AllocingMemoryErrorTest) {
    celix_ei_expect_calloc((void*)topologyManager_create, 0, nullptr);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingImportedServiceRankingOffsetMapErrorTest) {
    celix_ei_expect_celix_stringHashMap_create((void*)topologyManager_create, 0, nullptr);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, ImportedServiceRankingOffsetStringDuplicationErrorTest) {
    setenv("CELIX_RSA_IMPORTED_SERVICE_RANKING_OFFSETS", "configType1=10,configType2=20", 1);
    celix_ei_expect_celix_utils_strdup((void*)topologyManager_create, 1, nullptr);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
    unsetenv("CELIX_RSA_IMPORTED_SERVICE_RANKING_OFFSETS");
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, AddingImportedServiceRankingOffsetErrorTest) {
    setenv("CELIX_RSA_IMPORTED_SERVICE_RANKING_OFFSETS", "configType1=10,configType2=20", 1);
    celix_ei_expect_celix_stringHashMap_putLong((void*)topologyManager_create, 1, ENOMEM);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(ENOMEM, status);
    unsetenv("CELIX_RSA_IMPORTED_SERVICE_RANKING_OFFSETS");
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingImportedServiceMapErrorTest) {
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)topologyManager_create, 0, nullptr);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingMutexErrorTest) {
    celix_ei_expect_celixThreadMutex_create((void*)topologyManager_create, 0, CELIX_ENOMEM);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingRsaMapErrorTest) {
    celix_ei_expect_celix_longHashMap_create((void*)topologyManager_create, 0, nullptr);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingDynamicIPEndpointMapErrorTest) {
    celix_ei_expect_celix_longHashMap_create((void*)topologyManager_create, 0, nullptr, 2);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingRsaIfNameMapErrorTest) {
    celix_ei_expect_celix_longHashMap_create((void*)topologyManager_create, 0, nullptr, 3);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingScopeErrorTest) {
    celix_ei_expect_calloc((void*)scope_scopeCreate, 0, nullptr);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(TopologyManagerCreatingErrorInjectionTestSuite, CreatingExportedServiceMapErrorTest) {
    celix_ei_expect_celix_longHashMap_create((void*)topologyManager_create, 0, nullptr, 4);
    void *scope{};
    topology_manager_t *tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

class TopologyManagerErrorInjectionTestSuite : public TopologyManagerTestSuiteBaseClass {
public:
    TopologyManagerErrorInjectionTestSuite() = default;

    ~TopologyManagerErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_longHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_set(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_add(nullptr, 0, 0);
        celix_ei_expect_celix_filter_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_getAsStringArrayList(nullptr, 0, 0);
        celix_ei_expect_celix_properties_setLong(nullptr, 0, 0);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
    }

    void TestExportServiceFailure(void (*errorInject)(void)) {
        errorInject();
        TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef,
            void* exportedSvc, service_reference_pt eplSvcRef CELIX_UNUSED, void* eplSvc CELIX_UNUSED, celix_bundle_context_t* ctx CELIX_UNUSED) {
            //first add the exported service, then the rsa
            auto status = topologyManager_addExportedService(tm, exportedSvcRef, exportedSvc);
            EXPECT_EQ(CELIX_SUCCESS, status);
            status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
            EXPECT_EQ(CELIX_SUCCESS, status);
            status = topologyManager_endpointListenerAdded(tm, eplSvcRef, eplSvc);
            EXPECT_EQ(CELIX_SUCCESS, status);

            status = topologyManager_endpointListenerRemoved(tm, eplSvcRef, eplSvc);
            EXPECT_EQ(CELIX_SUCCESS, status);
            status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
            EXPECT_EQ(CELIX_SUCCESS, status);
            status = topologyManager_removeExportedService(tm, exportedSvcRef, exportedSvc);
            EXPECT_EQ(CELIX_SUCCESS, status);
            }, true, nullptr,
            [](void *handle CELIX_UNUSED, endpoint_description_t *endpoint CELIX_UNUSED, char *matchedFilter CELIX_UNUSED) -> celix_status_t {
            ADD_FAILURE() << "Should not be called";
            return CELIX_SUCCESS;
            });
    }
};

TEST_F(TopologyManagerErrorInjectionTestSuite, AllocingRsaSvcEntryMemoryErrorTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef CELIX_UNUSED,
            void* exportedSvc CELIX_UNUSED, service_reference_pt eplSvcRef CELIX_UNUSED, void* eplSvc CELIX_UNUSED, celix_bundle_context_t* ctx CELIX_UNUSED) {
        celix_ei_expect_calloc((void*)&topologyManager_rsaAdded, 0, nullptr);
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_ENOMEM, status);

        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, PutingRsaSvcEntryToMapErrorTest) {
    TestExportService([](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef CELIX_UNUSED,
            void* exportedSvc CELIX_UNUSED, service_reference_pt eplSvcRef CELIX_UNUSED, void* eplSvc CELIX_UNUSED, celix_bundle_context_t* ctx CELIX_UNUSED) {
        celix_ei_expect_celix_longHashMap_put((void*)&topologyManager_rsaAdded, 0, CELIX_ENOMEM);
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_ENOMEM, status);

        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, PutingExportedRegistrationToMapErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_longHashMap_put((void*)&topologyManager_rsaAdded, 0, CELIX_ENOMEM, 2);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CreatingDynamicIpEndpointMapErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_longHashMap_create((void*)&topologyManager_rsaAdded, 2, nullptr);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, PutingDynamicIpEndpointToRsaMapErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_longHashMap_put((void*)&topologyManager_rsaAdded, 2, CELIX_ENOMEM);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CreateDynamicIpEndpointListErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_arrayList_create((void*)&topologyManager_rsaAdded, 1, nullptr);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, PutingDynamicIpEndpointListToMapErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_longHashMap_put((void*)&topologyManager_rsaAdded, 1, CELIX_ENOMEM);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CreatingRsaIfNameListErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_arrayList_create((void*)&topologyManager_rsaAdded, 3, nullptr);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CopyRsaIfNamesStringErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_utils_strdup((void*)&topologyManager_rsaAdded, 3, nullptr, 2);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CopyRsaIfNameErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_utils_strdup((void*)&topologyManager_rsaAdded, 3, nullptr, 3);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, AddingIfNameToListErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_arrayList_add((void*)&topologyManager_rsaAdded, 3, CELIX_ENOMEM);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, PutingIfNameListToMapErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_longHashMap_put((void*)&topologyManager_rsaAdded, 3, CELIX_ENOMEM);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CreatingDynamicIpEndpointErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_properties_copy(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, SettingDynamicIpEndpointIfNamePropertyErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_properties_set((void*)&topologyManager_rsaAdded, 3, CELIX_ENOMEM);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, SettingDynamicIpEndpointEpUuidPropertyErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_properties_set((void*)&topologyManager_rsaAdded, 3, CELIX_ENOMEM, 2);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, SettingDynamicIpEndpointIPAddressPropertyErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_properties_set((void*)&topologyManager_rsaAdded, 3, CELIX_ENOMEM, 3);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, AddDynamicIpEndpointToListErrorTest) {
    TestExportServiceFailure([]() {
        celix_ei_expect_celix_arrayList_add((void*)&topologyManager_rsaAdded, 2, CELIX_ENOMEM, 2);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CreatingFilterErrorWhenImportScopeChangedTest) {
    celix_ei_expect_celix_filter_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    auto status = tm_addImportScope(tms.get(), (char*)"(service.imported.configs=tm_test_config_type)");
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(TopologyManagerErrorInjectionTestSuite, AllocingMemoryForImportedServiceEntryErrorTest) {
    TestImportService([](topology_manager_t* tm, service_reference_pt, void*, endpoint_description_t *importEndpoint) {
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_ENOMEM, status);

        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, GettingServiceImportedConfigsErrorTest) {
    TestImportService([](topology_manager_t* tm, service_reference_pt, void*, endpoint_description_t *importEndpoint) {
        celix_ei_expect_celix_properties_getAsStringArrayList(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(ENOMEM, status);

        celix_properties_unset(importEndpoint->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS);
        status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_ENOMEM, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, CopyImportedServicePropertiesErrorTest) {
    TestImportService([](topology_manager_t* tm, service_reference_pt, void*, endpoint_description_t *importEndpoint) {
        celix_ei_expect_celix_properties_copy(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);//called in endpointDescription_clone
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(ENOMEM, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, SettingImportedServiceRankingErrorTest) {
    setenv("CELIX_RSA_IMPORTED_SERVICE_RANKING_OFFSETS", "tm_test_config_type=10", 1);
    tm.reset();
    void* scope = nullptr;
    topology_manager_t* tmPtr{};
    auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
    EXPECT_EQ(status, CELIX_SUCCESS);
    tm = std::shared_ptr<topology_manager_t>{tmPtr, [](auto t) {topologyManager_destroy(t);}};

    TestImportService([](topology_manager_t* tm, service_reference_pt, void*, endpoint_description_t *importEndpoint) {
        celix_ei_expect_celix_properties_setLong(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);//called in endpointDescription_clone
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(ENOMEM, status);
    });

    unsetenv("CELIX_RSA_IMPORTED_SERVICE_RANKING_OFFSETS");
}


TEST_F(TopologyManagerErrorInjectionTestSuite, CreatingImportsMapFailureTest) {
    TestImportService([](topology_manager_t* tm, service_reference_pt, void*, endpoint_description_t *importEndpoint) {
        celix_ei_expect_celix_longHashMap_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(ENOMEM, status);

        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, AddingImportedServiceToMapFailureTest) {
    TestImportService([](topology_manager_t* tm, service_reference_pt, void*, endpoint_description_t *importEndpoint) {
        celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(ENOMEM, status);

        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, AddingImportedRegistrationToMapFailureTest) {
    TestImportService([this](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, endpoint_description_t *importEndpoint) {
        auto status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_ei_expect_celix_longHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);
        status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_service_filter_options_t opts{};
        opts.filter = (char *)"(service.imported.configs=tm_test_config_type)";
        auto svcId = celix_bundleContext_findServiceWithOptions(ctx.get(), &opts);
        EXPECT_EQ(svcId, -1);

        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}

TEST_F(TopologyManagerErrorInjectionTestSuite, AddingImportedRegistrationToMapFailureWhenAddRsaTest) {
    TestImportService([this](topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, endpoint_description_t *importEndpoint) {
        auto status = topologyManager_addImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_ei_expect_celix_longHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM, 2);
        status = topologyManager_rsaAdded(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_service_filter_options_t opts{};
        opts.filter = (char *)"(service.imported.configs=tm_test_config_type)";
        auto svcId = celix_bundleContext_findServiceWithOptions(ctx.get(), &opts);
        EXPECT_EQ(svcId, -1);

        status = topologyManager_rsaRemoved(tm, rsaSvcRef, rsaSvc);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = topologyManager_removeImportedService(tm, importEndpoint, nullptr);
        EXPECT_EQ(CELIX_SUCCESS, status);
    });
}