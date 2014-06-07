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
 * service_registry_test.c
 *
 *  \date       Feb 7, 2013
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
#include "service_registry_private.h"
#include "celix_log.h"

framework_logger_pt logger;
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(service_registry) {
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

extern "C" {
void serviceRegistryTest_serviceChanged(framework_pt framework, service_event_type_e eventType, service_registration_pt registration, properties_pt oldprops) {
}
}

TEST(service_registry, create) {
	framework_pt framework = (framework_pt) 0x10;
	service_registry_pt registry = NULL;

	serviceRegistry_create(framework, serviceRegistryTest_serviceChanged, &registry);

	POINTERS_EQUAL(framework, registry->framework);
	POINTERS_EQUAL(serviceRegistryTest_serviceChanged, registry->serviceChanged);
	LONGS_EQUAL(1l, registry->currentServiceId);
	CHECK(registry->inUseMap != NULL);
	CHECK(registry->listenerHooks != NULL);
	//CHECK(registry->mutex != NULL);
	CHECK(registry->serviceReferences == NULL);
	CHECK(registry->serviceRegistrations != NULL);
}

TEST(service_registry, getRegisteredServices) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	service_registration_pt reg = (service_registration_pt) 0x10;
	arrayList_add(registrations, reg);
	bundle_pt bundle = (bundle_pt) 0x20;
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	service_reference_pt ref = (service_reference_pt) 0x30;

	array_list_pt refs = NULL;
	arrayList_create(&refs);

	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", reg)
		.andReturnValue(true);
	mock()
		.expectOneCall("serviceRegistration_getBundle")
		.withParameter("registration", reg)
		.andOutputParameter("bundle", bundle)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceReference_create")
		.withParameter("bundle", bundle)
		.withParameter("registration", reg)
		.andOutputParameter("reference", ref)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", reg)
		.andOutputParameter("references", refs)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", ref)
		.andOutputParameter("registration", reg)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", reg)
		.andOutputParameter("references", refs)
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt services = NULL;
	serviceRegistry_getRegisteredServices(registry, bundle, &services);
	LONGS_EQUAL(1, arrayList_size(services));
	POINTERS_EQUAL(ref, arrayList_get(services, 0));
}

TEST(service_registry, getServicesInUse) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);

	array_list_pt usages = NULL;
	arrayList_create(&usages);
	bundle_pt bundle = (bundle_pt) 0x10;
	service_reference_pt ref = (service_reference_pt) 0x20;
	usage_count_pt usage = (usage_count_pt) apr_palloc(pool, sizeof(*usage));
	usage->reference = ref;
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);

	array_list_pt inUse = NULL;
	serviceRegistry_getServicesInUse(registry, bundle, &inUse);
	LONGS_EQUAL(1, arrayList_size(inUse));
	POINTERS_EQUAL(ref, arrayList_get(inUse, 0));
}

TEST(service_registry, registerServiceNoProps) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	std::string serviceName = "service";
	void *service = (void *) 0x20;
	service_registration_pt reg = (service_registration_pt) 0x30;

	mock()
		.expectOneCall("bundle_getMemoryPool")
		.withParameter("bundle", bundle)
		.andOutputParameter("pool", pool)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_create")
		.withParameter("pool", pool)
		.withParameter("registry", registry)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", (char *) serviceName.c_str())
		.withParameter("serviceId", 2)
		.withParameter("serviceObject", service)
		.withParameter("dictionary", (void *) NULL)
		.andReturnValue(reg);

	service_registration_pt registration = NULL;
	serviceRegistry_registerService(registry, bundle, (char *) serviceName.c_str(), service, NULL, &registration);
	POINTERS_EQUAL(reg, registration);
}

TEST(service_registry, registerServiceFactoryNoProps) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	std::string serviceName = "service";
	service_factory_pt factory = (service_factory_pt) 0x20;
	service_registration_pt reg = (service_registration_pt) 0x30;

	mock()
		.expectOneCall("bundle_getMemoryPool")
		.withParameter("bundle", bundle)
		.andOutputParameter("pool", pool)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_createServiceFactory")
		.withParameter("pool", pool)
		.withParameter("registry", registry)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", (char *) serviceName.c_str())
		.withParameter("serviceId", 2)
		.withParameter("serviceObject", factory)
		.withParameter("dictionary", (void *) NULL)
		.andReturnValue(reg);

	service_registration_pt registration = NULL;
	serviceRegistry_registerServiceFactory(registry, bundle, (char *) serviceName.c_str(), factory, NULL, &registration);
	POINTERS_EQUAL(reg, registration);
}

TEST(service_registry, unregisterService) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;

	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	framework_pt framework = (framework_pt) 0x50;
	registry->framework = framework;

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.andOutputParameter("properties", properties)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("properties_get")
		.withParameter("properties", properties)
		.withParameter("key", "objectClass")
		.andReturnValue("test");
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_invalidate")
		.withParameter("reference", reference)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_invalidate")
		.withParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);

	serviceRegistry_unregisterService(registry, bundle, registration);
}

