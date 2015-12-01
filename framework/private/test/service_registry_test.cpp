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
 * service_registry_test.cpp
 *
 *  \date       Feb 7, 2013
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
#include "CppUTestExt/MockSupport_c.h"

extern "C" {
#include "service_registry_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;

void serviceRegistryTest_serviceChanged(framework_pt framework, service_event_type_e eventType, service_registration_pt registration, properties_pt oldprops) {
	mock_c()->actualCall("serviceRegistryTest_serviceChanged")
			->withPointerParameters("framework", framework)
			->withIntParameters("eventType", eventType)
			->withPointerParameters("registration", registration)
			->withPointerParameters("oldprops", oldprops);
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

static int registry_callback_t_isEqual(const void* object1, const void* object2)
{
	registry_callback_t callback1 = *(registry_callback_t*) object1;
	registry_callback_t callback2 = *(registry_callback_t*) object2;
	return 	callback1.getUsingBundles == callback2.getUsingBundles &&
       		callback1.handle == callback2.handle &&
       		callback1.modified == callback2.modified &&
       		callback1.unregister == callback2.unregister;
}

static char * registry_callback_t_toString(const void* object)
{
	char buff[512];
	registry_callback_t callback = *(registry_callback_t*) object;
	snprintf(buff, 512, "< getUsingBudles: %p, handle: %p, modified: %p, unregister: %p >", callback.getUsingBundles, callback.handle, callback.modified, callback.unregister);

	return my_strdup(buff);
}
}

int main(int argc, char** argv) {
	mock_c()->installComparator("registry_callback_t", registry_callback_t_isEqual, registry_callback_t_toString);
	int ret = RUN_ALL_TESTS(argc, argv);
	mock_c()->removeAllComparators();
	return ret;
}

TEST_GROUP(service_registry) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(service_registry, create) {
	framework_pt framework = (framework_pt) 0x10;
	service_registry_pt registry = NULL;

	serviceRegistry_create(framework, serviceRegistryTest_serviceChanged, &registry);

	POINTERS_EQUAL(framework, registry->framework);
	POINTERS_EQUAL(serviceRegistryTest_serviceChanged, registry->serviceChanged);
	LONGS_EQUAL(1l, registry->currentServiceId);
	CHECK(registry->listenerHooks != NULL);
	//CHECK(registry->mutex != NULL);
	CHECK(registry->serviceReferences != NULL);
	CHECK(registry->serviceRegistrations != NULL);

	serviceRegistry_destroy(registry);
}

TEST(service_registry, getRegisteredServices) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	service_registration_pt reg = (service_registration_pt) 0x10;
	arrayList_add(registrations, reg);
	bundle_pt bundle = (bundle_pt) 0x20;
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	hash_map_pt usages = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt ref = (service_reference_pt) 0x30;
	hashMap_put(usages, reg, ref);
	hashMap_put(registry->serviceReferences, bundle, usages);

	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", reg)
		.andReturnValue(true);
	mock()
		.expectOneCall("serviceReference_retain")
		.withParameter("ref", ref);

	array_list_pt services = NULL;
	serviceRegistry_getRegisteredServices(registry, bundle, &services);
	LONGS_EQUAL(1, arrayList_size(services));
	POINTERS_EQUAL(ref, arrayList_get(services, 0));

	arrayList_destroy(services);
	arrayList_destroy(registrations);
	hashMap_remove(registry->serviceRegistrations, bundle);
	serviceRegistry_destroy(registry);
	hashMap_destroy(usages, false, false);
}

TEST(service_registry, getServicesInUse) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	hash_map_pt usages = hashMap_create(NULL, NULL, NULL, NULL);
	bundle_pt bundle = (bundle_pt) 0x10;
	service_reference_pt ref = (service_reference_pt) 0x20;
	service_registration_pt reg = (service_registration_pt) 0x30;
	hashMap_put(usages, reg, ref);
	hashMap_put(registry->serviceReferences, bundle, usages);

	array_list_pt inUse = NULL;
	serviceRegistry_getServicesInUse(registry, bundle, &inUse);
	LONGS_EQUAL(1, arrayList_size(inUse));
	POINTERS_EQUAL(ref, arrayList_get(inUse, 0));

	arrayList_destroy(inUse);
	serviceRegistry_destroy(registry);
	hashMap_destroy(usages, false, false);
}

