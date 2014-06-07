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
 * service_registration_test.cpp
 *
 *  \date       Feb 8, 2013
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
#include "service_registration_private.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(service_registration) {
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

TEST(service_registration, create) {
	service_registry_pt registry = (service_registry_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	std::string serviceName = "service";
	long serviceId = 1l;
	void *service = (void *) 0x30;
	properties_pt properties = (properties_pt) 0x40;

	mock().expectOneCall("properties_create")
		.andReturnValue(properties);
	mock().expectOneCall("properties_set")
		.withParameter("properties", properties)
		.withParameter("key", "service.id")
		.withParameter("value", "1")
		.andReturnValue((char *) NULL);
	mock().expectOneCall("properties_set")
		.withParameter("properties", properties)
		.withParameter("key", "objectClass")
		.withParameter("value", "service")
		.andReturnValue((char *) NULL);

	service_registration_pt registration = serviceRegistration_create(registry, bundle, (char *) serviceName.c_str(), serviceId, service, NULL);

	POINTERS_EQUAL(registry, registration->registry);
	STRCMP_EQUAL("service", registration->className);
	CHECK(registration->references);
	LONGS_EQUAL(0, arrayList_size(registration->references));
	POINTERS_EQUAL(bundle, registration->bundle);
	POINTERS_EQUAL(properties, registration->properties);
	POINTERS_EQUAL(service, registration->svcObj);
	LONGS_EQUAL(serviceId, registration->serviceId);
//	CHECK(registration->mutex);
	LONGS_EQUAL(0, registration->isUnregistering);
	LONGS_EQUAL(0, registration->isServiceFactory);
	POINTERS_EQUAL(NULL, registration->serviceFactory);
	POINTERS_EQUAL(NULL, registration->services);
	LONGS_EQUAL(0, registration->nrOfServices);
}

TEST(service_registration, createServiceFactory) {
	service_registry_pt registry = (service_registry_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	std::string serviceName = "service";
	long serviceId = 1l;
	void *service = (void *) 0x30;
	properties_pt properties = (properties_pt) 0x40;

	mock().expectOneCall("properties_create")
		.andReturnValue(properties);
	mock().expectOneCall("properties_set")
		.withParameter("properties", properties)
		.withParameter("key", "service.id")
		.withParameter("value", "1")
		.andReturnValue((char *) NULL);
	mock().expectOneCall("properties_set")
		.withParameter("properties", properties)
		.withParameter("key", "objectClass")
		.withParameter("value", "service")
		.andReturnValue((char *) NULL);

	service_registration_pt registration = serviceRegistration_createServiceFactory(registry, bundle, (char *) serviceName.c_str(), serviceId, service, NULL);

	POINTERS_EQUAL(registry, registration->registry);
	STRCMP_EQUAL("service", registration->className);
	CHECK(registration->references);
	LONGS_EQUAL(0, arrayList_size(registration->references));
	POINTERS_EQUAL(bundle, registration->bundle);
	POINTERS_EQUAL(properties, registration->properties);
	POINTERS_EQUAL(service, registration->svcObj);
	LONGS_EQUAL(serviceId, registration->serviceId);
//	CHECK(registration->mutex);
	LONGS_EQUAL(0, registration->isUnregistering);
	LONGS_EQUAL(1, registration->isServiceFactory);
	POINTERS_EQUAL(service, registration->serviceFactory);
	POINTERS_EQUAL(NULL, registration->services);
	LONGS_EQUAL(0, registration->nrOfServices);
}

TEST(service_registration, isValidTrue) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	void *service = (void *) 0x30;
	registration->svcObj = service;

	bool valid = serviceRegistration_isValid(registration);

	LONGS_EQUAL(1, valid);
}

TEST(service_registration, isValidFalse) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->svcObj = NULL;

	bool valid = serviceRegistration_isValid(registration);

	LONGS_EQUAL(0, valid);
}

TEST(service_registration, invalidate) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	celixThreadMutex_create(&registration->mutex, NULL);
	void *service = (void *) 0x30;
	registration->svcObj = service;

	serviceRegistration_invalidate(registration);

	POINTERS_EQUAL(NULL, registration->svcObj);
}

