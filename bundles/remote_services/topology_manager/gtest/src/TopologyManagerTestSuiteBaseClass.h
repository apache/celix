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

#ifndef CELIX_TOPOLOGYMANAGERTESTSUITEBASECLASS_H
#define CELIX_TOPOLOGYMANAGERTESTSUITEBASECLASS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <cstdlib>
#include <gtest/gtest.h>
#include "celix_errno.h"
#include "celix_constants.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_log_helper.h"

extern "C" {
#include "remote_constants.h"
#include "remote_service_admin.h"
#include "topology_manager.h"

struct import_registration {
    endpoint_description_t *endpoint;
};

struct export_reference {
    endpoint_description_t *endpoint;
    service_reference_pt reference;
};
struct export_registration {
    export_reference_t exportReference;
};

struct remote_service_admin {
    celix_bundle_context_t* ctx;
    bool dynamicIpSupport;
};
}

#define TEST_RSA_PORT "8080"

class TopologyManagerTestSuiteBaseClass : public ::testing::Test {
public:
    TopologyManagerTestSuiteBaseClass() {
        auto config = celix_properties_create();
        celix_properties_set(config, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(config, CELIX_FRAMEWORK_CACHE_DIR, ".tm_unit_test_cache");
        fw = std::shared_ptr<celix_framework_t>{celix_frameworkFactory_createFramework(config), [](auto f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(fw.get()), [](auto){/*nop*/}};
        logHelper = std::shared_ptr<celix_log_helper_t>{celix_logHelper_create(ctx.get(), "tm_unit_test"), [](auto l) {celix_logHelper_destroy(l);}};

        void* scope = nullptr;
        topology_manager_t* tmPtr{};
        auto status = topologyManager_create(ctx.get(), logHelper.get(), &tmPtr, &scope);
        EXPECT_EQ(status, CELIX_SUCCESS);
        tm = std::shared_ptr<topology_manager_t>{tmPtr, [](auto t) {topologyManager_destroy(t);}};
    }

    ~TopologyManagerTestSuiteBaseClass() = default;

    void TestExportService(void (*testBody)(topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, service_reference_pt exportedSvcRef,
                                            void* exportedSvc, service_reference_pt eplSvcRef, void* eplSvc, celix_bundle_context_t* ctx), bool testDynamicIp = false,
                           celix_status_t (*exportService)(remote_service_admin_t* admin, char* serviceId, celix_properties_t* properties, celix_array_list_t** registrations) = nullptr,
                           celix_status_t (*endpointAdded)(void *handle, endpoint_description_t *endpoint, char *matchedFilter) = nullptr) {
        if (testDynamicIp) {
            setenv("CELIX_RSA_INTERFACES_OF_PORT_" TEST_RSA_PORT, "eth0", true);
        }
        remote_service_admin_t rsa{};
        rsa.ctx = ctx.get();
        rsa.dynamicIpSupport = testDynamicIp;
        remote_service_admin_service_t rsaSvc{};
        rsaSvc.admin = &rsa;
        rsaSvc.exportService = exportService != nullptr ? exportService : [](remote_service_admin_t* admin, char* serviceId,
                                                                             celix_properties_t* properties, celix_array_list_t** registrations) -> celix_status_t {
            (void)properties;
            celix_array_list_t* references{};
            service_reference_pt reference{};
            char filter[256];
            snprintf(filter, 256, "(%s=%s)", (char *) CELIX_FRAMEWORK_SERVICE_ID, serviceId);
            auto status = bundleContext_getServiceReferences(admin->ctx, nullptr, filter, &references);
            EXPECT_EQ(CELIX_SUCCESS, status);
            reference = (service_reference_pt)celix_arrayList_get(references, 0);
            EXPECT_TRUE(reference != nullptr);
            auto exportReg = (export_registration_t*)malloc(sizeof(export_registration_t));
            auto endpointProps = celix_properties_create();
            unsigned int size = 0;
            char **keys;
            serviceReference_getPropertyKeys(reference, &keys, &size);
            for (unsigned int i = 0; i < size; i++) {
                char *key = keys[i];
                const char *value{};
                if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS
                    && strcmp(key, (char*) CELIX_RSA_SERVICE_EXPORTED_INTERFACES) != 0
                    && strcmp(key, (char*) CELIX_RSA_SERVICE_EXPORTED_CONFIGS) != 0) {
                    celix_properties_set(endpointProps, key, value);
                }
            }
            const char *fwUuid = celix_bundleContext_getProperty(admin->ctx, CELIX_FRAMEWORK_UUID, nullptr);
            celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
            celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_SERVICE_ID, serviceId);
            celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_ID, "319bddfa-0252-4654-a3bd-298354d30207");
            celix_properties_set(endpointProps, CELIX_RSA_SERVICE_IMPORTED, "true");
            celix_properties_set(endpointProps, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "tm_test_config_type");
            if (admin->dynamicIpSupport) {
                celix_properties_set(endpointProps, CELIX_RSA_PORT, TEST_RSA_PORT);
            }
            status = endpointDescription_create(endpointProps, &exportReg->exportReference.endpoint);
            EXPECT_EQ(CELIX_SUCCESS, status);
            exportReg->exportReference.reference = reference;
            *registrations = celix_arrayList_create();
            celix_arrayList_add(*registrations, exportReg);

            bundleContext_ungetServiceReference(admin->ctx, reference);
            free(keys);
            celix_arrayList_destroy(references);
            return CELIX_SUCCESS;
        };
        rsaSvc.exportRegistration_close = [](remote_service_admin_t* admin, export_registration_t* registration) -> celix_status_t {
            (void)admin;
            endpointDescription_destroy(registration->exportReference.endpoint);
            free(registration);
            return CELIX_SUCCESS;
        };
        rsaSvc.exportRegistration_getExportReference = [](export_registration_t* registration, export_reference_t** reference) -> celix_status_t {
            *reference = (export_reference_t*)calloc(1, sizeof(export_reference_t));
            (*reference)->endpoint = registration->exportReference.endpoint;
            (*reference)->reference = registration->exportReference.reference;
            return CELIX_SUCCESS;
        };
        rsaSvc.exportReference_getExportedEndpoint = [](export_reference_t* reference, endpoint_description_t** endpoint) -> celix_status_t {
            *endpoint = reference->endpoint;
            return CELIX_SUCCESS;
        };
        auto rsaProps = celix_properties_create();
        if (testDynamicIp) {
            celix_properties_setBool(rsaProps, CELIX_RSA_DYNAMIC_IP_SUPPORT, true);
        }
        auto rsaId = celix_bundleContext_registerService(ctx.get(), &rsaSvc, CELIX_RSA_REMOTE_SERVICE_ADMIN, rsaProps);
        EXPECT_TRUE(rsaId > 0);

