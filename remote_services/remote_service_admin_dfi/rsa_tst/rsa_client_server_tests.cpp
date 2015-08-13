/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include <remote_constants.h>
#include <constants.h>
#include <tst_service.h>
#include "CppUTest/CommandLineTestRunner.h"
#include "../../examples/calculator_service/public/include/calculator_service.h"

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "launcher.h"
#include "framework.h"
#include "remote_service_admin.h"
#include "calculator_service.h"

    static framework_pt serverFramework = NULL;
    static bundle_context_pt serverContext = NULL;

    static framework_pt clientFramework = NULL;
    static bundle_context_pt clientContext = NULL;

    static void setupFm(void) {
        int rc = 0;
        bundle_pt bundle = NULL;

        //server
        rc = celixLauncher_launch("server.properties", &serverFramework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle = NULL;
        rc = framework_getFrameworkBundle(serverFramework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &serverContext);
        CHECK_EQUAL(CELIX_SUCCESS, rc);


        //client
        rc = celixLauncher_launch("client.properties", &clientFramework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle = NULL;
        rc = framework_getFrameworkBundle(clientFramework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &clientContext);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
    }

    static void teardownFm(void) {
        int rc = 0;

        celixLauncher_stop(serverFramework);
        celixLauncher_waitForShutdown(serverFramework);
        celixLauncher_destroy(serverFramework);

        celixLauncher_stop(clientFramework);
        celixLauncher_waitForShutdown(clientFramework);
        celixLauncher_destroy(clientFramework);

        serverContext = NULL;
        serverFramework = NULL;
        clientContext = NULL;
        clientFramework = NULL;
    }

    static void test1(void) {
        int rc = 0;
        service_reference_pt ref = NULL;
        tst_service_pt tst = NULL;

        usleep(2000000); //TODO use tracker

        bundleContext_getServiceReference(clientContext, (char *)TST_SERVICE_NAME, &ref);
        CHECK_EQUAL(0, rc);
        CHECK(ref != NULL);

        bundleContext_getService(clientContext, ref, (void **)&tst);
        CHECK_EQUAL(0, rc);
        CHECK(tst != NULL);

        rc = tst->test(tst->handle);
        CHECK_EQUAL(0, rc);

        bundleContext_ungetService(clientContext, ref, NULL);
        bundleContext_ungetServiceReference(clientContext, ref);
    }

}


TEST_GROUP(RsaDfiClientServerTests) {
    void setup() {
        setupFm();
    }

    void teardown() {
        teardownFm();
    }
};

TEST(RsaDfiClientServerTests, Test1) {
    test1();
}
