/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "launcher.h"
#include "framework.h"
#include "remote_service_admin.h"


framework_pt framework = NULL;
bundle_context_pt context = NULL;
service_reference_pt rsaRef = NULL;
remote_service_admin_service_pt rsa = NULL;

static void setupFm(void) {
    int rc = 0;

    rc = celixLauncher_launch("config.properties", &framework);
    CHECK_EQUAL(CELIX_SUCCESS, rc);

    bundle_pt bundle = NULL;
    rc = framework_getFrameworkBundle(framework, &bundle);
    CHECK_EQUAL(CELIX_SUCCESS, rc);

    rc = bundle_getContext(bundle, &context);
    CHECK_EQUAL(CELIX_SUCCESS, rc);

    rc = bundleContext_getServiceReference(context, (char *)OSGI_RSA_REMOTE_SERVICE_ADMIN, &rsaRef);
    CHECK_EQUAL(CELIX_SUCCESS, rc);

    rc = bundleContext_getService(context, rsaRef, (void **)&rsa);
    CHECK_EQUAL(CELIX_SUCCESS, rc);
}

static void teardownFm(void) {
    int rc = 0;
    rc = bundleContext_ungetService(context, rsaRef, NULL);
    CHECK_EQUAL(CELIX_SUCCESS, rc);

    celixLauncher_stop(framework);
    celixLauncher_waitForShutdown(framework);
    celixLauncher_destroy(framework);

    rsaRef = NULL;
    rsa = NULL;
    context = NULL;
    framework = NULL;
}

static void testInfo(void) {
    int rc = 0;
    array_list_pt exported = NULL;
    array_list_pt imported = NULL;
    arrayList_create(&exported);
    arrayList_create(&imported);

    rc = rsa->getExportedServices(rsa->admin, &exported);
    CHECK_EQUAL(CELIX_SUCCESS, rc);
    CHECK_EQUAL(0, arrayList_size(exported));

    rc = rsa->getImportedEndpoints(rsa->admin, &imported);
    CHECK_EQUAL(CELIX_SUCCESS, rc);
    CHECK_EQUAL(0, arrayList_size(imported));
}

static void testExportService(void) {
    //TODO
}

static void testImportService(void) {
    //TODO
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
    testInfo();
}

TEST(RsaDfiTests, ExportService) {
    testExportService();
}

TEST(RsaDfiTests, ImportService) {
    testImportService();
}
