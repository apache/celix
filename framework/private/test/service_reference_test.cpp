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
 * service_reference_test.cpp
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
#include "service_reference_private.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(service_reference) {
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

TEST(service_reference, create) {
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;

	service_reference_pt reference = NULL;
	serviceReference_create(pool, bundle, registration, &reference);

	POINTERS_EQUAL(bundle, reference->bundle);
	POINTERS_EQUAL(registration, reference->registration);
}

TEST(service_reference, getBundle) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	bundle_pt bundle = (bundle_pt) 0x10;
	reference->bundle = bundle;

	bundle_pt actual = NULL;
	celix_status_t status = serviceReference_getBundle(reference, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundle, actual);
}

TEST(service_reference, getServiceRegistration) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;

	service_registration_pt actual = NULL;
	celix_status_t status = serviceReference_getServiceRegistration(reference, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registration, actual);
}

TEST(service_reference, invalidate) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;

	celix_status_t status = serviceReference_invalidate(reference);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, reference->registration);
}

TEST(service_reference, getUsingBundle) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;

	service_registry_pt registry = (service_registry_pt) 0x20;

	array_list_pt bundles = NULL;
	arrayList_create(&bundles);
	bundle_pt bundle = (bundle_pt) 0x30;
	arrayList_add(bundles, bundle);

	mock().expectOneCall("serviceRegistration_getRegistry")
		.withParameter("registration", registration)
		.andOutputParameter("registry", registry);
	mock().expectOneCall("serviceRegistry_getUsingBundles")
		.withParameter("registry", registry)
		.withParameter("pool", pool)
		.withParameter("reference", reference)
		.andReturnValue(bundles);

	array_list_pt actual = NULL;
	celix_status_t status = serviceReference_getUsingBundles(reference, pool, &actual);
	POINTERS_EQUAL(bundles, actual);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(bundle, arrayList_get(actual, 0));
}

TEST(service_reference, equals) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->bundle = bundle;

	service_reference_pt toCompare = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	registration = (service_registration_pt) 0x10;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->bundle = bundle;

	bool equal = false;
	celix_status_t status = serviceReference_equals(reference, toCompare, &equal);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(true, equal);

	toCompare = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	registration = (service_registration_pt) 0x11;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->bundle = bundle;

	equal = true;
	status = serviceReference_equals(reference, toCompare, &equal);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(false, equal);
}

TEST(service_reference, equals2) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->bundle = bundle;

	service_reference_pt toCompare = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	registration = (service_registration_pt) 0x10;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->bundle = bundle;

	bool equal = serviceReference_equals2(reference, toCompare);
	LONGS_EQUAL(true, equal);

	toCompare = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	registration = (service_registration_pt) 0x11;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->bundle = bundle;

	equal = serviceReference_equals2(reference, toCompare);
	LONGS_EQUAL(false, equal);
}

TEST(service_reference, hashCode) {
	service_reference_pt reference = (service_reference_pt) apr_palloc(pool, sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->bundle = bundle;

	unsigned int hash = serviceReference_hashCode(reference);
	LONGS_EQUAL(79, hash);
}