TEST(service_registry, unregisterServices) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);
	mock()
		.expectOneCall("serviceRegistration_unregister")
		.withParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
	serviceRegistry_unregisterServices(registry, bundle);
	LONGS_EQUAL(0, hashMap_size(registry->serviceRegistrations));
}

TEST(service_registry, getServiceReferences) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.andOutputParameter("properties", properties)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_getServiceName")
		.withParameter("registration", registration)
		.andOutputParameter("serviceName", "test")
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);
	mock()
		.expectOneCall("serviceRegistration_getBundle")
		.withParameter("registration", registration)
		.andOutputParameter("bundle", bundle)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceReference_create")
		.withParameter("bundle", bundle)
		.withParameter("registration", registration)
		.andOutputParameter("reference", reference)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt actual  = NULL;
	serviceRegistry_getServiceReferences(registry, "test", NULL, &actual);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(reference, arrayList_get(actual, 0));
}

TEST(service_registry, getService) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	module_pt module = (module_pt) 0x50;

	void * service = (void *) 0x60;

	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);
	mock()
		.expectOneCall("bundle_getCurrentModule")
		.withParameter("bundle", bundle)
		.andOutputParameter("module", module)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_getService")
		.withParameter("registration", registration)
		.withParameter("bundle", bundle)
		.andOutputParameter("service", service)
		.andReturnValue(true);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);

	void *actual = NULL;
	serviceRegistry_getService(registry, bundle, reference, &actual);
	POINTERS_EQUAL(service, actual);
}

TEST(service_registry, ungetService) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	array_list_pt usages = NULL;
	arrayList_create(&usages);
	usage_count_pt usage = (usage_count_pt) malloc(sizeof(*usage));
	usage->reference = reference;
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);

	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);

	bool result = false;
	serviceRegistry_ungetService(registry, bundle, reference, &result);
	LONGS_EQUAL(1, result);
}

TEST(service_registry, ungetServivces) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	array_list_pt usages = NULL;
	arrayList_create(&usages);
	usage_count_pt usage = (usage_count_pt) malloc(sizeof(*usage));
	usage->reference = reference;
	usage->count = 1;
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);

	mock()
		.expectOneCall("bundle_getMemoryPool")
		.withParameter("bundle", bundle)
		.andOutputParameter("pool", pool)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
//	mock()
//		.expectOneCall("serviceReference_equals")
//		.withParameter("reference", reference)
//		.withParameter("compareTo", reference)
//		.andOutputParameter("equal", true)
//		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);
	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);

	serviceRegistry_ungetServices(registry, bundle);
}

TEST(service_registry, getUsingBundles) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
    celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	array_list_pt usages = NULL;
	arrayList_create(&usages);
	usage_count_pt usage = (usage_count_pt) malloc(sizeof(*usage));
	usage->reference = reference;
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);

	array_list_pt actual = serviceRegistry_getUsingBundles(registry, reference);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(bundle, arrayList_get(actual, 0));
}

TEST(service_registry, createServiceReference) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	celixThreadMutexAttr_create(&registry->mutexAttr);
    celixThreadMutexAttr_settype(&registry->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE);
	celixThreadMutex_create(&registry->mutex, &registry->mutexAttr);
	registry->currentServiceId = 1l;
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
//	array_list_pt registrations = NULL;
//	arrayList_create(&registrations);
//	arrayList_add(registrations, registration);
//
//	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	mock()
		.expectOneCall("serviceRegistration_getBundle")
		.withParameter("registration", registration)
		.andOutputParameter("bundle", bundle)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceReference_create")
		.withParameter("bundle", bundle)
		.withParameter("registration", registration)
		.andOutputParameter("reference", reference)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);

	service_reference_pt actual  = NULL;
	serviceRegistry_createServiceReference(registry, registration, &actual);
	POINTERS_EQUAL(reference, actual);
}

TEST(service_registry, getListenerHooks) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	registry->listenerHooks = NULL;
	arrayList_create(&registry->listenerHooks);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	arrayList_add(registry->listenerHooks, registration);

	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	mock()
		.expectOneCall("serviceRegistration_getBundle")
		.withParameter("registration", registration)
		.andOutputParameter("bundle", bundle)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceReference_create")
		.withParameter("bundle", bundle)
		.withParameter("registration", registration)
		.andOutputParameter("reference", reference)
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.andOutputParameter("registration", registration)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_getServiceReferences")
		.withParameter("registration", registration)
		.andOutputParameter("references", references)
		.andReturnValue(CELIX_SUCCESS);

	array_list_pt hooks = NULL;
	celix_status_t status = serviceRegistry_getListenerHooks(registry, &hooks);
	LONGS_EQUAL(1, arrayList_size(hooks));
	POINTERS_EQUAL(reference, arrayList_get(hooks, 0));
}

TEST(service_registry, servicePropertiesModified) {
	service_registry_pt registry = (service_registry_pt) apr_palloc(pool, sizeof(*registry));
	service_registration_pt registration = (service_registration_pt) 0x10;
	properties_pt properties = (properties_pt) 0x20;

	serviceRegistry_servicePropertiesModified(registry, registration, properties);
}

