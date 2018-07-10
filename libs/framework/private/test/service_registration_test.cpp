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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"


extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "service_registration_private.h"
#include "celix_log.h"
#include "service_registry.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;

typedef celix_status_t (*callback_unregister_signature)(void*, bundle_pt, service_registration_pt);
typedef celix_status_t (*callback_modified_signature)(void*, service_registration_pt, properties_pt);

celix_status_t serviceRegistrationTest_getService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service) {
	mock_c()->actualCall("serviceRegistrationTest_getService")
			->withPointerParameters("factory", factory)
			->withPointerParameters("bundle", bundle)
			->withPointerParameters("registration", registration)
			->withOutputParameter("service", service);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t serviceRegistrationTest_ungetService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service) {
	mock_c()->actualCall("serviceRegistrationTest_ungetService")
				->withPointerParameters("factory", factory)
				->withPointerParameters("bundle", bundle)
				->withPointerParameters("registration", registration)
				->withOutputParameter("service", service);
		return mock_c()->returnValue().value.intValue;
}

}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

static char* my_strdup(const char* s) {
	if (s == NULL) {
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc(len + 1, sizeof(char));

	if (d == NULL) {
		return NULL;
	}

	strncpy(d, s, len);
	return d;
}

TEST_GROUP(service_registration) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(service_registration, create) {
	registry_callback_t callback;
	service_registry_pt registry = (service_registry_pt) 0x10;
	callback.handle = registry;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x20;
	unsigned long serviceId = 1UL;
	void *service = (void *) 0x30;

	service_registration_pt registration = serviceRegistration_create(callback, bundle, name, serviceId, service, NULL);

	STRCMP_EQUAL(name, registration->className);
	POINTERS_EQUAL(bundle, registration->bundle);
	POINTERS_EQUAL(service, registration->svcObj);
	UNSIGNED_LONGS_EQUAL(serviceId, registration->serviceId);

	LONGS_EQUAL(0, registration->isUnregistering);
	LONGS_EQUAL(CELIX_PLAIN_SERVICE, registration->svcType);
	POINTERS_EQUAL(NULL, registration->deprecatedFactory);
	POINTERS_EQUAL(NULL, registration->factory);
	POINTERS_EQUAL(NULL, registration->services);
	LONGS_EQUAL(0, registration->nrOfServices);

	const char* get;
	get = properties_get(registration->properties, (char*)"service.id");
	STRCMP_EQUAL("1", get);

	get = properties_get(registration->properties, (char*)"objectClass");
	STRCMP_EQUAL(name, get);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, createServiceFactory) {
	registry_callback_t callback;
	service_registry_pt registry = (service_registry_pt) 0x10;
	callback.handle = registry;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x20;
	unsigned long serviceId = 1UL;
	void *service = (void *) 0x30;

	service_registration_pt registration = serviceRegistration_createServiceFactory(callback, bundle, name, serviceId, service, NULL);

	STRCMP_EQUAL(name, registration->className);
	POINTERS_EQUAL(bundle, registration->bundle);
	POINTERS_EQUAL(service, registration->svcObj);
	UNSIGNED_LONGS_EQUAL(serviceId, registration->serviceId);
	LONGS_EQUAL(0, registration->isUnregistering);
	LONGS_EQUAL(CELIX_DEPRECATED_FACTORY_SERVICE, registration->svcType);
	POINTERS_EQUAL(service, registration->deprecatedFactory);
	POINTERS_EQUAL(NULL, registration->services);
	LONGS_EQUAL(0, registration->nrOfServices);

	const char* get;
	get = properties_get(registration->properties, (char*)"service.id");
	STRCMP_EQUAL("1", get);

	get = properties_get(registration->properties, (char*)"objectClass");
	STRCMP_EQUAL(name, get);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, retain_release){
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, NULL, NULL);

	LONGS_EQUAL(1, registration->refCount);
	serviceRegistration_retain(registration);
	LONGS_EQUAL(2, registration->refCount);
	serviceRegistration_release(registration);
	LONGS_EQUAL(1, registration->refCount);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, isValidTrue) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	void *service = (void *) 0x30;
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, service, NULL);

	bool valid = serviceRegistration_isValid(registration);

	LONGS_EQUAL(1, valid);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, isValidFalse) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, NULL, NULL);

	bool valid = serviceRegistration_isValid(registration);
	LONGS_EQUAL(0, valid);

	valid = serviceRegistration_isValid(NULL);
	LONGS_EQUAL(0, valid);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, invalidate) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	void *service = (void *) 0x30;
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, service, NULL);


	serviceRegistration_invalidate(registration);

	POINTERS_EQUAL(NULL, registration->svcObj);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, unregisterValid) {
	registry_callback_t callback;
	callback.unregister = ( (callback_unregister_signature)serviceRegistry_unregisterService );
	service_registry_pt registry = (service_registry_pt) 0x10;
	callback.handle = registry;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x20;
	void *service = (void *) 0x30;
	service_registration_pt registration = serviceRegistration_create(callback, bundle, name, 0, service, NULL);

	mock().expectOneCall("serviceRegistry_unregisterService")
		.withParameter("registry", registry)
		.withParameter("bundle", bundle)
		.withParameter("registration", registration);

	celix_status_t status = serviceRegistration_unregister(registration);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(1, registration->isUnregistering);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, unregisterInvalid) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = serviceRegistration_create(callback, bundle, name, 0, NULL, NULL);

	mock().expectOneCall("framework_logCode")
				.withParameter("code", CELIX_ILLEGAL_STATE);

	celix_status_t status = serviceRegistration_unregister(registration);
	LONGS_EQUAL(CELIX_ILLEGAL_STATE, status);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, getService) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x10;
	void *service = (void *) 0x20;
	service_registration_pt registration = serviceRegistration_create(callback, bundle, name, 0, service, NULL);

	const void *actual = NULL;
	celix_status_t status = serviceRegistration_getService(registration, bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, actual);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, getServiceFromFactory) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x10;
	void *service = (void *) 0x30;
	service_factory_pt factory = (service_factory_pt) malloc(sizeof(*factory));
	factory->getService = serviceRegistrationTest_getService;
	factory->handle = (void*) 0x40;
	service_registration_pt registration = serviceRegistration_createServiceFactory(callback, bundle, name, 0, factory, NULL);

	mock().expectOneCall("serviceRegistrationTest_getService")
			.withParameter("factory", factory->handle)
			.withParameter("bundle", bundle)
			.withParameter("registration", registration)
			.withOutputParameterReturning("service", &service, sizeof(service));

	const void *actual = NULL;
	celix_status_t status = serviceRegistration_getService(registration, bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, actual);

	serviceRegistration_release(registration);
	free(name);
	free(factory);
}

