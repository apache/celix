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
#include "celix_framework_factory.h"
#include "celix_bundle_context.h"
#include "celix_compiler.h"
#include "celix_constants.h"
#include "calculator_service.h"

extern "C" {

#include "remote_service_admin.h"
#include "calculator_service.h"

#define TST_CONFIGURATION_TYPE "org.amdatu.remote.admin.http"

    static celix_framework_t *framework = NULL;
    static celix_bundle_context_t *context = NULL;

    long calcSvcId = -1L;

    static void setupFm(void) {
        celix_properties_t *fwProperties = celix_properties_load("config.properties");
        ASSERT_TRUE(fwProperties != NULL);
        framework = celix_frameworkFactory_createFramework(fwProperties);
        ASSERT_TRUE(framework != NULL);
        context = celix_framework_getFrameworkContext(framework);
        ASSERT_TRUE(context != NULL);


        calcSvcId = celix_bundleContext_findService(context, CALCULATOR_SERVICE);
        ASSERT_TRUE(calcSvcId >= 0L);
    }

    static void teardownFm(void) {
        celix_frameworkFactory_destroyFramework(framework);
    }

    static void testServicesCallback(void *handle CELIX_UNUSED, void *svc) {
        auto* rsa = static_cast<remote_service_admin_service_t*>(svc);
        celix_array_list_t *exported = celix_arrayList_create();
        celix_array_list_t *imported = celix_arrayList_create();

        int rc = rsa->getExportedServices(rsa->admin, &exported);
        ASSERT_EQ(CELIX_SUCCESS, rc);
        ASSERT_EQ(0, celix_arrayList_size(exported));

        rc = rsa->getImportedEndpoints(rsa->admin, &imported);
        ASSERT_EQ(CELIX_SUCCESS, rc);
        ASSERT_EQ(0, celix_arrayList_size(imported));

        celix_arrayList_destroy(imported);
        celix_arrayList_destroy(exported);
    }

    static void testServices(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = CELIX_RSA_REMOTE_SERVICE_ADMIN;
        opts.use = testServicesCallback;
        opts.waitTimeoutInSeconds = 0.25;
        bool called = celix_bundleContext_useServiceWithOptions(context, &opts);
        ASSERT_TRUE(called);
    }

    static void testExportServiceCallback(void *handle CELIX_UNUSED, void *svc) {
        auto* rsa = static_cast<remote_service_admin_service_t*>(svc);

        char strSvcId[64];
        snprintf(strSvcId, 64, "%li", calcSvcId);

        celix_array_list_t *svcRegistration = NULL;
        int rc = rsa->exportService(rsa->admin, strSvcId, NULL, &svcRegistration);
        ASSERT_EQ(CELIX_SUCCESS, rc);

        ASSERT_EQ(1, celix_arrayList_size(svcRegistration));

        rc = rsa->exportRegistration_close(rsa->admin,(export_registration_t *)(celix_arrayList_get(svcRegistration,0)));
        ASSERT_EQ(CELIX_SUCCESS, rc);
        celix_arrayList_destroy(svcRegistration);
    }


    static void testExportService(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = CELIX_RSA_REMOTE_SERVICE_ADMIN;
        opts.use = testExportServiceCallback;
        opts.waitTimeoutInSeconds = 0.25;
        bool called = celix_bundleContext_useServiceWithOptions(context, &opts);
        ASSERT_TRUE(called);
    }

    static void testImportServiceCallback(void *handle CELIX_UNUSED, void *svc) {
        thread_local bool init = true;
        thread_local endpoint_description_t *endpoint = nullptr;
        if (init) {
            auto *rsa = static_cast<remote_service_admin_service_t *>(svc);
            celix_properties_t *props = celix_properties_create();
            celix_properties_set(props, CELIX_RSA_ENDPOINT_SERVICE_ID, "42");
            celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "eec5404d-51d0-47ef-8d86-c825a8beda42");
            celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, "eec5404d-51d0-47ef-8d86-c825a8beda42-42");
            celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, TST_CONFIGURATION_TYPE);
            celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, "org.apache.celix.Example");

            int rc = endpointDescription_create(props, &endpoint);
            ASSERT_EQ(CELIX_SUCCESS, rc);

            import_registration_t* reg = nullptr;
            rc = rsa->importService(rsa->admin, endpoint, &reg);
            ASSERT_EQ(CELIX_SUCCESS, rc);
            ASSERT_TRUE(reg != nullptr);

            init = false;
        } else {
            int rc = endpointDescription_destroy(endpoint);
            ASSERT_EQ(CELIX_SUCCESS, rc);
        }
    }

    static void testImportService(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = CELIX_RSA_REMOTE_SERVICE_ADMIN;
        opts.use = testImportServiceCallback;
        opts.waitTimeoutInSeconds = 0.25;

        //first call -> init
        bool called = celix_bundleContext_useServiceWithOptions(context, &opts);
        ASSERT_TRUE(called);

        celix_framework_waitForEmptyEventQueue(celix_bundleContext_getFramework(context));
        long svcId = celix_bundleContext_findService(context, "org.apache.celix.Example");
        EXPECT_GE(svcId, 0);

        //second call -> deinit
        called = celix_bundleContext_useServiceWithOptions(context, &opts);
        ASSERT_TRUE(called);
    }

    static void testBundles(void) {
        celix_array_list_t* bundles = NULL;

        int rc = bundleContext_getBundles(context, &bundles);
        ASSERT_EQ(0, rc);
        ASSERT_EQ(3, celix_arrayList_size(bundles)); //framework, rsa_dfi & calc

        /*
        int size = arrayList_size(bundles);
        int i;
        for (i = 0; i < size; i += 1) {
            celix_bundle_t *bundle = NULL;
            module_pt module = NULL;
            char *name = NULL;

            bundle = (celix_bundle_t *) arrayList_get(bundles, i);
            bundle_getCurrentModule(bundle, &module);
            module_getSymbolicName(module, &name);
            printf("got bundle with symbolic name '%s'", name);
        }*/

        celix_arrayList_destroy(bundles);
    }

}

class RsaDfiTests : public ::testing::Test {
public:
    RsaDfiTests() {
        setupFm();
    }
    ~RsaDfiTests() override {
        teardownFm();
    }

};


TEST_F(RsaDfiTests, InfoTest) {
    testServices();
}

TEST_F(RsaDfiTests, ExportService) {
    testExportService();
}

TEST_F(RsaDfiTests, ImportService) {
    testImportService();
}

TEST_F(RsaDfiTests, TestBundles) {
    testBundles();
}
