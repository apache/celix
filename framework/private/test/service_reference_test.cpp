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
#include "service_reference_private.h"
#include "celix_log.h"
#include "CppUTestExt/MockSupport_c.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;

celix_status_t serviceReferenceTest_getUsingBundles(void * registry, service_registration_pt registration, array_list_pt *bundles){
	mock_c()->actualCall("serviceReferenceTest_getUsingBundles")
			->withPointerParameters("registry", (service_registry_pt)registry)
			->withPointerParameters("registration", registration)
			->withOutputParameter("bundles", bundles);

	return mock_c()->returnValue().value.intValue;
}
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(service_reference) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(service_reference, create) {
	registry_callback_t callback;
	bundle_pt owner = (bundle_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	service_registration_pt registration = (service_registration_pt) 0x30;

	mock().expectOneCall("serviceRegistration_retain")
			.withParameter("registration", registration);

	mock().expectOneCall("serviceRegistration_getBundle")
			.withParameter("registration", registration)
			.withOutputParameterReturning("bundle", &bundle, sizeof(bundle));

	service_reference_pt reference = NULL;
	serviceReference_create(callback, owner, registration, &reference);

	POINTERS_EQUAL(owner, reference->referenceOwner);
	POINTERS_EQUAL(registration, reference->registration);

	mock().expectOneCall("serviceRegistration_release")
			.withParameter("registration", registration);

	bool destroyed;
	serviceReference_release(reference, &destroyed);

	CHECK(destroyed);
}

TEST(service_reference, getBundle) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	bundle_pt bundle = (bundle_pt) 0x10;
	reference->registrationBundle = bundle;
	reference->registration = (service_registration_pt) 0x20;

	bundle_pt actual = NULL;
	celix_status_t status = serviceReference_getBundle(reference, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundle, actual);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
}

TEST(service_reference, getServiceRegistration) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	celixThreadRwlock_create(&reference->lock, NULL);

	service_registration_pt actual = NULL;
	celix_status_t status = serviceReference_getServiceRegistration(reference, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registration, actual);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
}

TEST(service_reference, invalidate) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	celixThreadRwlock_create(&reference->lock, NULL);

	mock().expectOneCall("serviceRegistration_release")
			.withParameter("registration", registration);

	celix_status_t status = serviceReference_invalidate(reference);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, reference->registration);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
}

TEST(service_reference, getUsingBundle) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	service_registry_pt registry = (service_registry_pt) 0x20;
	registry_callback_t callback;
	callback.getUsingBundles = serviceReferenceTest_getUsingBundles;
	callback.handle = registry;
	reference->callback = callback;
	celixThreadRwlock_create(&reference->lock, NULL);



	array_list_pt bundles = NULL;
	arrayList_create(&bundles);
	bundle_pt bundle = (bundle_pt) 0x30;
	arrayList_add(bundles, bundle);

	mock().expectOneCall("serviceRegistration_retain")
			.withParameter("registration", registration);

	mock().expectOneCall("serviceReferenceTest_getUsingBundles")
			.withParameter("registry", registry)
			.withParameter("registration", registration)
			.withOutputParameterReturning("bundles", &bundles, sizeof(bundles));

	mock().expectOneCall("serviceRegistration_release")
				.withParameter("registration", registration);

	array_list_pt actual = NULL;
	celix_status_t status = serviceReference_getUsingBundles(reference, &actual);
	LONGS_EQUAL(status,CELIX_SUCCESS);
	POINTERS_EQUAL(bundles, actual);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(bundle, arrayList_get(actual, 0));


	arrayList_destroy(bundles);
	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
}

TEST(service_reference, equals) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->registrationBundle = bundle;
	celixThreadRwlock_create(&reference->lock, NULL);

	service_reference_pt toCompare = (service_reference_pt) malloc(sizeof(*reference));
	registration = (service_registration_pt) 0x10;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->registrationBundle = bundle;
	celixThreadRwlock_create(&toCompare->lock, NULL);

	bool equal = false;
	celix_status_t status = serviceReference_equals(reference, toCompare, &equal);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(true, equal);

	celixThreadRwlock_destroy(&toCompare->lock);
	free(toCompare);

	toCompare = (service_reference_pt) malloc(sizeof(*reference));
	registration = (service_registration_pt) 0x11;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->registrationBundle = bundle;
	celixThreadRwlock_create(&toCompare->lock, NULL);

	equal = true;
	status = serviceReference_equals(reference, toCompare, &equal);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(false, equal);

	celixThreadRwlock_destroy(&toCompare->lock);
	celixThreadRwlock_destroy(&reference->lock);
	free(toCompare);
	free(reference);
}

TEST(service_reference, equals2) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->registrationBundle = bundle;
	celixThreadRwlock_create(&reference->lock, NULL);

	service_reference_pt toCompare = (service_reference_pt) malloc(sizeof(*reference));
	registration = (service_registration_pt) 0x10;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->registrationBundle = bundle;
	celixThreadRwlock_create(&toCompare->lock, NULL);

	bool equal = serviceReference_equals2(reference, toCompare);
	LONGS_EQUAL(true, equal);

	celixThreadRwlock_destroy(&toCompare->lock);
	free(toCompare);

	toCompare = (service_reference_pt) malloc(sizeof(*reference));
	registration = (service_registration_pt) 0x11;
	toCompare->registration = registration;
	bundle = (bundle_pt) 0x30;
	toCompare->registrationBundle = bundle;
	celixThreadRwlock_create(&toCompare->lock, NULL);

	equal = serviceReference_equals2(reference, toCompare);
	LONGS_EQUAL(false, equal);

	celixThreadRwlock_destroy(&toCompare->lock);
	celixThreadRwlock_destroy(&reference->lock);
	free(toCompare);
	free(reference);
}

TEST(service_reference, hashCode) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->registrationBundle = bundle;
	celixThreadRwlock_create(&reference->lock, NULL);

	unsigned int hash = serviceReference_hashCode(reference);
	LONGS_EQUAL(79, hash);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
}
