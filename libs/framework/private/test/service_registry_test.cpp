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
#include "constants.h"
#include "listener_hook_service.h"
#include "service_registry.h"
#include "service_registry_private.h"
#include "service_registration_private.h"
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

static const char * registry_callback_t_toString(const void* object)
{
	char buff[512];
	registry_callback_t callback = *(registry_callback_t*) object;
	snprintf(buff, 512, "getUsingBudles: %p, handle: %p, modified: %p, unregister: %p", callback.getUsingBundles, callback.handle, callback.modified, callback.unregister);

	return my_strdup(buff);
}
}

int main(int argc, char** argv) {
	mock_c()->installComparator("registry_callback_t", registry_callback_t_isEqual, registry_callback_t_toString);
	int ret = RUN_ALL_TESTS(argc, argv);
	mock_c()->removeAllComparatorsAndCopiers();
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
	UNSIGNED_LONGS_EQUAL(1UL, registry->currentServiceId);
	CHECK(registry->listenerHooks != NULL);
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
	service_registration_pt reg = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	reg->serviceId = 10UL;
	arrayList_add(registrations, reg);
	bundle_pt bundle = (bundle_pt) 0x20;
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	hash_map_pt usages = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt ref = (service_reference_pt) 0x30;
	hashMap_put(usages, (void*)reg->serviceId, ref);
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
	free(reg);
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
	factory->handle = (void*) 0x20;
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

TEST(service_registry, registerServiceListenerHook) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	char * serviceName = my_strdup(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME);
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
	LONGS_EQUAL(1, arrayList_size(registry->listenerHooks));
	POINTERS_EQUAL(reg, arrayList_get(registry->listenerHooks, 0));

	//cleanup
	array_list_pt destroy_this = (array_list_pt) hashMap_remove(registry->serviceRegistrations, bundle);
	arrayList_destroy(destroy_this);
	arrayList_remove(registry->listenerHooks, 0);
	serviceRegistry_destroy(registry);
	free(serviceName);
}

TEST(service_registry, unregisterService) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	registration->serviceId = 20UL;

	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);
	hashMap_put(registry->serviceRegistrations, bundle, registrations);
	service_reference_pt reference = (service_reference_pt) 0x30;
	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);

	hashMap_put(references, (void*)registration->serviceId, reference);
	hashMap_put(registry->serviceReferences, bundle, references);
	properties_pt properties = (properties_pt) 0x40;

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.withOutputParameterReturning("properties", &properties, sizeof(properties))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("properties_get")
		.withParameter("properties", properties)
		.withParameter("key", (char *)OSGI_FRAMEWORK_OBJECTCLASS)
		.andReturnValue((char*)OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME);

	mock()
		.expectOneCall("serviceRegistryTest_serviceChanged")
		.withParameter("framework", framework)
		.withParameter("eventType", OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING)
		.withParameter("registration", registration)
		.withParameter("oldprops", (void*) NULL);
	mock()
		.expectOneCall("serviceReference_invalidate")
		.withParameter("reference", reference);

		mock()
		.expectOneCall("serviceRegistration_invalidate")
		.withParameter("registration", registration);

	mock()
		.expectOneCall("serviceRegistration_release")
		.withParameter("registration", registration);



	serviceRegistry_unregisterService(registry, bundle, registration);
	hashMap_destroy(references, false,false);
	free(registration);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, clearServiceRegistrations){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	service_registration_pt reg = (service_registration_pt) 0x10;
	arrayList_add(registrations, reg);
	bundle_pt bundle = (bundle_pt) 0x20;
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	//removing the first registration
	char *serviceName = (char *) "test_service";
	mock()
		.expectOneCall("serviceRegistration_getServiceName")
		.withParameter("registration", reg)
		.withOutputParameterReturning("serviceName", &serviceName, sizeof(serviceName))
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("framework_log");

	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", reg)
		.andReturnValue(true);

	//this call normally removes the registration from registry->serviceRegistrations
	//but it remains since the mock does not call the callback->unregister therefore it acts like 2 separate registrations
	mock().expectOneCall("serviceRegistration_unregister")
			.withParameter("registration", reg);

	//removing the "second" registration
	mock()
		.expectOneCall("serviceRegistration_getServiceName")
		.withParameter("registration", reg)
		.withOutputParameterReturning("serviceName", &serviceName, sizeof(serviceName))
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("framework_log");

	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", reg)
		.andReturnValue(false);

	serviceRegistry_clearServiceRegistrations(registry, bundle);

	//clean up
	hashMap_remove(registry->serviceRegistrations, bundle);
	arrayList_destroy(registrations);

	serviceRegistry_destroy(registry);
}

