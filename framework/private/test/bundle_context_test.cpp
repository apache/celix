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
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
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

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(bundle_context) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);

		logger = (framework_logger_pt) apr_palloc(pool, sizeof(*logger));
        logger->logFunction = frameworkLogger_log;
	}

	void teardown() {
		apr_pool_destroy(pool);
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(bundle_context, create) {
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;

	mock().expectOneCall("bundle_getMemoryPool")
		.withParameter("bundle", bundle)
		.andOutputParameter("pool", pool)
		.andReturnValue(CELIX_SUCCESS);

	bundle_context_pt context = NULL;
	bundleContext_create(framework, logger, bundle, &context);
	POINTERS_EQUAL(framework, context->framework)
	POINTERS_EQUAL(bundle, context->bundle)
	CHECK(context->pool);
}

TEST(bundle_context, destroy) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *spool = NULL;
	apr_pool_create(&spool, pool);
	context->framework = framework;
	context->bundle = bundle;
	context->pool = spool;

	celix_status_t status = bundleContext_destroy(context);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	status = bundleContext_destroy(NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getBundle) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *pool = (apr_pool_t *) 0x30;
	context->framework = framework;
	context->bundle = bundle;
	context->pool = pool;

	celix_status_t status;
	bundle_pt actualBundle = NULL;
	status = bundleContext_getBundle(context, &actualBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundle, actualBundle);

	framework_pt actualFramework = NULL;
	status = bundleContext_getFramework(context, &actualFramework);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(framework, actualFramework);

	apr_pool_t *actualPool = NULL;
	status = bundleContext_getMemoryPool(context, &actualPool);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(pool, actualPool);

	actualBundle = NULL;
	status = bundleContext_getBundle(NULL, &actualBundle);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	actualFramework = NULL;
	status = bundleContext_getFramework(NULL, &actualFramework);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	actualPool = NULL;
	status = bundleContext_getMemoryPool(NULL, &actualPool);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, installBundle) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *pool = (apr_pool_t *) 0x30;
	context->framework = framework;
	context->bundle = bundle;
	context->pool = pool;

	char location[] = "test.zip";
	bundle_pt installedBundle = (bundle_pt) 0x40;
	mock().expectOneCall("fw_installBundle")
		.withParameter("framework", framework)
		.withParameter("location", location)
		.withParameter("inputFile", (char *) NULL)
		.andOutputParameter("bundle", installedBundle)
		.andReturnValue(CELIX_SUCCESS);

	bundle_pt actualInstalledBundle = NULL;
	celix_status_t status = bundleContext_installBundle(context, location, &actualInstalledBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(installedBundle, actualInstalledBundle);
}

