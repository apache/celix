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

	#define DISCOVERY_CFG_NAME      "apache_celix_rsa_discovery_shm"
	#define RSA_HTTP_NAME           "apache_celix_remote_service_admin_shm"
	#define TOPOLOGY_MANAGER_NAME   "apache_celix_rs_topology_manager"
	#define CALCULATOR_PROXY   		"apache_celix_remoting_calculator_proxy"
	#define CALCULATOR_ENDPOINT   	"apache_celix_remoting_calculator_endpoint"

	static framework_pt	serverFramework = NULL;
	static celix_bundle_context_t *serverContext = NULL;

	static framework_pt clientFramework = NULL;
	static celix_bundle_context_t *clientContext = NULL;

	static void setupFm(void) {
		int rc = 0;
		celix_bundle_t *bundle = NULL;

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
		calculator_service_t *calcService = NULL;
		int retries = 6;

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


	static celix_status_t getPermutations(array_list_pt bundleIds, int from, int to, array_list_pt permutations) {
		celix_status_t status = CELIX_SUCCESS;
		int i = 0;

		if (from == to) {
			long* permutation = (long*) calloc(to + 1, sizeof(*permutation));

			if (!permutation) {
				status = CELIX_ENOMEM;
			} else {
				for (; i <= to; i++) {
					permutation[i] = (long) arrayList_get(bundleIds, i);
				}

				arrayList_add(permutations, permutation);
			}
		} else {
			for (i = from; i <= to; i++) {
				long fromOrg = (long) arrayList_get(bundleIds, from);
				long iOrg = (long) arrayList_get(bundleIds, i);

				arrayList_set(bundleIds, from, (void*) iOrg);
				arrayList_set(bundleIds, i, (void*) fromOrg);

				status = getPermutations(bundleIds, from + 1, to, permutations);

				arrayList_set(bundleIds, from, (void*) fromOrg);
				arrayList_set(bundleIds, i, (void*) iOrg);
			}
		}

		return status;
	}

	static celix_status_t getSpecifiedBundles(celix_bundle_context_t *context, array_list_pt bundleNames, array_list_pt retrievedBundles) {
		celix_status_t status;
		array_list_pt bundles = NULL;

		status = bundleContext_getBundles(context, &bundles);

		if (status == CELIX_SUCCESS) {
			unsigned int size = arrayList_size(bundles);
			unsigned int i;

			for (i = 0; i < size; i++) {
				module_pt module = NULL;
				const char *name = NULL;

				celix_bundle_t *bundle = (celix_bundle_t *) arrayList_get(bundles, i);

				status = bundle_getCurrentModule(bundle, &module);

				if (status == CELIX_SUCCESS) {
					status = module_getSymbolicName(module, &name);
				}

				if (status == CELIX_SUCCESS) {
					array_list_iterator_pt iter = arrayListIterator_create(bundleNames);

					while(arrayListIterator_hasNext(iter)) {
						char* bundleName = (char*) arrayListIterator_next(iter);

						if ((strcmp(name, bundleName) == 0)) {

							bundle_archive_pt bundleArchive = NULL;
							long bundleId = -1;

							status = bundle_getArchive(bundle, &bundleArchive);

							if (status == CELIX_SUCCESS) {
								status = bundleArchive_getId(bundleArchive, &bundleId);
							}

							if (status == CELIX_SUCCESS) {
								arrayList_add(retrievedBundles, (void*) bundleId);
								break;
							}
						}
					}

					arrayListIterator_destroy(iter);

				}
			}

			arrayList_destroy(bundles);
		}

		return status;
	}

	static celix_status_t stopStartPermutation(celix_bundle_context_t *context, long* permutation, int size) {
		celix_status_t status = CELIX_SUCCESS;
		int y = 0;

		printf("Test stop/start permutation: ");

		for (y = 0; (y < size) && (status == CELIX_SUCCESS); y++) {
			celix_bundle_t *bundle = NULL;

			status = bundleContext_getBundleById(context, permutation[y], &bundle);

			if (status == CELIX_SUCCESS) {
				module_pt module = NULL;
				const char *name = NULL;

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
			for (y = 0; (y < size) && (status == CELIX_SUCCESS); y++) {
				celix_bundle_t *bundle = NULL;

				status = bundleContext_getBundleById(context, permutation[y], &bundle);

				if (status == CELIX_SUCCESS) {
					printf("stop bundle: %ld\n", permutation[y]);
					status = bundle_stop(bundle);
				}
			}
		}

		// verify stop state
		if (status == CELIX_SUCCESS) {
			for (y = 0; (y < size) && (status == CELIX_SUCCESS); y++) {
				celix_bundle_t *bundle = NULL;

				status = bundleContext_getBundleById(context, permutation[y], &bundle);

				if (status == CELIX_SUCCESS) {
					bundle_state_e state;
					status = bundle_getState(bundle, &state);

					if (state != OSGI_FRAMEWORK_BUNDLE_RESOLVED) {
						printf("bundle %ld has state %d (should be %d) \n", permutation[y], state, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
						status = CELIX_ILLEGAL_STATE;
					}
				}
			}
		}

		// start all bundles
		if (status == CELIX_SUCCESS) {

			for (y = 0; (y < size) && (status == CELIX_SUCCESS); y++) {
				celix_bundle_t *bundle = NULL;

				status = bundleContext_getBundleById(context, permutation[y], &bundle);

				if (status == CELIX_SUCCESS) {
					printf("start bundle: %ld\n", permutation[y]);
					status = bundle_start(bundle);
				}
			}
		}

		// verify started state
		if (status == CELIX_SUCCESS) {
			for (y = 0; (y < size) && (status == CELIX_SUCCESS); y++) {
				celix_bundle_t *bundle = NULL;

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
		array_list_pt bundleNames = NULL;
		array_list_pt bundlePermutations = NULL;
		array_list_pt rsaBundles = NULL;
		unsigned int i, size;

		arrayList_create(&bundleNames);
		arrayList_create(&bundlePermutations);
		arrayList_create(&rsaBundles);

		arrayList_add(bundleNames, (void*) DISCOVERY_CFG_NAME);
		arrayList_add(bundleNames, (void*) RSA_HTTP_NAME);
		arrayList_add(bundleNames, (void*) TOPOLOGY_MANAGER_NAME);

		status = getSpecifiedBundles(clientContext, bundleNames, rsaBundles);
		CHECK_EQUAL(CELIX_SUCCESS, status);
		CHECK_EQUAL(arrayList_size(rsaBundles), arrayList_size(bundleNames));

		status = getPermutations(rsaBundles, 0, arrayList_size(rsaBundles) - 1, bundlePermutations);
		CHECK_EQUAL(CELIX_SUCCESS, status);

		size = arrayList_size(bundlePermutations);

		for (i = 0; i < size; ++i) {
			long* singlePermutation = (long*) arrayList_get(bundlePermutations, i);

			status = stopStartPermutation(clientContext, singlePermutation, arrayList_size(rsaBundles));
			CHECK_EQUAL(CELIX_SUCCESS, status);

			// check whether calc service is available
			test1();

			free(singlePermutation);
		}

		arrayList_destroy(bundlePermutations);
		arrayList_destroy(bundleNames);
		arrayList_destroy(rsaBundles);
	}

	static void testExport(void) {
		celix_status_t status;
		array_list_pt bundleNames = NULL;
		array_list_pt bundlePermutations = NULL;
		array_list_pt rsaBundles = NULL;

		unsigned int i, size;

		arrayList_create(&bundleNames);
		arrayList_create(&bundlePermutations);
		arrayList_create(&rsaBundles);

		arrayList_add(bundleNames, (void*) DISCOVERY_CFG_NAME);
		arrayList_add(bundleNames, (void*) RSA_HTTP_NAME);
		arrayList_add(bundleNames, (void*) TOPOLOGY_MANAGER_NAME);

		status = getSpecifiedBundles(serverContext, bundleNames, rsaBundles);
		CHECK_EQUAL(CELIX_SUCCESS, status);
		CHECK_EQUAL(arrayList_size(rsaBundles), arrayList_size(bundleNames));

		status = getPermutations(rsaBundles, 0, arrayList_size(rsaBundles) - 1, bundlePermutations);
		CHECK_EQUAL(CELIX_SUCCESS, status);

		size = arrayList_size(bundlePermutations);

		for (i = 0; i < size; ++i) {
			long* singlePermutation = (long*) arrayList_get(bundlePermutations, i);

			status = stopStartPermutation(serverContext, singlePermutation, arrayList_size(rsaBundles));
			CHECK_EQUAL(CELIX_SUCCESS, status);

            /* we need to sleep here for a bit to ensure
             * that the client has flushed the old discovery
             * values
             */
            sleep(2);

			// check whether calc service is available
			test1();

			free(singlePermutation);
		}

		arrayList_destroy(bundlePermutations);
		arrayList_destroy(bundleNames);
		arrayList_destroy(rsaBundles);
	}

	/*
	static void testProxyRemoval(void) {
		celix_status_t status;
		celix_bundle_t *bundle = NULL;
		array_list_pt bundleNames = NULL;
		array_list_pt proxyBundle = NULL;
		service_reference_pt ref = NULL;

		arrayList_create(&bundleNames);
		arrayList_create(&proxyBundle);

		arrayList_add(bundleNames, (void*) CALCULATOR_PROXY);
		status = getSpecifiedBundles(clientContext, bundleNames, proxyBundle);
		CHECK_EQUAL(CELIX_SUCCESS, status);
		CHECK_EQUAL(arrayList_size(proxyBundle), arrayList_size(bundleNames));

		status = bundleContext_getBundleById(clientContext, (long) arrayList_get(proxyBundle, 0), &bundle);
		CHECK_EQUAL(CELIX_SUCCESS, status);

		status = bundle_stop(bundle);
		CHECK_EQUAL(CELIX_SUCCESS, status);

		status = bundleContext_getServiceReference(clientContext, (char *) CALCULATOR_SERVICE, &ref);
		CHECK_EQUAL(CELIX_SUCCESS, status);
		CHECK(ref == NULL);

		arrayList_destroy(bundleNames);
		arrayList_destroy(proxyBundle);
	}
	*/

	/*
	static void testEndpointRemoval(void) {
		celix_status_t status;
		celix_bundle_t *bundle = NULL;
		array_list_pt bundleNames = NULL;
		array_list_pt endpointBundle = NULL;
		service_reference_pt ref = NULL;

		arrayList_create(&bundleNames);
		arrayList_create(&endpointBundle);

		arrayList_add(bundleNames, (void*) CALCULATOR_ENDPOINT);
		status = getSpecifiedBundles(serverContext, bundleNames, endpointBundle);
		CHECK_EQUAL(CELIX_SUCCESS, status);
		CHECK_EQUAL(arrayList_size(endpointBundle), arrayList_size(bundleNames));

		status = bundleContext_getBundleById(serverContext, (long) arrayList_get(endpointBundle, 0), &bundle);
		CHECK_EQUAL(CELIX_SUCCESS, status);

		status = bundle_stop(bundle);
		CHECK_EQUAL(CELIX_SUCCESS, status);

		status = bundleContext_getServiceReference(serverContext, (char *) CALCULATOR_SERVICE, &ref);
		CHECK_EQUAL(CELIX_SUCCESS, status);
		CHECK(ref == NULL);

		arrayList_destroy(bundleNames);
		arrayList_destroy(endpointBundle);
	}
	*/
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

/*
TEST(RsaShmClientServerTests, TestProxyRemoval) {
	// test is currently failing
	// testProxyRemoval();
}

TEST(RsaShmClientServerTests, TestEndpointRemoval) {
	// test is currently failing
	//testEndpointRemoval();
}
*/