TEST(service_registry, getServiceReference){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	registration->serviceId = 20UL;
	service_reference_pt reference = (service_reference_pt) 0x50;

	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references, (void*)registration->serviceId, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	mock().expectOneCall("serviceReference_retain")
			.withParameter("ref", reference);

	service_reference_pt get_reference;
	celix_status_t status = serviceRegistry_getServiceReference(registry, bundle, registration, &get_reference);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(reference, get_reference);

	hashMap_destroy(references, false, false);
	free(registration);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, getServiceReference_unknownRef){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	registration->serviceId = 20UL;
	service_reference_pt reference = (service_reference_pt) 0x50;

	//test getting ref from bundle without refs
	mock().expectOneCall("serviceRegistration_getBundle")
				.withParameter("registration", registration)
				.withOutputParameterReturning("bundle", &bundle, sizeof(bundle));

	mock().expectOneCall("serviceReference_create")
			.withParameter("referenceOwner", bundle)
			.withParameter("registration", registration)
			.withOutputParameterReturning("reference", &reference, sizeof(reference))
			.ignoreOtherParameters();


	service_reference_pt get_reference;
	celix_status_t status = serviceRegistry_getServiceReference(registry, bundle, registration, &get_reference);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(reference, get_reference);

	//cleanup
	hash_map_pt del = (hash_map_pt) hashMap_remove(registry->serviceReferences, bundle);
	hashMap_destroy(del, false, false);
	free(registration);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, getServiceReferences) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	registration->serviceId = 20UL;

	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;
	filter_pt filter = (filter_pt) 0x40;

	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt reference = (service_reference_pt) 0x50;
	hashMap_put(references, (void*)registration->serviceId, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	mock()
		.expectOneCall("serviceRegistration_retain")
		.withParameter("registration", registration);

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.withOutputParameterReturning("properties", &properties, sizeof(properties))
		.andReturnValue(CELIX_SUCCESS);
	bool matchResult = true;
	mock().expectNCalls(2, "filter_match")
		.withParameter("filter", filter)
		.withParameter("properties", properties)
		.withOutputParameterReturning("result", &matchResult, sizeof(matchResult));
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

	mock()
		.expectOneCall("serviceRegistration_release")
		.withParameter("registration", registration);

	array_list_pt actual  = NULL;

	serviceRegistry_getServiceReferences(registry, bundle, "test", filter, &actual);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(reference, arrayList_get(actual, 0));

	hashMap_destroy(references, false, false);
	arrayList_destroy(actual);
	arrayList_destroy(registrations);
	hashMap_remove(registry->serviceRegistrations, bundle);
	free(registration);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, getServiceReferences_noFilterOrName) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	registration->serviceId = 20UL;

	array_list_pt registrations = NULL;
	arrayList_create(&registrations);
	arrayList_add(registrations, registration);
	hashMap_put(registry->serviceRegistrations, bundle, registrations);

	properties_pt properties = (properties_pt) 0x30;

	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt reference = (service_reference_pt) 0x50;
	hashMap_put(references, (void*)registration->serviceId, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	mock()
		.expectOneCall("serviceRegistration_retain")
		.withParameter("registration", registration);

	mock()
		.expectOneCall("serviceRegistration_getProperties")
		.withParameter("registration", registration)
		.withOutputParameterReturning("properties", &properties, sizeof(properties))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(true);

	mock()
		.expectOneCall("serviceReference_retain")
		.withParameter("ref", reference);

	mock()
		.expectOneCall("serviceRegistration_release")
		.withParameter("registration", registration);

	array_list_pt actual  = NULL;

	serviceRegistry_getServiceReferences(registry, bundle, NULL, NULL, &actual);
	LONGS_EQUAL(1, arrayList_size(actual));
	POINTERS_EQUAL(reference, arrayList_get(actual, 0));

	hashMap_destroy(references, false, false);
	arrayList_destroy(actual);
	arrayList_destroy(registrations);
	hashMap_remove(registry->serviceRegistrations, bundle);
	free(registration);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, retainServiceReference){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	service_reference_pt reference = (service_reference_pt) 0x10;
	bundle_pt bundle = (bundle_pt) 0x20;
	bundle_pt bundle2 = (bundle_pt) 0x30;

	//test unknown reference (reference not present in registry->deletedServiceReferences)
	mock().expectOneCall("framework_log");

	serviceRegistry_retainServiceReference(registry, bundle, reference);

	//test known reference, but owner != bundle
	registry->checkDeletedReferences = false;

	mock().expectOneCall("serviceReference_getOwner")
			.withParameter("reference", reference)
			.withOutputParameterReturning("owner", &bundle2, sizeof(bundle2));
	mock().expectOneCall("framework_log");

	serviceRegistry_retainServiceReference(registry, bundle, reference);

	registry->checkDeletedReferences = true;
	//test known reference, with owner == bundle
	hashMap_put(registry->deletedServiceReferences, reference, (void*) false);
	mock().expectOneCall("serviceReference_getOwner")
			.withParameter("reference", reference)
			.withOutputParameterReturning("owner", &bundle, sizeof(bundle));
	mock().expectOneCall("serviceReference_retain")
			.withParameter("ref", reference);

	serviceRegistry_retainServiceReference(registry, bundle, reference);

	//cleanup
	hashMap_remove(registry->deletedServiceReferences, reference);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, ungetServiceReference){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	service_registration_pt registration = (service_registration_pt) 0x10;
	service_registration_pt registration2 = (service_registration_pt) 0x20;
	service_registration_pt registration3 = (service_registration_pt) 0x30;
	service_reference_pt reference = (service_reference_pt) 0x40;
	service_reference_pt reference2 = (service_reference_pt) 0x50;
	service_reference_pt reference3 = (service_reference_pt) 0x60;
	bundle_pt bundle = (bundle_pt) 0x70;
	bundle_pt bundle2 = (bundle_pt) 0x80;
    module_pt module = (module_pt) 0x90;

	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references, registration, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	hash_map_pt references2 = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references2, registration3, reference3);
	hashMap_put(registry->serviceReferences, bundle2, references2);

	//test unknown reference (reference not present in registry->deletedServiceReferences)
	mock().expectOneCall("framework_log");

	serviceRegistry_ungetServiceReference(registry, bundle, reference);

	//test known reference, but destroyed == false
	size_t count = 0;
	bool destroyed = false;
	hashMap_put(registry->deletedServiceReferences, reference, (void*) false);

	mock().expectOneCall("serviceReference_getUsageCount")
			.withParameter("reference", reference)
			.withOutputParameterReturning("count", &count, sizeof(count));
	mock().expectOneCall("serviceReference_release")
			.withParameter("ref", reference)
			.withOutputParameterReturning("destroyed", &destroyed, sizeof(destroyed));

	serviceRegistry_ungetServiceReference(registry, bundle, reference);

	//test known reference, destroyed == true, but wrong bundle (cannot find reference in bundle->refsMap)
	destroyed = true;

	mock().expectOneCall("serviceReference_getUsageCount")
			.withParameter("reference", reference)
			.withOutputParameterReturning("count", &count, sizeof(count));
	mock().expectOneCall("serviceReference_release")
			.withParameter("ref", reference)
			.withOutputParameterReturning("destroyed", &destroyed, sizeof(destroyed));
	mock().expectOneCall("framework_log");

	serviceRegistry_ungetServiceReference(registry, bundle2, reference);

	//test known reference, destroyed == true, but count > 0
	destroyed = true;
	count = 5;

    const char* mod_name = "mod name";
    //const char* srv_name = "srv name";
	mock().expectOneCall("serviceReference_getUsageCount")
			.withParameter("reference", reference)
			.withOutputParameterReturning("count", &count, sizeof(count));
	mock().expectOneCall("serviceReference_release")
			.withParameter("ref", reference)
			.withOutputParameterReturning("destroyed", &destroyed, sizeof(destroyed));
    mock().expectNCalls(1, "bundle_getCurrentModule")
            .withParameter("bundle", bundle)
			.withOutputParameterReturning("module", &module, sizeof(module));
    mock().expectNCalls(1, "module_getSymbolicName")
            .withParameter("module", module)
			.withOutputParameterReturning("symbolicName", &mod_name, sizeof(mod_name));
	mock().expectNCalls(2, "framework_log");

	serviceRegistry_ungetServiceReference(registry, bundle, reference);

	CHECK((bool)hashMap_remove(registry->deletedServiceReferences, reference));

	//test known reference2, destroyed == true, and count == 0
	references = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references, registration, reference);
	hashMap_put(references, registration2, reference2);
	hashMap_put(registry->serviceReferences, bundle, references);
	hashMap_put(registry->deletedServiceReferences, reference2, (void*) false);
	destroyed = true;
	count = 0;

	mock().expectOneCall("serviceReference_getUsageCount")
			.withParameter("reference", reference2)
			.withOutputParameterReturning("count", &count, sizeof(count));
	mock().expectOneCall("serviceReference_release")
			.withParameter("ref", reference2)
			.withOutputParameterReturning("destroyed", &destroyed, sizeof(destroyed));

	serviceRegistry_ungetServiceReference(registry, bundle, reference2);

	CHECK((bool)hashMap_remove(registry->deletedServiceReferences, reference2));//check that ref2 deleted == true
	POINTERS_EQUAL(reference, hashMap_remove(references, registration)); //check that ref1 is untouched

	//cleanup
	hashMap_remove(references2, registration3);
	hashMap_destroy(references, false, false);
	hashMap_destroy(references2, false, false);
	serviceRegistry_destroy(registry);
}