TEST(service_registry, registerServiceNoProps) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	char * serviceName = my_strdup("service");
	void *service = (void *) 0x20;
	service_registration_pt reg = (service_registration_pt) 0x30;

	mock()
		.expectOneCall("serviceRegistration_create")
		.withParameterOfType("registry_callback_t", "callback", &registry->callback)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("serviceId", 2)
		.withParameter("serviceObject", service)
		.withParameter("dictionary", (void *) NULL)
		.andReturnValue(reg);

	mock().expectOneCall("serviceRegistryTest_serviceChanged")
			.withParameter("framework", framework)
			.withParameter("eventType", OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED)
			.withParameter("registration", reg)
			.withParameter("oldprops", (void*)NULL);

	service_registration_pt registration = NULL;
	serviceRegistry_registerService(registry, bundle, serviceName, service, NULL, &registration);
	POINTERS_EQUAL(reg, registration);

	array_list_pt destroy_this = (array_list_pt) hashMap_remove(registry->serviceRegistrations, bundle);
	arrayList_destroy(destroy_this);
	serviceRegistry_destroy(registry);
	free(serviceName);
}

TEST(service_registry, registerServiceFactoryNoProps) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	char * serviceName = my_strdup("service");
	service_factory_pt factory = (service_factory_pt) malloc(sizeof(*factory));
	factory->factory = (void*) 0x20;
	service_registration_pt reg = (service_registration_pt) 0x40;

	mock()
		.expectOneCall("serviceRegistration_createServiceFactory")
		.withParameterOfType("registry_callback_t", "callback", &registry->callback)
		.withParameter("bundle", bundle)
		.withParameter("serviceName", serviceName)
		.withParameter("serviceId", 2)
		.withParameter("serviceObject", factory)
		.withParameter("dictionary", (void *) NULL)
		.andReturnValue(reg);

	mock().expectOneCall("serviceRegistryTest_serviceChanged")
		.withParameter("framework", framework)
		.withParameter("eventType", OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED)
		.withParameter("registration", reg)
		.withParameter("oldprops", (void*)NULL);

	service_registration_pt registration = NULL;
	serviceRegistry_registerServiceFactory(registry, bundle, serviceName, factory, NULL, &registration);
	POINTERS_EQUAL(reg, registration);

	array_list_pt destroy_this = (array_list_pt) hashMap_remove(registry->serviceRegistrations, bundle);
	arrayList_destroy(destroy_this);
	serviceRegistry_destroy(registry);
	free(serviceName);
	free(factory);
}

TEST(service_registry, unregisterService) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);
	properties_pt properties = (properties_pt) 0x30;

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.withOutputParameterReturning("properties", &properties, sizeof(properties))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("properties_get")
		.withParameter("properties", properties)
		.withParameter("key", "objectClass")
		.andReturnValue("test");

	mock()
		.expectOneCall("serviceRegistryTest_serviceChanged")
		.withParameter("framework", framework)
		.withParameter("eventType", OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING)
		.withParameter("registration", registration)
		.withParameter("oldprops", (void*) NULL);

		mock()
		.expectOneCall("serviceRegistration_invalidate")
		.withParameter("registration", registration);

	mock()
		.expectOneCall("serviceRegistration_release")
		.withParameter("registration", registration);



	serviceRegistry_unregisterService(registry, bundle, registration);

	serviceRegistry_destroy(registry);
}

/*TEST(service_registry, unregisterServices) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework, NULL, &registry);

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
	serviceRegistry_unregisterService(registry, bundle, );
	LONGS_EQUAL(0, hashMap_size(registry->serviceRegistrations));

	free(registry);
}*/

/*TEST(service_registry, getServiceReferencesForRegistration){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);
	registry->serviceReferences = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	serviceRegistry_getServiceReferencesForRegistration(registry, registration, &references);
}*/

