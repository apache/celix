/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

extern "C" {

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "celix_launcher.h"
#include "framework.h"


    static framework_pt framework = NULL;
    static bundle_context_pt context = NULL;

    static void setupFm(void) {
        int rc = 0;

        rc = celixLauncher_launch("config.properties", &framework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle_pt bundle = NULL;
        rc = framework_getFrameworkBundle(framework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &context);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
    }

    static void teardownFm(void) {
        int rc = 0;

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);

        context = NULL;
        framework = NULL;
    }

    static void testFramework(void) {
        //intentional empty. start/shutdown test
        printf("testing startup/shutdown single framework\n");
    }

    //TODO test register / use service

}


TEST_GROUP(CelixFramework) {
    void setup() {
        setupFm();
    }

    void teardown() {
        teardownFm();
    }
};

TEST(CelixFramework, testFramework) {
    testFramework();
}
