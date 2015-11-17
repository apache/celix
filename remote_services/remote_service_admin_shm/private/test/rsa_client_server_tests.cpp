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
#include "bundle.h"

#define NUM_OF_BUNDLES          3
#define DISCOVERY_CFG_NAME      "apache_celix_rsa_discovery_shm"
#define RSA_HTTP_NAME           "apache_celix_remote_service_admin_shm"
#define TOPOLOGY_MANAGER_NAME   "apache_celix_rs_topology_manager"

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
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;
    service_reference_pt ref = NULL;
    calculator_service_pt calcService = NULL;
    int retries = 24;

    while (ref == NULL && retries > 0) {
    	printf("Waiting for service .. %d\n", retries);
		status = bundleContext_getServiceReference(clientContext, (char *) CALCULATOR_SERVICE, &ref);
		usleep(1000000);
		--retries;
    }
    CHECK_EQUAL(CELIX_SUCCESS, status);
    CHECK(ref != NULL);

    status = bundleContext_getService(clientContext, ref, (void **) &calcService);
    CHECK_EQUAL(CELIX_SUCCESS, status);
    CHECK(calcService != NULL);

    double result = 0;
    status = calcService->add(calcService->calculator, 2.0, 5.0, &result);
    CHECK_EQUAL(CELIX_SUCCESS, status);
    CHECK_EQUAL(7.0, result);

    bundleContext_ungetService(clientContext, ref, NULL);
    bundleContext_ungetServiceReference(clientContext, ref);
}

static celix_status_t getPermutations(long bundleIds[], int from, int to, array_list_pt permutations) {
    celix_status_t status = CELIX_SUCCESS;
    int i = 0;

    if (from == to) {
        long* permutation = (long*) calloc(to + 1, sizeof(bundleIds[0]));

        if (!permutation) {
            status = CELIX_ENOMEM;
        } else {
            for (; i <= to; i++) {
                permutation[i] = bundleIds[i];
            }

            arrayList_add(permutations, permutation);
        }
    } else {
        for (i = from; i <= to; i++) {
            long fromOrg = bundleIds[from];
            long iOrg = bundleIds[i];

            bundleIds[from] = iOrg;
            bundleIds[i] = fromOrg;

            status = getPermutations(bundleIds, from + 1, to, permutations);

            bundleIds[from] = fromOrg;
            bundleIds[i] = iOrg;
        }
    }

    return status;
}

static celix_status_t getRemoteServicesBundles(bundle_context_pt context, long rsaBundles[]) {
    celix_status_t status;
    array_list_pt bundles = NULL;

    status = bundleContext_getBundles(clientContext, &bundles);

    if (status == CELIX_SUCCESS) {
        unsigned int size = arrayList_size(bundles);
        unsigned int bundleCnt = 0;
        unsigned int i;

        for (i = 0; i < size; i++) {
            module_pt module = NULL;
            char *name = NULL;

            bundle_pt bundle = (bundle_pt) arrayList_get(bundles, i);

            status = bundle_getCurrentModule(bundle, &module);

            if (status == CELIX_SUCCESS) {
                status = module_getSymbolicName(module, &name);

            }

            if (status == CELIX_SUCCESS) {
                if ((strcmp(name, DISCOVERY_CFG_NAME) == 0) || (strcmp(name, RSA_HTTP_NAME) == 0) || (strcmp(name, TOPOLOGY_MANAGER_NAME) == 0)) {
                    bundle_archive_pt bundleArchive = NULL;
                    long bundleId = -1;

                    status = bundle_getArchive(bundle, &bundleArchive);

                    if (status == CELIX_SUCCESS) {
                        status = bundleArchive_getId(bundleArchive, &bundleId);
                    }

                    if (status == CELIX_SUCCESS) {
                        rsaBundles[bundleCnt] = bundleId;
                        ++bundleCnt;
                    }
                }
            }
        }

        arrayList_destroy(bundles);
    }

    return status;
}

