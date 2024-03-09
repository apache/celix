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