TEST(service_registry, getServiceReferences) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;

	hash_map_pt usages = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt reference = (service_reference_pt) 0x30;
	hashMap_put(usages, registration, reference);
	hashMap_put(registry->serviceReferences, bundle, usages);

	mock()
		.expectOneCall("serviceRegistration_retain")
		.withParameter("registration", registration);

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.withOutputParameterReturning("properties", &properties, sizeof(properties))
		.andReturnValue(CELIX_SUCCESS);
	char *serviceName = (char *) "test";
	mock()
		.expectOneCall("serviceRegistration_getServiceName")
		.withParameter("registration", registration)
		.withOutputParameterReturning("serviceName", &serviceName, sizeof(serviceName))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);

	mock()
		.expectOneCall("serviceReference_retain")
		.withParameter("ref", reference);
	/*mock()
		.expectOneCall("serviceRegistration_getBundle")
		.withParameter("registration", registration)
		.withOutputParameterReturning("bundle", &bundle, sizeof(bundle))
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();

	mock()
		.expectOneCall("serviceReference_create")
		.withParameter("referenceOwner", bundle)
		.withParameter("registration", registration)
		.withOutputParameterReturning("reference", &reference, sizeof(reference))
		.ignoreOtherParameters();*/


	mock()
		.expectOneCall("serviceRegistration_release")
		.withParameter("registration", registration);

	array_list_pt actual  = NULL;

	serviceRegistry_getServiceReferences(registry, bundle, "test", NULL, &actual);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(reference, arrayList_get(actual, 0));

	hashMap_destroy(usages, false, false);
	arrayList_destroy(actual);
	arrayList_destroy(registrations);
	hashMap_remove(registry->serviceRegistrations, bundle);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, getService) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	service_reference_pt reference = (service_reference_pt) 0x40;

	hashMap_put(registry->deletedServiceReferences, reference, (void*) false);

	void * service = (void *) 0x60;

	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.withOutputParameterReturning("registration", &registration, sizeof(registration))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);

	int count = 1;
	mock()
		.expectOneCall("serviceReference_increaseUsage")
		.withParameter("reference", reference)
		.withOutputParameterReturning("updatedCount", &count, sizeof(count));

	mock()
		.expectOneCall("serviceRegistration_getService")
		.withParameter("registration", registration)
		.withParameter("bundle", bundle)
		.withOutputParameterReturning("service", &service, sizeof(service))
		.andReturnValue(true);

	mock()
		.expectOneCall("serviceReference_setService")
		.withParameter("ref", reference)
		.withParameter("service", service);

	mock()
		.expectOneCall("serviceReference_getService")
		.withParameter("reference", reference)
		.withOutputParameterReturning("service", &service, sizeof(service));

	void *actual = NULL;
	serviceRegistry_getService(registry, bundle, reference, &actual);
	POINTERS_EQUAL(service, actual);

	hashMap_remove(registry->serviceRegistrations, bundle);
	arrayList_destroy(registrations);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, ungetService) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	service_reference_pt reference = (service_reference_pt) 0x40;

	array_list_pt usages = NULL;
	arrayList_create(&usages);
	hashMap_put(registry->serviceReferences, bundle, reference);
	hashMap_put(registry->deletedServiceReferences, reference, (void*) false);
	void * service = (void*) 0x50;

	int count = 0;
	mock()
		.expectOneCall("serviceReference_decreaseUsage")
		.withParameter("ref", reference)
		.withOutputParameterReturning("updatedCount", &count, sizeof(count));

	mock()
		.expectOneCall("serviceReference_getService")
		.withParameter("reference", reference)
		.withOutputParameterReturning("service", &service, sizeof(service));

	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.withOutputParameterReturning("registration", &registration, sizeof(registration));

	mock()
		.expectOneCall("serviceRegistration_ungetService")
		.withParameter("registration", registration)
		.withParameter("bundle", bundle)
		.withOutputParameterReturning("service", &service, sizeof(service));

/*	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);*/

	bool result = false;
	celix_status_t status;
	status = serviceRegistry_ungetService(registry, bundle, reference, &result);
	LONGS_EQUAL(CELIX_SUCCESS, status)
	LONGS_EQUAL(true, result);

	hashMap_remove(registry->deletedServiceReferences, reference);
	hashMap_put(registry->deletedServiceReferences, reference, (void*) true);

	mock()
		.expectOneCall("framework_log");

	status = serviceRegistry_ungetService(registry, bundle, reference, &result);
	LONGS_EQUAL(CELIX_BUNDLE_EXCEPTION, status);


	arrayList_destroy(registrations);
	hashMap_remove(registry->serviceRegistrations, bundle);
	serviceRegistry_destroy(registry);
	arrayList_destroy(usages);
}