        struct TmTestService {
            void* handle;
        } exportedService{};
        auto exportedSvcProps = celix_properties_create();
        celix_properties_set(exportedSvcProps, "service.exported.interfaces", "*");
        auto exportedSvcId = celix_bundleContext_registerService(ctx.get(), &exportedService, "tmTestService", exportedSvcProps);
        EXPECT_TRUE(exportedSvcId > 0);

        endpoint_listener_t epl{};
        epl.handle = nullptr;
        epl.endpointAdded = endpointAdded != nullptr ? endpointAdded : [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
            (void)handle;
            (void)endpoint;
            (void)matchedFilter;
            return CELIX_SUCCESS;
        };
        epl.endpointRemoved = [](void *handle, endpoint_description_t *endpoint, char *matchedFilter) -> celix_status_t {
            (void)handle;
            (void)endpoint;
            (void)matchedFilter;
            return CELIX_SUCCESS;
        };
        const char *fwUuid = celix_bundleContext_getProperty(ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
        char scope[256] = {0};
        (void)snprintf(scope, sizeof(scope), "(&(%s=*)(%s=%s))", CELIX_FRAMEWORK_SERVICE_NAME,
                       CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
        auto elpProps = celix_properties_create();
        celix_properties_set(elpProps, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);
        if (testDynamicIp) {
            celix_properties_setBool(elpProps, CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT, true);
        }
        auto eplId = celix_bundleContext_registerService(ctx.get(), &epl, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, elpProps);
        EXPECT_TRUE(eplId > 0);

        service_reference_pt rsaSvcRef{};
        auto status = bundleContext_getServiceReference(ctx.get(), CELIX_RSA_REMOTE_SERVICE_ADMIN, &rsaSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);
        service_reference_pt exportedSvcRef{};
        status = bundleContext_getServiceReference(ctx.get(), "tmTestService", &exportedSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);
        service_reference_pt eplSvcRef{};
        status = bundleContext_getServiceReference(ctx.get(), CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, &eplSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);

        testBody(tm.get(), rsaSvcRef, &rsaSvc, exportedSvcRef, &exportedService, eplSvcRef, &epl, ctx.get());

        status = bundleContext_ungetServiceReference(ctx.get(), exportedSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = bundleContext_ungetServiceReference(ctx.get(), eplSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);
        status = bundleContext_ungetServiceReference(ctx.get(), rsaSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);

        celix_bundleContext_unregisterService(ctx.get(), eplId);
        celix_bundleContext_unregisterService(ctx.get(), exportedSvcId);
        celix_bundleContext_unregisterService(ctx.get(), rsaId);

        if (testDynamicIp) {
            unsetenv("CELIX_RSA_INTERFACES_OF_PORT_" TEST_RSA_PORT);
        }
    }

    void TestImportService(void (*testBody)(topology_manager_t* tm, service_reference_pt rsaSvcRef, void* rsaSvc, endpoint_description_t *importEndpoint)) {
        remote_service_admin_t rsa{};
        rsa.ctx = ctx.get();
        remote_service_admin_service_t rsaSvc{};
        rsaSvc.admin = &rsa;
        rsaSvc.importService = [](remote_service_admin_t* admin, endpoint_description_t* endpoint, import_registration_t** registration) -> celix_status_t {
            (void)admin;
            auto importReg = (import_registration_t*)calloc(1, sizeof(import_registration_t));
            importReg->endpoint = endpoint;
            *registration = importReg;
            return CELIX_SUCCESS;
        };
        rsaSvc.importRegistration_close = [](remote_service_admin_t* admin, import_registration_t* registration) -> celix_status_t {
            (void)admin;
            free(registration);
            return CELIX_SUCCESS;
        };
        auto rsaId = celix_bundleContext_registerService(ctx.get(), &rsaSvc, CELIX_RSA_REMOTE_SERVICE_ADMIN, nullptr);
        EXPECT_TRUE(rsaId > 0);

        auto endpointProps = celix_properties_create();
        celix_properties_set(endpointProps, CELIX_FRAMEWORK_SERVICE_NAME, "tmTestService");
        celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "1fb0bb2a-95ad-4cf9-8e79-072ec8bd4a85");
        celix_properties_setLong(endpointProps, CELIX_RSA_ENDPOINT_SERVICE_ID, 100);
        celix_properties_set(endpointProps, CELIX_RSA_ENDPOINT_ID, "319bddfa-0252-4654-a3bd-298354d30207");
        celix_properties_set(endpointProps, CELIX_RSA_SERVICE_IMPORTED, "true");
        celix_properties_set(endpointProps, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "tm_test_config_type");
        endpoint_description_t* endpoint{};
        auto status = endpointDescription_create(endpointProps, &endpoint);
        EXPECT_EQ(CELIX_SUCCESS, status);

        service_reference_pt rsaSvcRef{};
        status = bundleContext_getServiceReference(ctx.get(), CELIX_RSA_REMOTE_SERVICE_ADMIN, &rsaSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);

        testBody(tm.get(), rsaSvcRef, &rsaSvc, endpoint);

        status = bundleContext_ungetServiceReference(ctx.get(), rsaSvcRef);
        EXPECT_EQ(CELIX_SUCCESS, status);

        endpointDescription_destroy(endpoint);
        celix_bundleContext_unregisterService(ctx.get(), rsaId);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    std::shared_ptr<topology_manager_t> tm{};
};

#ifdef __cplusplus
}
#endif

#endif //CELIX_TOPOLOGYMANAGERTESTSUITEBASECLASS_H
