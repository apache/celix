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
/**
 * config_admin_test.cpp
 *
 * 	\date       Sep 15, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
#include "celix_threads.h"
#include "array_list.h"

#include "celix_launcher.h"
#include "celix_constants.h"
#include "framework.h"
#include "configuration_admin.h"
#include "example_managed_service_impl.h"
#include "example2_managed_service_impl.h"


static framework_pt framework = NULL;
static bundle_context_pt context = NULL;

service_reference_pt confAdminRef = NULL;
configuration_admin_service_pt confAdminServ = NULL;

service_reference_pt testRef = NULL;
tst_service_pt testServ = NULL;

service_reference_pt test2Ref = NULL;
tst2_service_pt test2Serv = NULL;

	void setupFw(void) {
    	int rc = 0;

        rc = celixLauncher_launch("config.properties", &framework);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        bundle_pt bundle = NULL;
        rc = framework_getFrameworkBundle(framework, &bundle);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundle_getContext(bundle, &context);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *) CONFIGURATION_ADMIN_SERVICE_NAME, &confAdminRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

		rc = bundleContext_getService(context, confAdminRef, (void **) &confAdminServ);
		CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *) TST_SERVICE_NAME, &testRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

		rc = bundleContext_getService(context, testRef, (void **)&testServ);
		CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_getServiceReference(context, (char *) TST2_SERVICE_NAME, &test2Ref);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

		rc = bundleContext_getService(context, test2Ref, (void **)&test2Serv);
		CHECK_EQUAL(CELIX_SUCCESS, rc);

	}

	void teardownFw(void) {
        int rc = 0;

		rc = bundleContext_ungetService(context, testRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetServiceReference(context, testRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetService(context, test2Ref, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetServiceReference(context, test2Ref);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

 		rc = bundleContext_ungetService(context, confAdminRef, NULL);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        rc = bundleContext_ungetServiceReference(context, confAdminRef);
        CHECK_EQUAL(CELIX_SUCCESS, rc);

        celixLauncher_stop(framework);
        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);

        test2Ref = NULL;
        test2Serv = NULL;
        testRef = NULL;
        testServ = NULL;
        confAdminRef = NULL;
        confAdminServ = NULL;
        context = NULL;
        framework = NULL;
	}

	static void cleanUp(void) {
		system("rm -rf .cache");
		system("rm -rf store");
	}
	 static void testBundles(void) {
		 printf("begin: %s\n", __func__);
        array_list_pt bundles = NULL;

        int rc = bundleContext_getBundles(context, &bundles);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(4, arrayList_size(bundles)); //framework, config_admin, example_test, example_test2
        arrayList_destroy(bundles);
		 printf("end: %s\n", __func__);
	 }


	static void testManagedService(void) {
		printf("begin: %s\n", __func__);
		const char *pid = "base.device1";
		char value[80];
		properties_pt properties;
		/* ------------------ get Configuration -------------------*/

		configuration_pt configuration;
		(*confAdminServ->getConfiguration)(confAdminServ->configAdmin,(char *)pid, &configuration);
		configuration->configuration_getProperties(configuration->handle, &properties);
		CHECK((properties ==  NULL));
		testServ->get_type(testServ->handle, value);
		CHECK_TEXT("default_value", value);

		/* ------------------ clear configuration ----------------*/
		configuration->configuration_update(configuration->handle , NULL);
		sleep(1);
		/* check if it is also cleared in the service */
		testServ->get_type(testServ->handle, value);
		CHECK_TEXT("", value);

		/* ------------------ update Configuration ----------------*/
		const char *prop1 = "type";
		const char *value1 = "printer";
		properties = properties_create();
		properties_set(properties, (char *)prop1, (char *)value1);
		// configuration_update transfers ownership of properties structure to the configuration object
		configuration->configuration_update(configuration->handle , properties);

		sleep(1);
		testServ->get_type(testServ->handle, value);
		CHECK_TEXT("printer", value);
		properties = NULL;
		configuration->configuration_getProperties(configuration->handle, &properties);
		if (properties != NULL) {
			const char *test_value = properties_get(properties, (char*)CELIX_FRAMEWORK_SERVICE_PID);
			CHECK_TEXT("base.device1", test_value);
			test_value = properties_get(properties, (char *)prop1);
			CHECK_TEXT("printer", test_value);
		}
		printf("end: %s\n", __func__);
	 }

	static void testManagedServicePersistent(void) {
			printf("begin: %s\n", __func__);
			const char *pid = "base.device1";
			properties_pt properties = NULL;
			const char *key = "type";
			/* ------------------ get Configuration -------------------*/

			configuration_pt configuration;
			(*confAdminServ->getConfiguration)(confAdminServ->configAdmin, (char *)pid, &configuration);
			configuration->configuration_getProperties(configuration->handle, &properties);
			CHECK((properties != NULL));
			if (properties != NULL) {
				const char *test_value = properties_get(properties, (char*)CELIX_FRAMEWORK_SERVICE_PID);
				CHECK_TEXT("base.device1", test_value);
				test_value = properties_get(properties, (char *)key);
				CHECK_TEXT("printer", test_value);
			}

			printf("end: %s\n", __func__);
		 }

	static void testTwoManagedService(void) {
			printf("begin: %s\n", __func__);
			const char *pid = "base.device1";
			const char *tst2Pid = "test2_pid";
			char value[80];
			properties_pt properties;
			properties_pt properties2;
			/* ------------------ get Configuration -------------------*/

			configuration_pt configuration;
			configuration_pt configuration2;
			(*confAdminServ->getConfiguration)(confAdminServ->configAdmin, (char *)pid, &configuration);
			(*confAdminServ->getConfiguration)(confAdminServ->configAdmin, (char *)tst2Pid, &configuration2);

			/* ------------------ update Configuration ----------------*/
			const char *prop1 = "type";
			const char *value1 = "printer";
			properties = properties_create();
			properties_set(properties, (char *)prop1, (char *)value1);
			// configuration_update transfers ownership of properties structure to the configuration object
			configuration->configuration_update(configuration->handle , properties);
			properties2 = properties_create();
			properties_set(properties2, (char *)prop1, (char *)"test2_printer");
			// configuration_update transfers ownership of properties structure to the configuration object
			configuration2->configuration_update(configuration2->handle , properties2);

			sleep(1);
			testServ->get_type(testServ->handle, value);
			CHECK_TEXT("printer", value);
			test2Serv->get_type(test2Serv->handle, value);
			CHECK_TEXT("test2_printer", value);
			properties = NULL;
			configuration->configuration_getProperties(configuration->handle, &properties);
			if (properties != NULL) {
				const char *test_value = properties_get(properties, (char*)CELIX_FRAMEWORK_SERVICE_PID);
				CHECK_TEXT("base.device1", test_value);
				test_value = properties_get(properties, (char *)prop1);
				CHECK_TEXT("printer", test_value);
			}
			configuration2->configuration_getProperties(configuration2->handle, &properties);
			if (properties != NULL) {
				const char *test_value = properties_get(properties, (char*)CELIX_FRAMEWORK_SERVICE_PID);
				CHECK_TEXT("test2_pid", test_value);
				test_value = properties_get(properties, (char *)prop1);
				CHECK_TEXT("test2_printer", test_value);
			}
			printf("end: %s\n", __func__);
		 }

	    static void testAddManagedServiceProperty(void) {
			printf("begin: %s\n", __func__);
			const char *pid = "base.device1";
			char value[80];
			properties_pt properties;
			/* ------------------ get Configuration -------------------*/

			configuration_pt configuration;
			(*confAdminServ->getConfiguration)(confAdminServ->configAdmin, (char *)pid, &configuration);

			/* ------------------ update Configuration ----------------*/
			const char *prop1 = "type";
			const char *value1 = "printer";
			const char *prop2 = "second_type";
			const char *value2 = "my_second_value";
			properties = properties_create();
			properties_set(properties, (char *)prop1, (char *)value1);
			// configuration_update transfers ownership of properties structure to the configuration object
			configuration->configuration_update(configuration->handle , properties);
			sleep(1);
			testServ->get_type(testServ->handle, value);
			CHECK_TEXT("printer", value);
			configuration->configuration_getProperties(configuration->handle, &properties);
			CHECK((properties != NULL));
			properties_set(properties, (char *)prop2, (char *)value2);
			// extend an existing configuration with a new property
			configuration->configuration_update(configuration->handle, properties);
			value2 = NULL;
			testServ->get_second_type(testServ->handle, value);
			CHECK_TEXT("my_second_value", value);
			printf("end: %s\n", __func__);
		 }

	    static void testManagedServiceRemoved(void) {
			printf("begin: %s\n", __func__);
			const char *pid = "base.device1";
			char value[80];
			properties_pt properties = NULL;
			bundle_pt	bundle = NULL;
			bundle_pt   fwBundle = NULL;
			bundle_archive_pt	archive = NULL;
			bundle_context_pt   context = NULL;
			const char                *location = NULL;
			/* ------------------ get Configuration -------------------*/

			configuration_pt configuration;
			(*confAdminServ->getConfiguration)(confAdminServ->configAdmin, (char *)pid, &configuration);

			/* ------------------ update Configuration ----------------*/
			const char *prop1 = "type";
			const char *value1 = "printer";
			const char *prop2 = "second_type";
			const char *value2 = "my_second_value";
			char org_location[128];
			properties = properties_create();
			properties_set(properties, (char *)prop1, (char *)value1);
			properties_set(properties, (char *)prop2, (char *)value2);
			// configuration_update transfers ownership of properties structure to the configuration object
			configuration->configuration_update(configuration->handle , properties);
			sleep(1);
			serviceReference_getBundle(testRef, &bundle);
			bundle_getArchive(bundle, &archive);
			bundle_getBundleLocation(bundle, &location);
			bundle_stop(bundle);
			strcpy(org_location, location);
			configuration->configuration_getProperties(configuration->handle, &properties);
			bundle_uninstall(bundle);
			configuration->configuration_getProperties(configuration->handle, &properties);
			CHECK((properties != NULL));
			bundle = NULL;
			framework_getFrameworkBundle(framework, &fwBundle);
		    bundle_getContext(fwBundle, &context);
		    bundleContext_installBundle(context, org_location, &bundle);
			bundle_startWithOptions(bundle, 0);
			// extend an existing configuration with a new property
			configuration->configuration_getProperties(configuration->handle, &properties);
			CHECK((properties != NULL));
			testRef = NULL;
	        bundleContext_getServiceReference(context, (char *) TST_SERVICE_NAME, &testRef);
			testServ = NULL;
			bundleContext_getService(context, testRef, (void **)&testServ);
			testServ->get_second_type(testServ->handle, value);
			CHECK_TEXT("my_second_value", value);
			printf("end: %s\n", __func__);
	}
}



int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}


//----------------------TEST THREAD FUNCTION DECLARATIONS----------------------

//----------------------TESTGROUP DEFINES----------------------

TEST_GROUP(managed_service) {

    void setup(void) {
        cleanUp();
    	setupFw();
    }

	void teardown(void) {
		teardownFw();
	}

};

TEST_GROUP(managed_service_persistent) {

    void setup(void) {
    	setupFw();
    }

	void teardown(void) {
		teardownFw();
	}

};

// TODO: test service factory PID

//----------------------THREAD_POOL TESTS----------------------
TEST(managed_service, test_managed_service_removed) {
	testManagedServiceRemoved();
}

TEST(managed_service, add_managed_service_property) {
    testAddManagedServiceProperty();
}

TEST(managed_service, test_two_managed_service) {
    testTwoManagedService();
}

TEST(managed_service_persistent, test_persistent) {
    testManagedServicePersistent();
}

TEST(managed_service, test_managed_service) {
    testManagedService();
}

TEST(managed_service, test_bundles) {
    testBundles();
}
