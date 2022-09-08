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
#include <rsa_shm_constants.h>
#include <remote_service_admin.h>
#include <calculator_service.h>
#include <remote_constants.h>
#include <celix_api.h>
#include <uuid/uuid.h>
#include <gtest/gtest.h>

static void rsaShmTestSuite_setRsaService(void *handle, void *svc);

class RsaShmTestSuite : public ::testing::Test {
public:
    RsaShmTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME, "onFirstInit");
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".rsa_shm_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        long bundleId{-1};
        bundleId = celix_bundleContext_installBundle(ctx.get(), RSA_SHM_BUNDLE, true);
        EXPECT_TRUE(bundleId >= 0);
        bundleId = celix_bundleContext_installBundle(ctx.get(), RSA_RPC_BUNDLE, true);
        EXPECT_TRUE(bundleId >= 0);
    }


    void SetUp() {
        adminSvcTrkId = celix_bundleContext_trackServiceAsync(ctx.get(), OSGI_RSA_REMOTE_SERVICE_ADMIN, this, rsaShmTestSuite_setRsaService);
        celix_bundleContext_waitForAsyncTracker(ctx.get(), adminSvcTrkId);
    }

    void TearDown() {
        celix_bundleContext_stopTracker(ctx.get(), adminSvcTrkId);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<remote_service_admin_service_t> admin{};
    long adminSvcTrkId{-1};
    std::shared_ptr<celix_array_list_t> exportRegs{};
};

static void rsaShmTestSuite_setRsaService(void *handle, void *svc) {
    auto *tstSuite = static_cast<RsaShmTestSuite *>(handle);
    auto *rsaSvc = static_cast<remote_service_admin_service_t *>(svc);
    if (tstSuite->admin.get() == nullptr) {
        tstSuite->admin = std::shared_ptr<remote_service_admin_service_t>{rsaSvc, [](auto*){/*nop*/}};
    }
    return ;
}

static void rsaShmTestSuite_addProviderSvc(void *handle, void*svc, const celix_properties_t *props) {
    (void)svc;
    auto *tstSuite = static_cast<RsaShmTestSuite *>(handle);
    celix_array_list_t *exportRegs = nullptr;
    char *serviceId = const_cast<char *>(celix_properties_get(props, OSGI_FRAMEWORK_SERVICE_ID, nullptr));
    auto status = tstSuite->admin->exportService(tstSuite->admin->admin, serviceId,
            const_cast<celix_properties_t *>(props), &exportRegs);
    EXPECT_EQ(CELIX_SUCCESS, status);
    tstSuite->exportRegs = std::shared_ptr<celix_array_list_t>{exportRegs, [](auto*){/*nop*/}};
    tstSuite->exportRegs = std::shared_ptr<celix_array_list_t>{exportRegs, [](auto *r){celix_arrayList_destroy(r);}};
    return;
}

static void rsaShmTestSuite_removeProviderSvc(void *handle, void*svc, const celix_properties_t *props) {
    (void)svc;
    (void)props;
    auto *tstSuite = static_cast<RsaShmTestSuite *>(handle);
    int size = celix_arrayList_size(tstSuite->exportRegs.get());
    for (int i = 0; i < size; ++i) {
        auto *reg = static_cast<export_registration_t *>(celix_arrayList_get(tstSuite->exportRegs.get(), i));
        auto status = tstSuite->admin->exportRegistration_close(tstSuite->admin->admin, reg);
        EXPECT_EQ(CELIX_SUCCESS, status);
    }
    return;
}

TEST_F(RsaShmTestSuite, ExportService) {
    long bundleId{-1};
    bundleId = celix_bundleContext_installBundle(ctx.get(), PROVIDER_BUNDLE, true);
    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = CALCULATOR_SERVICE;
    opts.callbackHandle = this;
    opts.addWithProperties = rsaShmTestSuite_addProviderSvc;
    opts.removeWithProperties = rsaShmTestSuite_removeProviderSvc;
    EXPECT_TRUE(bundleId >= 0);
    long providerSvcTrkId = celix_bundleContext_trackServicesWithOptionsAsync(ctx.get(), &opts);
    celix_bundleContext_waitForAsyncTracker(ctx.get(), providerSvcTrkId);
    int size = celix_arrayList_size(exportRegs.get());
    for (int i = 0; i < size; ++i) {
        auto *reg = static_cast<export_registration_t *>(celix_arrayList_get(exportRegs.get(), i));
        export_reference_t *ref = nullptr;
        auto status = admin->exportRegistration_getException(reg);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = admin->exportRegistration_getExportReference(reg, &ref);
        EXPECT_EQ(CELIX_SUCCESS, status);
        endpoint_description_t *endpoint = nullptr;
        status = admin->exportReference_getExportedEndpoint(ref, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, status);
        service_reference_pt service = nullptr;
        status = admin->exportReference_getExportedService(ref, &service);
        EXPECT_EQ(CELIX_SUCCESS, status);
        free(ref);
    }
    celix_bundleContext_stopTrackerAsync(ctx.get(), providerSvcTrkId, nullptr, nullptr);
    celix_bundleContext_waitForAsyncStopTracker(ctx.get(), providerSvcTrkId);
}

TEST_F(RsaShmTestSuite, ImportService) {
    endpoint_description_t endpointDesc{};
    endpointDesc.properties = celix_properties_create();
    EXPECT_TRUE(endpointDesc.properties != nullptr);
    uuid_t endpoint_uid;
    uuid_generate(endpoint_uid);
    char endpoint_uuid[37];
    uuid_unparse_lower(endpoint_uid, endpoint_uuid);
    const char *uuid{nullptr};
    uuid = celix_bundleContext_getProperty(ctx.get(), OSGI_FRAMEWORK_FRAMEWORK_UUID, nullptr);
    celix_properties_set(endpointDesc.properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    celix_properties_set(endpointDesc.properties, OSGI_FRAMEWORK_OBJECTCLASS, CALCULATOR_SERVICE);
    celix_properties_set(endpointDesc.properties, OSGI_RSA_ENDPOINT_ID, endpoint_uuid);
    celix_properties_set(endpointDesc.properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(endpointDesc.properties, RSA_SHM_SERVER_NAME_KEY, "shm-server-tst");
    char importedConfigs[1024];
    snprintf(importedConfigs, 1024, "%s,%s", RSA_SHM_CONFIGURATION_TYPE, RSA_SHM_RPC_TYPE_DEFAULT);
    celix_properties_set(endpointDesc.properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, importedConfigs);
    endpointDesc.frameworkUUID = (char*)celix_properties_get(endpointDesc.properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
    endpointDesc.serviceId = 0;
    endpointDesc.id = (char*)celix_properties_get(endpointDesc.properties, OSGI_RSA_ENDPOINT_ID, NULL);
    endpointDesc.service = strdup(CALCULATOR_SERVICE);

    import_registration_t *registration = nullptr;
    auto status = admin->importService(admin->admin, &endpointDesc, &registration);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, registration);
    free(endpointDesc.service);
    celix_properties_destroy(endpointDesc.properties);

    status = admin->importRegistration_close(admin->admin, registration);
    EXPECT_EQ(CELIX_SUCCESS, status);
}
