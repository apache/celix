/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include <remote_constants.h>
#include <constants.h>
#include "CppUTest/CommandLineTestRunner.h"
#include "../../../examples/calculator_service/public/include/calculator_service.h"

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "celix_launcher.h"
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
        celix_status_t status;
        service_reference_pt ref = NULL;
        calculator_service_pt calcService = NULL;
        usleep(2000000); //TODO use tracker

        status = bundleContext_getServiceReference(clientContext, (char *)CALCULATOR_SERVICE, &ref);
        CHECK_EQUAL(CELIX_SUCCESS, status);
        CHECK(ref != NULL);

        status = bundleContext_getService(clientContext, ref, (void **)&calcService);
        CHECK_EQUAL(CELIX_SUCCESS, status);
        CHECK(calcService != NULL);

        double result = 0;
        status = calcService->add(calcService->calculator, 2.0, 5.0, &result);
        CHECK_EQUAL(CELIX_SUCCESS, status);
        CHECK_EQUAL(7.0, result);

        bool ungetResult = false;
        bundleContext_ungetService(clientContext, ref, NULL);
        bundleContext_ungetServiceReference(clientContext, ref);
    }

}


TEST_GROUP(RsaHttpClientServerTests) {
    void setup() {
        setupFm();
    }

    void teardown() {
        teardownFm();
    }
};

TEST(RsaHttpClientServerTests, Test1) {
    test1();
}