/*TEST(service_registry, ungetServivces) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

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
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.withOutputParameterReturning("registration", &registration, sizeof(registration))
		.andReturnValue(CELIX_SUCCESS);

	bool out = true;
	mock()
			.expectOneCall("serviceReference_equals")
			.withParameter("reference", reference)
			.withParameter("compareTo", reference)
			.withOutputParameterReturning("equal", &out, sizeof(out))
			.andReturnValue(CELIX_SUCCESS);

	out =  true;
	mock()
			.expectOneCall("serviceReference_equals")
			.withParameter("reference", reference)
			.withParameter("compareTo", reference)
			.withOutputParameterReturning("equal", &out, sizeof(out))
			.andReturnValue(CELIX_SUCCESS);

	mock().expectOneCall("serviceRegistration_ungetService")
			.withParameter("registration", registration)
			.withParameter("bundle", bundle)
			.withOutputParameterReturning("service", &reference, sizeof(reference));

	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);

	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.withOutputParameterReturning("registration", &registration, sizeof(registration))
		.andReturnValue(CELIX_SUCCESS);

	serviceRegistry_ungetServices(registry, bundle);

	free(registry);
}*/

/*TEST(service_registry, getUsingBundles) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
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
	hashMap_put(registry->serviceReferences, bundle, usages);

	bool out = true;
	mock()
			.expectOneCall("serviceReference_equals")
			.withParameter("reference", reference)
			.withParameter("compareTo", reference)
			.withOutputParameterReturning("equal", &out, sizeof(out))
			.andReturnValue(CELIX_SUCCESS);

	array_list_pt actual = serviceRegistry_getUsingBundles(registry, reference);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(bundle, arrayList_get(actual, 0));

	free(registry);
}*/

/*TEST(service_registry, createServiceReference) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;*/
	/*array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);

	hashMap_put(registry->serviceRegistrations, bundle, registrations);*/

/*	array_list_pt references = NULL;
	arrayList_create(&references);
	service_reference_pt reference = (service_reference_pt) 0x40;
	arrayList_add(references, reference);

	mock()
		.expectOneCall("serviceRegistration_getBundle")
		.withParameter("registration", registration)
		.withOutputParameterReturning("bundle", &bundle, sizeof(bundle))
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("serviceReference_create")
		.withParameter("bundle", bundle)
		.withParameter("registration", registration)
		.withOutputParameterReturning("reference", &reference, sizeof(reference))
		.andReturnValue(CELIX_SUCCESS)
		.ignoreOtherParameters();

	service_reference_pt actual  = NULL;
	serviceRegistry_createServiceReference(registry, NULL, registration, &actual);
	POINTERS_EQUAL(reference, actual);

	free(registry);
}*/

TEST(service_registry, getListenerHooks) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) 0x20;
	arrayList_add(registry->listenerHooks, registration);

	hash_map_pt usages = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt reference = (service_reference_pt) 0x30;
	hashMap_put(usages, registration, reference);
	hashMap_put(registry->serviceReferences, bundle, usages);

	mock()
		.expectOneCall("serviceRegistration_retain")
		.withParameter("registration", registration);
	mock()
		.expectOneCall("serviceReference_retain")
		.withParameter("ref", reference);
	mock()
		.expectOneCall("serviceRegistration_release")
		.withParameter("registration", registration);

	array_list_pt hooks = NULL;
	serviceRegistry_getListenerHooks(registry, bundle, &hooks);
	LONGS_EQUAL(1, arrayList_size(hooks));
	POINTERS_EQUAL(reference, arrayList_get(hooks, 0));

	hashMap_destroy(usages, false, false);
	arrayList_destroy(hooks);
	arrayList_remove(registry->listenerHooks, 0);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, servicePropertiesModified) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	service_registration_pt registration = (service_registration_pt) 0x02;
	properties_pt properties = (properties_pt) 0x03;

	mock().expectOneCall("serviceRegistryTest_serviceChanged")
		.withParameter("framework", registry->framework)
		.withParameter("eventType", OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED)
		.withParameter("registration", registration)
		.withParameter("oldprops", properties);

	serviceRegistry_servicePropertiesModified(registry, registration, properties);

	serviceRegistry_destroy(registry);
}
