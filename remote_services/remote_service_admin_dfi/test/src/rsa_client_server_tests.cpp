/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <CppUTest/TestHarness.h>
#include <remote_constants.h>
#include <constants.h>
#include <tst_service.h>
#include "CppUTest/CommandLineTestRunner.h"
#include "calculator_service.h"

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
        celix_status_t rc;
        service_reference_pt ref = NULL;
        tst_service_pt tst = NULL;
        int retries = 4;

        while (ref == NULL && retries > 0) {
            printf("Waiting for service .. %d\n", retries);
            rc = bundleContext_getServiceReference(clientContext, (char *) TST_SERVICE_NAME, &ref);
            usleep(1000000);
            --retries;
        }

        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(ref != NULL);

        rc = bundleContext_getService(clientContext, ref, (void **)&tst);
        CHECK_EQUAL(CELIX_SUCCESS, rc);
        CHECK(tst != NULL);

        rc = tst->test(tst->handle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bool result;
        bundleContext_ungetService(clientContext, ref, &result);
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