TEST(service_registry, clearReferencesFor_1){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	service_registration_pt registration = (service_registration_pt) 0x10;
	service_reference_pt reference = (service_reference_pt) 0x40;
	bundle_pt bundle = (bundle_pt) 0x70;
	//module_pt module = (module_pt) 0x80;
	//const char* modName = "mod name";

	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references, registration, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	size_t useCount = 0;
	size_t refCount = 0;
	bool destroyed = true;
	hashMap_put(registry->deletedServiceReferences, reference, (void*) false);

	//expected calls for removing reference1
	mock().expectOneCall("serviceReference_getUsageCount")
			.withParameter("reference", reference)
			.withOutputParameterReturning("count", &useCount, sizeof(useCount));
	mock().expectOneCall("serviceReference_getReferenceCount")
				.withParameter("reference", reference)
				.withOutputParameterReturning("count", &refCount, sizeof(refCount));
	mock().expectOneCall("serviceReference_release")
			.withParameter("ref", reference)
			.withOutputParameterReturning("destroyed", &destroyed, sizeof(destroyed));

	serviceRegistry_clearReferencesFor(registry, bundle);

	serviceRegistry_destroy(registry);
}

TEST(service_registry, clearReferencesFor_2){
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	service_registration_pt registration = (service_registration_pt) 0x10;
	service_reference_pt reference = (service_reference_pt) 0x40;
	bundle_pt bundle = (bundle_pt) 0x70;
	module_pt module = (module_pt) 0x80;
	const char* modName = "mod name";
    const char* srvName = "srv name";

	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references, registration, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	//expected calls for removing reference2 (including count error logging)
	size_t useCount = 1;
	size_t refCount = 1;
    bool destroyed = true;
	mock().expectOneCall("serviceReference_getUsageCount")
			.withParameter("reference", reference)
			.withOutputParameterReturning("count", &useCount, sizeof(useCount));
	mock().expectOneCall("serviceReference_getReferenceCount")
			.withParameter("reference", reference)
			.withOutputParameterReturning("count", &refCount, sizeof(refCount));
	mock().expectNCalls(2, "framework_log");
	size_t updatedUseCount = 0;
	mock().expectOneCall("serviceReference_decreaseUsage")
			.withParameter("ref", reference)
			.withOutputParameterReturning("updatedCount", &updatedUseCount, sizeof(updatedUseCount));
	mock().expectNCalls(1, "serviceReference_release")
			.withParameter("ref", reference)
			.withOutputParameterReturning("destroyed", &destroyed, sizeof(destroyed));
	mock().expectNCalls(2, "bundle_getCurrentModule")
			.withParameter("bundle", bundle)
			.withOutputParameterReturning("module", &module, sizeof(module));
	mock().expectNCalls(2, "module_getSymbolicName")
			.withParameter("module", module)
			.withOutputParameterReturning("symbolicName", &modName, sizeof(modName));
    mock().expectOneCall("serviceReference_getProperty")
            .withParameter("reference", reference)
            .withParameter("key", OSGI_FRAMEWORK_OBJECTCLASS)
			.withOutputParameterReturning("value", &srvName, sizeof(srvName));
    mock().expectOneCall("serviceReference_getServiceRegistration")
            .withParameter("reference", reference)
			.withOutputParameterReturning("registration", &registration, sizeof(registration));
    mock().expectOneCall("serviceRegistration_getBundle")
            .withParameter("registration", registration)
			.withOutputParameterReturning("bundle", &bundle, sizeof(bundle));
	mock().expectOneCall("framework_log");

	serviceRegistry_clearReferencesFor(registry, bundle);

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

	void * service = (void *) 0x60;

	//test invalid reference
	mock().expectOneCall("framework_log");


	const void *actual = (void*) 0x666;//generic non null pointer value
	celix_status_t status = serviceRegistry_getService(registry, bundle, reference, &actual);
	LONGS_EQUAL(CELIX_BUNDLE_EXCEPTION, status);
	//test reference with invalid registration
	hashMap_put(registry->deletedServiceReferences, reference, (void*) false);

	mock()
		.expectOneCall("serviceReference_getServiceRegistration")
		.withParameter("reference", reference)
		.withOutputParameterReturning("registration", &registration, sizeof(registration))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceRegistration_isValid")
		.withParameter("registration", registration)
		.andReturnValue(false);

	actual = (void*) 0x666;//generic non null pointer value
	serviceRegistry_getService(registry, bundle, reference, &actual);
	POINTERS_EQUAL(NULL, actual);

	//test reference with valid registration
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

	actual = NULL;
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

TEST(service_registry, getListenerHooks) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);
	bundle_pt bundle = (bundle_pt) 0x10;
	service_registration_pt registration = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	registration->serviceId = 20UL;
	arrayList_add(registry->listenerHooks, registration);

	hash_map_pt usages = hashMap_create(NULL, NULL, NULL, NULL);
	service_reference_pt reference = (service_reference_pt) 0x30;
	hashMap_put(usages, (void*)registration->serviceId, reference);
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
	free(registration);
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

