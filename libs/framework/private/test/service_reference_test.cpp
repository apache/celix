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
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "service_reference_private.h"
#include "service_reference.h"
#include "constants.h"
#include "celix_log.h"
#include "service_registry.h"
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

static char* my_strdup(const char* s){
	if(s==NULL){
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc (len + 1,sizeof(char));

	if (d == NULL){
		return NULL;
	}

	strncpy (d,s,len);
	return d;
}


TEST_GROUP(service_reference) {
	void setup(void) {
	}

	void teardown() {
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

	mock().checkExpectations();
}

TEST(service_reference, retain){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	reference->refCount = 1;

    celix_status_t status = serviceReference_retain(reference);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(2, reference->refCount);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, release){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	service_registration_pt registration = (service_registration_pt) 0x20;
	reference->registration = registration;
	reference->refCount = 2;

	bool destroyed;
	celix_status_t status = serviceReference_release(reference, &destroyed);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_FALSE(destroyed);
	LONGS_EQUAL(1, reference->refCount);
	
	mock().expectOneCall("serviceRegistration_release")
			.withParameter("registration", registration);
	
	status = serviceReference_release(reference, &destroyed);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(destroyed);

	mock().checkExpectations();
}

TEST(service_reference, increaseUsage){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	reference->usageCount = 1;

	size_t get_count;
	celix_status_t status = serviceReference_increaseUsage(reference, &get_count);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(2 == get_count);
	CHECK(2 == reference->usageCount);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, decreaseUsage){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	reference->usageCount = 2;

	//test decreasing usage
	size_t get_count;
	celix_status_t status = serviceReference_decreaseUsage(reference, &get_count);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(1 == get_count);
	CHECK(1 == reference->usageCount);


	status = serviceReference_decreaseUsage(reference, &get_count);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(0 == get_count);
	CHECK(0 == reference->usageCount);

	//test attempting to decrease usage below 0
	mock().expectOneCall("framework_log");

	status = serviceReference_decreaseUsage(reference, &get_count);
	LONGS_EQUAL(CELIX_BUNDLE_EXCEPTION, status);
	CHECK(0 == get_count);
	CHECK(0 == reference->usageCount);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, getUsageCount){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	reference->usageCount = 5;

	size_t get_count;
	celix_status_t status = serviceReference_getUsageCount(reference, &get_count);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(5 == get_count);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, getReferenceCount){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	reference->refCount = 5;

	size_t get_count;
	celix_status_t status = serviceReference_getReferenceCount(reference, &get_count);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(5 == get_count);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, getService){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	void * service = (void*) 0x01;
	reference->service = service;

	void * get_service;
	celix_status_t status = serviceReference_getService(reference, &get_service);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, get_service);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, setService){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	void * service = (void*) 0x01;

	celix_status_t status = serviceReference_setService(reference, service);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, reference->service);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
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

	mock().checkExpectations();
}