TEST(service_registration, unregisterValid) {
	service_registry_pt registry = (service_registry_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->registry = registry;
	registration->bundle = bundle;
	celixThreadMutex_create(&registration->mutex, NULL);
	void *service = (void *) 0x30;
	registration->svcObj = service;

	mock().expectOneCall("serviceRegistry_unregisterService")
		.withParameter("registry", registry)
		.withParameter("bundle", bundle)
		.withParameter("registration", registration);

	celix_status_t status = serviceRegistration_unregister(registration);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	LONGS_EQUAL(1, registration->isUnregistering);
}

TEST(service_registration, unregisterInvalid) {
	service_registry_pt registry = (service_registry_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->registry = registry;
	registration->bundle = bundle;
	celixThreadMutex_create(&registration->mutex, NULL);
	registration->svcObj = NULL;

	celix_status_t status = serviceRegistration_unregister(registration);
	LONGS_EQUAL(CELIX_ILLEGAL_STATE, status);
}

TEST(service_registration, getService) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	bundle_pt bundle = (bundle_pt) 0x10;
	registration->bundle = bundle;
	void *service = (void *) 0x20;
	registration->svcObj = service;
	registration->isServiceFactory = false;

	void *actual = NULL;
	celix_status_t status = serviceRegistration_getService(registration, bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(service, actual);
}

celix_status_t serviceRegistrationTest_getService(void *factory, bundle_pt bundle, service_registration_pt registration, void **service) {
	*service = (void *) 0x20;
	return CELIX_SUCCESS;
}

TEST(service_registration, getServiceFromFactory) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	bundle_pt bundle = (bundle_pt) 0x10;
	registration->bundle = bundle;
	service_factory_pt factory = (service_factory_pt) apr_palloc(pool, sizeof(*factory));
	factory->getService = serviceRegistrationTest_getService;
	registration->svcObj = factory;
	registration->serviceFactory = factory;
	registration->isServiceFactory = true;

	void *actual = NULL;
	celix_status_t status = serviceRegistration_getService(registration, bundle, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(0x20, actual);
}

TEST(service_registration, getProperties) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	properties_pt properties = (properties_pt) 0x10;
	registration->properties = properties;

	properties_pt actual = NULL;
	celix_status_t status = serviceRegistration_getProperties(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(properties, actual);
}

TEST(service_registration, getPropertiesIllegalArgument) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->properties = NULL;

	properties_pt actual = (properties_pt) 0x01;
	celix_status_t status = serviceRegistration_getProperties(registration, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(service_registration, getRegistry) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	service_registry_pt registry = (service_registry_pt) 0x10;
	registration->registry = registry;

	service_registry_pt actual = NULL;
	celix_status_t status = serviceRegistration_getRegistry(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(registry, actual);
}

TEST(service_registration, getRegistryIllegalArgument) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->registry = NULL;

	service_registry_pt actual = (service_registry_pt) 0x01;
	celix_status_t status = serviceRegistration_getRegistry(registration, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(service_registration, getServiceReferences) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	array_list_pt references = (array_list_pt) 0x10;
	registration->references = references;

	array_list_pt actual = NULL;
	celix_status_t status = serviceRegistration_getServiceReferences(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(references, actual);
}

TEST(service_registration, getServiceReferencesIllegalArgument) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->registry = NULL;

	array_list_pt actual = (array_list_pt) 0x01;
	celix_status_t status = serviceRegistration_getServiceReferences(registration, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(service_registration, getServiceName) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	std::string serviceName = "service";
	registration->className = (char *) serviceName.c_str();

	char *actual = NULL;
	celix_status_t status = serviceRegistration_getServiceName(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	STRCMP_EQUAL("service", actual);
}

TEST(service_registration, getServiceNameIllegalArgument) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->className = NULL;

	char *actual = (char *) 0x01;
	celix_status_t status = serviceRegistration_getServiceName(registration, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST(service_registration, getBundle) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	bundle_pt bundle = (bundle_pt) 0x10;
	registration->bundle = bundle;

	bundle_pt actual = NULL;
	celix_status_t status = serviceRegistration_getBundle(registration, &actual);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(bundle, actual);
}

TEST(service_registration, getBundleIllegalArgument) {
	service_registration_pt registration = (service_registration_pt) apr_palloc(pool, sizeof(*registration));
	registration->bundle = NULL;

	bundle_pt actual = (bundle_pt) 0x01;
	celix_status_t status = serviceRegistration_getBundle(registration, &actual);
	LONGS_EQUAL(CELIX_ILLEGAL_ARGUMENT, status);
}