TEST(service_registry, getUsingBundles) {
	service_registry_pt registry = NULL;
	framework_pt framework = (framework_pt) 0x01;
	serviceRegistry_create(framework,serviceRegistryTest_serviceChanged, &registry);

	service_registration_pt registration  = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	service_registration_pt registration2 = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	service_registration_pt registration3 = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));
	service_registration_pt registration4 = (service_registration_pt) calloc(1,sizeof(struct serviceRegistration));

	registration->serviceId  = 10UL;
	registration2->serviceId = 20UL;
	registration3->serviceId = 30UL;
	registration4->serviceId = 40UL;

	service_reference_pt reference = (service_reference_pt) 0x50;
	service_reference_pt reference2 = (service_reference_pt) 0x60;
	service_reference_pt reference3 = (service_reference_pt) 0x70;
	service_reference_pt reference4 = (service_reference_pt) 0x80;
	service_reference_pt reference5 = (service_reference_pt) 0x90;

	bundle_pt bundle = (bundle_pt) 0xA0;
	bundle_pt bundle2 = (bundle_pt) 0xB0;
	bundle_pt bundle3 = (bundle_pt) 0xC0;

	//only contains registration1
	hash_map_pt references = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references, (void*)registration->serviceId, reference);
	hashMap_put(registry->serviceReferences, bundle, references);

	//contains registration1 and one other
	hash_map_pt references2 = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references2, (void*)registration->serviceId, reference2);
	hashMap_put(references2, (void*)registration2->serviceId, reference3);
	hashMap_put(registry->serviceReferences, bundle2, references2);

	//contains 2 registrations, but not registration1
	hash_map_pt references3 = hashMap_create(NULL, NULL, NULL, NULL);
	hashMap_put(references3, (void*)registration3->serviceId, reference4);
	hashMap_put(references3, (void*)registration4->serviceId, reference5);
	hashMap_put(registry->serviceReferences, bundle3, references3);

	//call to getUsingBundles
	array_list_pt get_bundles_list = NULL;
	registry->callback.getUsingBundles(registry, registration, &get_bundles_list);

	//check that both bundle1 and bundle2 have been found
	LONGS_EQUAL(2, arrayList_size(get_bundles_list));
	bundle_pt get_bundle = (bundle_pt) arrayList_get(get_bundles_list, 0);
	if (get_bundle == bundle){
		get_bundle = (bundle_pt) arrayList_get(get_bundles_list, 1);
		POINTERS_EQUAL(bundle2, get_bundle);
	} else {
		POINTERS_EQUAL(bundle2, get_bundle);
		get_bundle = (bundle_pt) arrayList_get(get_bundles_list, 1);
		POINTERS_EQUAL(bundle, get_bundle);
	}

	//cleanup
	arrayList_destroy(get_bundles_list);

	hashMap_destroy(references, false, false);
	hashMap_destroy(references2, false, false);
	hashMap_destroy(references3, false, false);
	serviceRegistry_destroy(registry);

   free(registration);
   free(registration2);
   free(registration3);
   free(registration4);

}