TEST(bundle_context, installBundle2) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *pool = (apr_pool_t *) 0x30;
	context->framework = framework;
	context->bundle = bundle;
	context->pool = pool;

	char location[] = "test.zip";
	char inputFile[] = "input.zip";
	bundle_pt installedBundle = (bundle_pt) 0x40;
	mock().expectOneCall("fw_installBundle")
		.withParameter("framework", framework)
		.withParameter("location", location)
		.withParameter("inputFile", inputFile)
		.andOutputParameter("bundle", installedBundle)
		.andReturnValue(CELIX_SUCCESS);

	bundle_pt actualInstalledBundle = NULL;
	celix_status_t status = bundleContext_installBundle2(context, location, inputFile, &actualInstalledBundle);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(installedBundle, actualInstalledBundle);

	actualInstalledBundle = NULL;
	status = bundleContext_installBundle2(NULL, location, inputFile, &actualInstalledBundle);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, registerService) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *pool = (apr_pool_t *) 0x30;
	context->framework = framework;
	context->bundle = bundle;
	context->pool = pool;

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
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);

	service_registration_pt actualRegistration = NULL;
	celix_status_t status = bundleContext_registerService(context, serviceName, service, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registration, actualRegistration);

	actualRegistration = NULL;
	status = bundleContext_registerService(NULL, serviceName, service, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, registerServiceFactory) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *pool = (apr_pool_t *) 0x30;
	context->framework = framework;
	context->bundle = bundle;
	context->pool = pool;

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
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);

	service_registration_pt actualRegistration = NULL;
	celix_status_t status = bundleContext_registerServiceFactory(context, serviceName, serviceFactory, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registration, actualRegistration);

	actualRegistration = NULL;
	status = bundleContext_registerServiceFactory(NULL, serviceName, serviceFactory, properties, &actualRegistration);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getServiceReferences) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	apr_pool_t *pool = (apr_pool_t *) 0x30;
	context->framework = framework;
	context->bundle = bundle;
	context->pool = pool;

	char serviceName[] = "service";
	char filter[] = "";
	array_list_pt references = (array_list_pt) 0x40;

	mock().expectOneCall("fw_getServiceReferences")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("filter", filter)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt actualReferences = NULL;
	celix_status_t status = bundleContext_getServiceReferences(context, serviceName, filter, &actualReferences);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(references, actualReferences);

	actualReferences = NULL;
	status = bundleContext_getServiceReferences(NULL, serviceName, filter, &actualReferences);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getServiceReference) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);

	service_reference_pt actualReference = NULL;
	celix_status_t status = bundleContext_getServiceReference(context, serviceName, &actualReference);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(reference, actualReference);

	actualReference = NULL;
	status = bundleContext_getServiceReference(context, NULL, &actualReference);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getService) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char serviceName[] = "service";
	service_reference_pt serviceReference = (service_reference_pt) 0x30;
	void *service = (void *) 0x40;

	mock().expectOneCall("fw_getService")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("reference", serviceReference)
		.andOutputParameter("service", service)
		.andReturnValue(CELIX_SUCCESS);

	void *actualService = NULL;
	celix_status_t status = bundleContext_getService(context, serviceReference, &actualService);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, actualService);

	actualService = NULL;
	status = bundleContext_getService(context, NULL, &actualService);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, ungetService) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char serviceName[] = "service";
	service_reference_pt serviceReference = (service_reference_pt) 0x30;
	bool result = true;

	mock().expectOneCall("framework_ungetService")
		.withParameter("framework", framework)
		.withParameter("bundle", bundle)
		.withParameter("reference", serviceReference)
		.andOutputParameter("result", result)
		.andReturnValue(CELIX_SUCCESS);

	bool actualResult = false;
	celix_status_t status = bundleContext_ungetService(context, serviceReference, &actualResult);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(result, actualResult);

	actualResult = false;
	status = bundleContext_ungetService(context, NULL, &actualResult);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getBundles) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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
}

TEST(bundle_context, getBundleById) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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
}

TEST(bundle_context, addServiceListener) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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

	bundle_pt actualBundle = NULL;
	celix_status_t status = bundleContext_addServiceListener(context, listener, filter);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	actualBundle = NULL;
	status = bundleContext_addServiceListener(context, NULL, filter);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, removeServiceListener) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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

	bundle_pt actualBundle = NULL;
	celix_status_t status = bundleContext_removeServiceListener(context, listener);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	actualBundle = NULL;
	status = bundleContext_removeServiceListener(context, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, addBundleListener) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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

	bundle_pt actualBundle = NULL;
	celix_status_t status = bundleContext_addBundleListener(context, listener);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	actualBundle = NULL;
	status = bundleContext_addBundleListener(context, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, removeBundleListener) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
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

	bundle_pt actualBundle = NULL;
	celix_status_t status = bundleContext_removeBundleListener(context, listener);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	actualBundle = NULL;
	status = bundleContext_removeBundleListener(context, NULL);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(bundle_context, getProperty) {
	bundle_context_pt context = (bundle_context_pt) apr_palloc(pool, sizeof(*context));
	framework_pt framework = (framework_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	context->framework = framework;
	context->bundle = bundle;

	char name[] = "property";
	char value[] = "value";
	mock().expectOneCall("fw_getProperty")
		.withParameter("framework", framework)
		.withParameter("name", name)
		.withParameter("value", value)
		.andReturnValue(CELIX_SUCCESS);

	char *actualValue = NULL;
	celix_status_t status = bundleContext_getProperty(context, name, &actualValue);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	STRCMP_EQUAL(value, actualValue);

	actualValue = NULL;
	status = bundleContext_getProperty(context, NULL, &actualValue);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}
