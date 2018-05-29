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
/*
 * bundle_context_test.cpp
 *
 *  \date       Sep 14, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "bundle_context_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(bundle_context) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(bundle_context, create) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;

	bundle_context_pt context = NULL;
	bundleContext_create(framework, logger, bundle, &context);
	POINTERS_EQUAL(framework, context->framework)
	POINTERS_EQUAL(bundle, context->bundle)

	bundleContext_create(NULL, logger, NULL, &context);

	bundleContext_destroy(context);
}

TEST(bundle_context, destroy) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);
	bundle_context_pt context = NULL;
        bundleContext_create((framework_pt) 0x10, (framework_logger_pt)(0x30), (bundle_pt)(0x20), &context);

	celix_status_t status = bundleContext_destroy(context);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	status = bundleContext_destroy(NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getBundle) {
	mock().expectNCalls(2, "framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	celix_status_t status;
	bundle_pt actualBundle = NULL;
	status = bundleContext_getBundle(context, &actualBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundle, actualBundle);

	framework_pt actualFramework = NULL;
	status = bundleContext_getFramework(context, &actualFramework);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(framework, actualFramework);

	actualBundle = NULL;
	status = bundleContext_getBundle(NULL, &actualBundle);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	actualFramework = NULL;
	status = bundleContext_getFramework(NULL, &actualFramework);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, installBundle) {
	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char location[] = "test.zip";
	bundle_pt installedBundle = (bundle_pt) 0x40;
	mock().expectOneCall("fw_installBundle")
		.withParameter("framework", framework)
		.withParameter("location", location)
		.withParameter("inputFile", (char *) NULL)
		.withOutputParameterReturning("bundle", &installedBundle, sizeof(installedBundle))
		.andReturnValue(CELIX_SUCCESS);

	bundle_pt actualInstalledBundle = NULL;
	celix_status_t status = bundleContext_installBundle(context, location, &actualInstalledBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(installedBundle, actualInstalledBundle);

	free(context);
}

TEST(bundle_context, installBundle2) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char location[] = "test.zip";
	char inputFile[] = "input.zip";
	bundle_pt installedBundle = (bundle_pt) 0x40;
	mock().expectOneCall("fw_installBundle")
		.withParameter("framework", framework)
		.withParameter("location", location)
		.withParameter("inputFile", inputFile)
		.withOutputParameterReturning("bundle", &installedBundle, sizeof(installedBundle))
		.andReturnValue(CELIX_SUCCESS);

	bundle_pt actualInstalledBundle = NULL;
	celix_status_t status = bundleContext_installBundle2(context, location, inputFile, &actualInstalledBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(installedBundle, actualInstalledBundle);

	actualInstalledBundle = NULL;
	status = bundleContext_installBundle2(NULL, location, inputFile, &actualInstalledBundle);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, registerService) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char serviceName[] = "service";
	void *service = (void *) 0x40;
	properties_pt properties = (properties_pt) 0x50;
	service_registration_pt registration = (service_registration_pt) 0x60;

	mock().expectOneCall("fw_registerService")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("service", service)
		.withParameter("properties", properties)
		.withOutputParameterReturning("registration", &registration, sizeof(registration))
		.andReturnValue(CELIX_SUCCESS);

	service_registration_pt actualRegistration = NULL;
	celix_status_t status = bundleContext_registerService(context, serviceName, service, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registration, actualRegistration);

	actualRegistration = NULL;
	status = bundleContext_registerService(NULL, serviceName, service, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, registerServiceFactory) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char serviceName[] = "service";
	service_factory_pt serviceFactory = (service_factory_pt) 0x40;
	properties_pt properties = (properties_pt) 0x50;
	service_registration_pt registration = (service_registration_pt) 0x60;

	mock().expectOneCall("fw_registerServiceFactory")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("serviceFactory", serviceFactory)
		.withParameter("properties", properties)
		.withOutputParameterReturning("registration", &registration, sizeof(registration))
		.andReturnValue(CELIX_SUCCESS);

	service_registration_pt actualRegistration = NULL;
	celix_status_t status = bundleContext_registerServiceFactory(context, serviceName, serviceFactory, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registration, actualRegistration);

	actualRegistration = NULL;
	status = bundleContext_registerServiceFactory(NULL, serviceName, serviceFactory, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, getServiceReferences) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char serviceName[] = "service";
	char filter[] = "";
	array_list_pt references = (array_list_pt) 0x40;

	mock().expectOneCall("fw_getServiceReferences")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("filter", filter)
		.withOutputParameterReturning("references", &references, sizeof(references))
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt actualReferences = NULL;
	celix_status_t status = bundleContext_getServiceReferences(context, serviceName, filter, &actualReferences);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(references, actualReferences);

	actualReferences = NULL;
	status = bundleContext_getServiceReferences(NULL, serviceName, filter, &actualReferences);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, getServiceReference) {
	mock().expectNCalls(3, "framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char serviceName[] = "service";
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	mock().expectOneCall("fw_getServiceReferences")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("filter", (char *) NULL)
		.withOutputParameterReturning("references", &references, sizeof(references))
		.andReturnValue(CELIX_SUCCESS);

	service_reference_pt actualReference = NULL;
	LONGS_EQUAL(CELIX_SUCCESS, bundleContext_getServiceReference(context, serviceName, &actualReference));
	POINTERS_EQUAL(reference, actualReference);

	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleContext_getServiceReference(NULL, serviceName, &actualReference));
	actualReference = NULL;
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleContext_getServiceReference(context, NULL, &actualReference));

	free(context);
}

TEST(bundle_context, ungetServiceReference) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;
	service_reference_pt reference = (service_reference_pt) 0x30;

    mock().expectOneCall("framework_ungetServiceReference")
    		.withParameter("framework", framework)
    		.withParameter("bundle", bundle)
    		.withParameter("reference", reference);

	LONGS_EQUAL(CELIX_SUCCESS, bundleContext_ungetServiceReference(context, reference));

    LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleContext_ungetServiceReference(context, NULL));

    free(context);
}

TEST(bundle_context, getService) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	service_reference_pt serviceReference = (service_reference_pt) 0x30;
	void *service = (void *) 0x40;

	mock().expectOneCall("fw_getService")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("reference", serviceReference)
		.withOutputParameterReturning("service", &service, sizeof(service))
		.andReturnValue(CELIX_SUCCESS);

	void *actualService = NULL;
	celix_status_t status = bundleContext_getService(context, serviceReference, &actualService);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, actualService);

	actualService = NULL;
	status = bundleContext_getService(context, NULL, &actualService);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, ungetService) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	service_reference_pt serviceReference = (service_reference_pt) 0x30;
	bool result = true;

	mock().expectOneCall("framework_ungetService")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("reference", serviceReference)
		.withOutputParameterReturning("result", &result, sizeof(result))
		.andReturnValue(CELIX_SUCCESS);

	bool actualResult = false;
	celix_status_t status = bundleContext_ungetService(context, serviceReference, &actualResult);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(result, actualResult);

	actualResult = false;
	status = bundleContext_ungetService(context, NULL, &actualResult);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, getBundles) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	array_list_pt bundles = (array_list_pt) 0x40;

	mock().expectOneCall("framework_getBundles")
		.withParameter("framework", framework)
		.andReturnValue(bundles);

	array_list_pt actualBundles = NULL;
	celix_status_t status = bundleContext_getBundles(context, &actualBundles);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundles, actualBundles);

	actualBundles = NULL;
	status = bundleContext_getBundles(NULL, &actualBundles);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, getBundleById) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	bundle_pt result = (bundle_pt) 0x40;
	int id = 1;

	mock().expectOneCall("framework_getBundleById")
		.withParameter("framework", framework)
		.withParameter("id", id)
		.andReturnValue(result);

	bundle_pt actualBundle = NULL;
	celix_status_t status = bundleContext_getBundleById(context, id, &actualBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(result, actualBundle);

	actualBundle = NULL;
	status = bundleContext_getBundleById(NULL, id, &actualBundle);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, addServiceListener) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	service_listener_pt listener = (service_listener_pt) 0x30;
	char filter[] = "";
	mock().expectOneCall("fw_addServiceListener")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("listener", listener)
		.withParameter("filter", filter)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundleContext_addServiceListener(context, listener, filter);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	status = bundleContext_addServiceListener(context, NULL, filter);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, removeServiceListener) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	service_listener_pt listener = (service_listener_pt) 0x30;
	mock().expectOneCall("fw_removeServiceListener")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundleContext_removeServiceListener(context, listener);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	status = bundleContext_removeServiceListener(context, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, addBundleListener) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	bundle_listener_pt listener = (bundle_listener_pt) 0x30;
	mock().expectOneCall("fw_addBundleListener")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundleContext_addBundleListener(context, listener);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	status = bundleContext_addBundleListener(context, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, removeBundleListener) {
	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	bundle_listener_pt listener = (bundle_listener_pt) 0x30;
	mock().expectOneCall("fw_removeBundleListener")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	celix_status_t status = bundleContext_removeBundleListener(context, listener);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	status = bundleContext_removeBundleListener(context, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	free(context);
}

TEST(bundle_context, addFrameworkListener){
		mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

		bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
		framework_pt framework = (framework_pt) 0x10;
		bundle_pt bundle = (bundle_pt) 0x20;
		context->framework = framework;
		context->bundle = bundle;
		framework_listener_pt listener = (framework_listener_pt) 0x30;

	    mock().expectOneCall("fw_addframeworkListener")
	    		.withParameter("framework", framework)
	            .withParameter("bundle", bundle)
	            .withParameter("listener", listener);

		LONGS_EQUAL(CELIX_SUCCESS, bundleContext_addFrameworkListener(context, listener));

		LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleContext_addFrameworkListener(context, NULL));

		free(context);
}

TEST(bundle_context, removeFrameworkListener){
		mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

		bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
		framework_pt framework = (framework_pt) 0x10;
		bundle_pt bundle = (bundle_pt) 0x20;
		context->framework = framework;
		context->bundle = bundle;
		framework_listener_pt listener = (framework_listener_pt) 0x30;

	    mock().expectOneCall("fw_removeframeworkListener")
	    		.withParameter("framework", framework)
	            .withParameter("bundle", bundle)
	            .withParameter("listener", listener);

		LONGS_EQUAL(CELIX_SUCCESS, bundleContext_removeFrameworkListener(context, listener));

		LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, bundleContext_removeFrameworkListener(context, NULL));

		free(context);
}

TEST(bundle_context, getProperty) {
//	mock().expectOneCall("framework_logCode").withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	bundle_context_pt context = (bundle_context_pt) malloc(sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char *name = (char *) "property";
	char *value = (char *) "value";
	mock().expectOneCall("fw_getProperty")
		.withParameter("framework", framework)
		.withParameter("name", name)
		.withStringParameter("defaultValue", NULL)
		.withOutputParameterReturning("value", &value, sizeof(value))
		.andReturnValue(CELIX_SUCCESS);

	const char *actualValue = NULL;
	celix_status_t status = bundleContext_getProperty(context, name, &actualValue);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	STRCMP_EQUAL(value, actualValue);

	free(context);
}
