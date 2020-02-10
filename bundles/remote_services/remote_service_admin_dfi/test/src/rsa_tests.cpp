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

#include <remote_constants.h>
#include "celix_api.h"
#include "calculator_service.h"


#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

extern "C" {

#include "remote_service_admin.h"
#include "calculator_service.h"

#define TST_CONFIGURATION_TYPE "org.amdatu.remote.admin.http"

    static celix_framework_t *framework = NULL;
    static celix_bundle_context_t *context = NULL;

    long calcSvcId = -1L;

    static void setupFm(void) {
        celix_properties_t *fwProperties = celix_properties_load("config.properties");
        CHECK_TRUE(fwProperties != NULL);
        framework = celix_frameworkFactory_createFramework(fwProperties);
        CHECK_TRUE(framework != NULL);
        context = celix_framework_getFrameworkContext(framework);
        CHECK_TRUE(context != NULL);


        calcSvcId = celix_bundleContext_findService(context, CALCULATOR_SERVICE);
        CHECK_TRUE(calcSvcId >= 0L);
    }

    static void teardownFm(void) {
        celix_frameworkFactory_destroyFramework(framework);
    }

    static void testServicesCallback(void *handle __attribute__((unused)), void *svc) {
        auto* rsa = static_cast<remote_service_admin_service_t*>(svc);
        celix_array_list_t *exported = celix_arrayList_create();
        celix_array_list_t *imported = celix_arrayList_create();

        int rc = rsa->getExportedServices(rsa->admin, &exported);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK_EQUAL(0, celix_arrayList_size(exported));

        rc = rsa->getImportedEndpoints(rsa->admin, &imported);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK_EQUAL(0, celix_arrayList_size(imported));

        celix_arrayList_destroy(imported);
        celix_arrayList_destroy(exported);
    }

    static void testServices(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = OSGI_RSA_REMOTE_SERVICE_ADMIN;
        opts.use = testServicesCallback;
        opts.filter.ignoreServiceLanguage = true;
        opts.waitTimeoutInSeconds = 0.25;
        bool called = celix_bundleContext_useServiceWithOptions(context, &opts);
        CHECK_TRUE(called);
    }

    static void testExportServiceCallback(void *handle __attribute__((unused)), void *svc) {
        auto* rsa = static_cast<remote_service_admin_service_t*>(svc);

        char strSvcId[64];
        snprintf(strSvcId, 64, "%li", calcSvcId);

        celix_array_list_t *svcRegistration = NULL;
        int rc = rsa->exportService(rsa->admin, strSvcId, NULL, &svcRegistration);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        CHECK_EQUAL(1, celix_arrayList_size(svcRegistration));

        rc = rsa->exportRegistration_close(rsa->admin,(export_registration_t *)(arrayList_get(svcRegistration,0)));
        CHECK_EQUAL(CELIX_SUCCESS, rc);
    }


    static void testExportService(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = OSGI_RSA_REMOTE_SERVICE_ADMIN;
        opts.use = testExportServiceCallback;
        opts.filter.ignoreServiceLanguage = true;
        opts.waitTimeoutInSeconds = 0.25;
        bool called = celix_bundleContext_useServiceWithOptions(context, &opts);
        CHECK_TRUE(called);
    }

    static void testImportServiceCallback(void *handle __attribute__((unused)), void *svc) {
        auto *rsa = static_cast<remote_service_admin_service_t *>(svc);

        int rc = 0;
        import_registration_t *reg = NULL;
        endpoint_description_t *endpoint = NULL;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, OSGI_RSA_ENDPOINT_SERVICE_ID, "42");
        celix_properties_set(props, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, "eec5404d-51d0-47ef-8d86-c825a8beda42");
        celix_properties_set(props, OSGI_RSA_ENDPOINT_ID, "eec5404d-51d0-47ef-8d86-c825a8beda42-42");
        celix_properties_set(props, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, TST_CONFIGURATION_TYPE);
        celix_properties_set(props, OSGI_FRAMEWORK_OBJECTCLASS, "org.apache.celix.Example");

        rc = endpointDescription_create(props, &endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = rsa->importService(rsa->admin, endpoint, &reg);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(reg != NULL);

        service_reference_pt ref = NULL;
        rc = bundleContext_getServiceReference(context, (char *)"org.apache.celix.Example", &ref);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(ref != NULL);

        rc = bundleContext_ungetServiceReference(context, ref);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = endpointDescription_destroy(endpoint);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        /* Cannot test. uses requesting bundles descriptor
        void *service = NULL;
        rc = bundleContext_getService(context, ref, &service);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(service != NULL);
         */
    }

    static void testImportService(void) {
        celix_service_use_options_t opts{};
        opts.filter.serviceName = OSGI_RSA_REMOTE_SERVICE_ADMIN;
        opts.use = testImportServiceCallback;
        opts.filter.ignoreServiceLanguage = true;
        opts.waitTimeoutInSeconds = 0.25;
        bool called = celix_bundleContext_useServiceWithOptions(context, &opts);
        CHECK_TRUE(called);
    }

    static void testBundles(void) {
        array_list_pt bundles = NULL;

        int rc = bundleContext_getBundles(context, &bundles);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(3, arrayList_size(bundles)); //framework, rsa_dfi & calc

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

        arrayList_destroy(bundles);
    }

}


TEST_GROUP(RsaDfiTests) {
    void setup() {
        setupFm();
    }

    void teardown() {
        teardownFm();
    }
};

TEST(RsaDfiTests, InfoTest) {
    testServices();
}

TEST(RsaDfiTests, ExportService) {
    testExportService();
}

TEST(RsaDfiTests, ImportService) {
    testImportService();
}

TEST(RsaDfiTests, TestBundles) {
    testBundles();
}