TEST(service_reference, getOwner){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	celixThreadRwlock_create(&reference->lock, NULL);
	bundle_pt bundle = (bundle_pt) 0x10;
	reference->registrationBundle = bundle;
	bundle_pt owner = (bundle_pt) 0x20;
	reference->referenceOwner = owner;
	reference->registration = (service_registration_pt) 0x30;

	bundle_pt actual = NULL;
	celix_status_t status = serviceReference_getOwner(reference, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(owner, actual);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
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

	mock().checkExpectations();
}

TEST(service_reference, getProperty){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	celixThreadRwlock_create(&reference->lock, NULL);
	properties_pt props = (properties_pt) 0x20;
	char * key = my_strdup("key");
	char * value = my_strdup("value");
	const char * get_value = (char*) NULL;

	//test getting a property
	mock().expectOneCall("serviceRegistration_getProperties")
			.withParameter("registration", registration)
			.withOutputParameterReturning("properties", &props, sizeof(props));

	mock().expectOneCall("properties_get")
			.withParameter("key", key)
			.withParameter("properties", props)
			.ignoreOtherParameters()
			.andReturnValue(value);

	celix_status_t status = serviceReference_getProperty(reference, key, &get_value);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	STRCMP_EQUAL(value, get_value);

	//check invalid registration
	reference->registration = (service_registration_pt) NULL;

	status = serviceReference_getProperty(reference, key, &get_value);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(NULL, get_value);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
	free(key);
	free(value);

	mock().checkExpectations();
}

static void mock_register_str(char * str){
	mock().actualCall("mock_register_str")
			.withStringParameter("str", str);
}

TEST(service_reference, getPropertyKeys){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	celixThreadRwlock_create(&reference->lock, NULL);
	properties_pt props = hashMap_create(NULL, NULL, NULL, NULL);
	char * key = my_strdup("key");
	char * key2 = my_strdup("key2");
	char * key3 = my_strdup("key3");
	hashMap_put(props, key, (void*) 0x01);
	hashMap_put(props, key2, (void*) 0x02);
	hashMap_put(props, key3, (void*) 0x03);

	char **keys;
	unsigned int size;
	mock().expectOneCall("serviceRegistration_getProperties")
			.withParameter("registration", registration)
			.withOutputParameterReturning("properties", &props, sizeof(props));

	serviceReference_getPropertyKeys(reference, &keys, &size);

	UNSIGNED_LONGS_EQUAL(3, size);
	//check for all 3 keys, in no specific order
	mock().expectOneCall("mock_register_str")
			.withParameter("str", key);
	mock().expectOneCall("mock_register_str")
			.withParameter("str", key2);
	mock().expectOneCall("mock_register_str")
			.withParameter("str", key3);

	mock_register_str(keys[0]);
	mock_register_str(keys[1]);
	mock_register_str(keys[2]);

	hashMap_destroy(props, true, false);
	celixThreadRwlock_destroy(&reference->lock);
	free(reference);
	free(keys);

	mock().checkExpectations();
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

	mock().checkExpectations();
}

TEST(service_reference, isValid){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	reference->registration = (service_registration_pt) 0x10;
	celixThreadRwlock_create(&reference->lock, NULL);

	mock().checkExpectations();
	celix_status_t status;
	bool result = false;

	//test valid
	status = serviceReference_isValid(reference, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK(result);

	//test invalid
	reference->registration = NULL;
	status = serviceReference_isValid(reference, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	CHECK_FALSE(result);

	celixThreadRwlock_destroy(&reference->lock);
	free(reference);

	mock().checkExpectations();
}

TEST(service_reference, isAssignableTo){
	bool result = false;
	result = serviceReference_isAssignableTo(NULL, NULL, NULL);
	CHECK(result);
}

TEST(service_reference, equals) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	reference->registration = (service_registration_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->registrationBundle = bundle;
	celixThreadRwlock_create(&reference->lock, NULL);

	service_reference_pt toCompare = (service_reference_pt) malloc(sizeof(*reference));
	toCompare->registration = (service_registration_pt) 0x10;
	bundle = (bundle_pt) 0x30;
	toCompare->registrationBundle = bundle;
	celixThreadRwlock_create(&toCompare->lock, NULL);

	bool equal = false;
	celix_status_t status = serviceReference_equals(reference, toCompare, &equal);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(true, equal);

	toCompare->registration = (service_registration_pt) 0x11;

	equal = true;
	status = serviceReference_equals(reference, toCompare, &equal);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(false, equal);

	status = serviceReference_equals(NULL, toCompare, &equal);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
	LONGS_EQUAL(false, equal);

	celixThreadRwlock_destroy(&toCompare->lock);
	celixThreadRwlock_destroy(&reference->lock);
	free(toCompare);
	free(reference);

	mock().checkExpectations();
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

	mock().checkExpectations();
}

TEST(service_reference, compareTo){
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	bundle_pt bundle = (bundle_pt) 0x20;
	reference->registrationBundle = bundle;
	properties_pt props = (properties_pt) 0x30;
	celixThreadRwlock_create(&reference->lock, NULL);

	service_reference_pt reference2 = (service_reference_pt) malloc(sizeof(*reference2));
	service_registration_pt registration2 = (service_registration_pt) 0x40;
	reference2->registration = registration2;
	bundle_pt bundle2 = (bundle_pt) 0x50;
	reference2->registrationBundle = bundle2;
	properties_pt props2 = (properties_pt) 0x60;
	celixThreadRwlock_create(&reference2->lock, NULL);

	int compare;

	mock().expectNCalls(12, "serviceRegistration_getProperties")
			.withParameter("registration", registration)
			.withOutputParameterReturning("properties", &props, sizeof(props));
	mock().expectNCalls(12, "serviceRegistration_getProperties")
			.withParameter("registration", registration2)
			.withOutputParameterReturning("properties", &props2, sizeof(props2));

	//service 1 is higher ranked and has a irrelevant ID
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.withParameter("properties", props)
			.ignoreOtherParameters()
			.andReturnValue("2");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.withParameter("properties", props2)
			.ignoreOtherParameters()
			.andReturnValue("1");

	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.withParameter("properties", props)
			.ignoreOtherParameters()
			.andReturnValue("2");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.withParameter("properties", props2)
			.ignoreOtherParameters()
			.andReturnValue("1");

	serviceReference_compareTo(reference, reference2, &compare);
	LONGS_EQUAL(1, compare);

	//service 1 is equally ranked and has a lower ID
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.withParameter("properties", props)
			.ignoreOtherParameters()
			.andReturnValue("1");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.withParameter("properties", props2)
			.ignoreOtherParameters()
			.andReturnValue("2");

	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.withParameter("properties", props)
			.ignoreOtherParameters()
			.andReturnValue("1");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("1");

	serviceReference_compareTo(reference, reference2, &compare);
	LONGS_EQUAL(1, compare);

	//service 1 is equally ranked and has a higher ID
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("2");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.withParameter("properties", props2)
			.ignoreOtherParameters()
			.andReturnValue("1");

	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("1");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("1");

	serviceReference_compareTo(reference, reference2, &compare);
	LONGS_EQUAL(-1, compare);

	//service 1 is lower ranked and has a irrelevant ID
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("1");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("2");

	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("1");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("2");

	serviceReference_compareTo(reference, reference2, &compare);
	LONGS_EQUAL(-1, compare);

	//service 1 is equal in ID and irrelevantly ranked
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("1);");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("1");

	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("1");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("1");

	serviceReference_compareTo(reference, reference2, &compare);
	LONGS_EQUAL(0, compare);

	//services have no rank and service 1 has a higher ID
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue("2");
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_ID)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue("1");

	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props)
			.andReturnValue((void*)NULL);
	mock().expectOneCall("properties_get")
			.withParameter("key", OSGI_FRAMEWORK_SERVICE_RANKING)
			.ignoreOtherParameters()
			.withParameter("properties", props2)
			.andReturnValue((void*)NULL);

	serviceReference_compareTo(reference, reference2, &compare);
	LONGS_EQUAL(-1, compare);

	free(reference);
	free(reference2);

	mock().checkExpectations();
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

	mock().checkExpectations();
}

TEST(service_reference, getUsingBundle) {
	service_reference_pt reference = (service_reference_pt) malloc(sizeof(*reference));
	service_registration_pt registration = (service_registration_pt) 0x10;
	reference->registration = registration;
	service_registry_pt registry = (service_registry_pt) 0x20;
	registry_callback_t callback;
	memset(&callback,0,sizeof(registry_callback_t));
	callback.getUsingBundles = serviceReferenceTest_getUsingBundles;
	callback.handle = registry;
	reference->callback = callback;
	celixThreadRwlock_create(&reference->lock, NULL);

	//test correct functionality
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

	//test getusingbundles = null
	callback.getUsingBundles = NULL;
	reference->callback = callback;
	actual = NULL;

	mock().expectOneCall("serviceRegistration_retain")
			.withParameter("registration", registration);
	mock().expectOneCall("framework_log");
	mock().expectOneCall("serviceRegistration_release")
			.withParameter("registration", registration);

	status = serviceReference_getUsingBundles(reference, &actual);
	LONGS_EQUAL(status,CELIX_BUNDLE_EXCEPTION);
	POINTERS_EQUAL(NULL, actual);

	celixThreadRwlock_destroy(&reference->lock);
	arrayList_destroy(bundles);
	free(reference);

	mock().checkExpectations();
}