TEST(service_registration, ungetServiceFromFactory) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x10;
	void *service = (void *) 0x30;
	service_factory_pt factory = (service_factory_pt) malloc(sizeof(*factory));
	factory->ungetService = serviceRegistrationTest_ungetService;
	factory->handle = (void*) 0x40;
	service_registration_pt registration = serviceRegistration_createServiceFactory(callback, bundle, name, 0, factory, NULL);


	mock().expectOneCall("serviceRegistrationTest_ungetService")
			.withParameter("factory", factory->handle)
			.withParameter("bundle", bundle)
			.withParameter("registration", registration)
			.withOutputParameterReturning("service", &service, sizeof(service));

	const void *actual = NULL;
	celix_status_t status = serviceRegistration_ungetService(registration, bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, actual);

	serviceRegistration_release(registration);
	free(name);
	free(factory);
}


TEST(service_registration, getProperties) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 5, NULL, NULL);

	properties_pt actual = NULL;
	celix_status_t status = serviceRegistration_getProperties(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	const char* get;
	get = properties_get(registration->properties, (char*)"service.id");
	STRCMP_EQUAL("5", get);

	get = properties_get(registration->properties, (char*)"objectClass");
	STRCMP_EQUAL(name, get);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, setProperties){
	registry_callback_t callback;
	callback.modified = (callback_modified_signature) serviceRegistry_servicePropertiesModified;
	service_registry_pt registry = (service_registry_pt) 0x10;
	callback.handle = registry;
	char * name = my_strdup("sevice_name");
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, NULL, NULL);

	properties_pt properties = properties_create();
	properties_pt old_properties = registration->properties;

	mock().expectOneCall("serviceRegistry_servicePropertiesModified")
			.withParameter("registry", registry)
			.withParameter("registration", registration)
			.withParameter("oldprops", old_properties);

	serviceRegistration_setProperties(registration, properties);

	POINTERS_EQUAL(properties, registration->properties);

	properties_destroy(old_properties);
	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, getServiceName) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, NULL, NULL);

	const char *actual = NULL;
	celix_status_t status = serviceRegistration_getServiceName(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	STRCMP_EQUAL(name, actual);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, getBundle) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = serviceRegistration_create(callback, bundle, name, 0, NULL, NULL);

	bundle_pt actual = NULL;
	celix_status_t status = serviceRegistration_getBundle(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundle, actual);

	serviceRegistration_release(registration);
	free(name);
}

TEST(service_registration, getBundleIllegalArgument) {
	registry_callback_t callback;
	char * name = my_strdup("sevice_name");
	service_registration_pt registration = serviceRegistration_create(callback, NULL, name, 0, NULL, NULL);
	bundle_pt actual = (bundle_pt) 0x01;

	mock().expectOneCall("framework_logCode")
			.withParameter("code", CELIX_ILLEGAL_ARGUMENT);

	celix_status_t status = serviceRegistration_getBundle(registration, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);

	serviceRegistration_release(registration);
	free(name);
}