static celix_status_t stopStartPermutation(bundle_context_pt context, long* permutation) {
    celix_status_t status = CELIX_SUCCESS;
    int y = 0;

    printf("Test stop/start permutation: ");

    for (y = 0; (y < NUM_OF_BUNDLES) && (status == CELIX_SUCCESS); y++) {
        bundle_pt bundle = NULL;

        status = bundleContext_getBundleById(context, permutation[y], &bundle);

        if (status == CELIX_SUCCESS) {
            module_pt module = NULL;
            char *name = NULL;

            status = bundle_getCurrentModule(bundle, &module);

            if (status == CELIX_SUCCESS) {
                status = module_getSymbolicName(module, &name);
                printf("%s (%ld) ", name, permutation[y]);
            }
        }
    }
    printf("\n");

    // stop all bundles
    if (status == CELIX_SUCCESS) {
        for (y = 0; (y < NUM_OF_BUNDLES) && (status == CELIX_SUCCESS); y++) {
            bundle_pt bundle = NULL;

            status = bundleContext_getBundleById(context, permutation[y], &bundle);

            if (status == CELIX_SUCCESS) {
                printf("stop bundle: %ld\n", permutation[y]);
                status = bundle_stop(bundle);
            }

        }
    }

    //usleep(10000000);
    // verify stop state
    if (status == CELIX_SUCCESS) {
        for (y = 0; (y < NUM_OF_BUNDLES) && (status == CELIX_SUCCESS); y++) {
            bundle_pt bundle = NULL;

            status = bundleContext_getBundleById(context, permutation[y], &bundle);

            if (status == CELIX_SUCCESS) {
                bundle_state_e state;
                status = bundle_getState(bundle, &state);
                module_pt module = NULL;
                char *name = NULL;

                status = bundle_getCurrentModule(bundle, &module);

                status = module_getSymbolicName(module, &name);

                printf("bundle %s (%ld) has state %d (should be %d) \n", name, permutation[y], state, OSGI_FRAMEWORK_BUNDLE_RESOLVED);

                if (state != OSGI_FRAMEWORK_BUNDLE_RESOLVED) {
                    printf("bundle %ld has state %d (should be %d) \n", permutation[y], state, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
                    status = CELIX_ILLEGAL_STATE;
                }
            }

        }
    }

    // start all bundles
    if (status == CELIX_SUCCESS) {

        for (y = 0; (y < NUM_OF_BUNDLES) && (status == CELIX_SUCCESS); y++) {
            bundle_pt bundle = NULL;

            status = bundleContext_getBundleById(context, permutation[y], &bundle);

            if (status == CELIX_SUCCESS) {
                printf("start bundle: %ld\n", permutation[y]);
                status = bundle_start(bundle);
            }

        }
    }

    // verify started state
    if (status == CELIX_SUCCESS) {
        for (y = 0; (y < NUM_OF_BUNDLES) && (status == CELIX_SUCCESS); y++) {
            bundle_pt bundle = NULL;

            status = bundleContext_getBundleById(context, permutation[y], &bundle);

            if (status == CELIX_SUCCESS) {
                bundle_state_e state;
                status = bundle_getState(bundle, &state);

                if (state != OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
                    printf("bundle %ld has state %d (should be %d) \n", permutation[y], state, OSGI_FRAMEWORK_BUNDLE_ACTIVE);
                    status = CELIX_ILLEGAL_STATE;
                }
            }

        }
    }

    return status;
}


static void testImport(void) {
    celix_status_t status;
    array_list_pt bundlePermutations = NULL;
    long rsaBundles[NUM_OF_BUNDLES];
    unsigned int i;

    arrayList_create(&bundlePermutations);

    status = getRemoteServicesBundles(clientContext, rsaBundles);
    CHECK_EQUAL(CELIX_SUCCESS, status);

    status = getPermutations(rsaBundles, 0, NUM_OF_BUNDLES - 1, bundlePermutations);
    CHECK_EQUAL(CELIX_SUCCESS, status);

    for (i = 0; i < arrayList_size(bundlePermutations); ++i) {
        long* singlePermutation = (long*) arrayList_get(bundlePermutations, i);

        status = stopStartPermutation(clientContext, singlePermutation);
        CHECK_EQUAL(CELIX_SUCCESS, status);

        // check whether calc service is available
        test1();

        free(singlePermutation);
    }

    arrayList_destroy(bundlePermutations);
}

static void testExport(void) {
    celix_status_t status;
    array_list_pt bundlePermutations = NULL;
    long rsaBundles[NUM_OF_BUNDLES];
    unsigned int i;

    arrayList_create(&bundlePermutations);

    status = getRemoteServicesBundles(serverContext, rsaBundles);
    CHECK_EQUAL(CELIX_SUCCESS, status);

    status = getPermutations(rsaBundles, 0, NUM_OF_BUNDLES - 1, bundlePermutations);
    CHECK_EQUAL(CELIX_SUCCESS, status);

    for (i = 0; i < arrayList_size(bundlePermutations); ++i) {
        long* singlePermutation = (long*) arrayList_get(bundlePermutations, i);

        status = stopStartPermutation(serverContext, singlePermutation);
        CHECK_EQUAL(CELIX_SUCCESS, status);

        // check whether calc service is available
        test1();

        free(singlePermutation);
    }

    arrayList_destroy(bundlePermutations);
}
}

TEST_GROUP(RsaShmClientServerTests) {
    void setup() {
        setupFm();
    }

    void teardown() {
        teardownFm();
    }
};

TEST(RsaShmClientServerTests, Test1) {
    test1();
}

TEST(RsaShmClientServerTests, TestImport) {
    testImport();
}

TEST(RsaShmClientServerTests, TestExport) {
    testExport();
}
