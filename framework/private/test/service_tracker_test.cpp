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
 * service_tracker_test.cpp
 *
 *  \date       Feb 6, 2013
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

extern "C"
{
#include "service_tracker_private.h"
#include "service_reference_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
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

TEST_GROUP(service_tracker) {
	void setup(void) {
	}

	void teardown() {
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(service_tracker, create) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt context = (bundle_context_pt) 0x123;
	char * service = my_strdup("service");
	status = serviceTracker_create(context, service, NULL, &tracker);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(context, tracker->context);
	POINTERS_EQUAL(NULL, tracker->customizer);
	POINTERS_EQUAL(NULL, tracker->listener);
	POINTERS_EQUAL(tracker, tracker->tracker);
	STRCMP_EQUAL("(objectClass=service)", tracker->filter);

	serviceTracker_destroy(tracker);
	free(service);
}

TEST(service_tracker, createWithCustomizer) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt context = (bundle_context_pt) 0x123;
	char * service = my_strdup("service");
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	status = serviceTracker_create(context, service, customizer, &tracker);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(context, tracker->context);
	POINTERS_EQUAL(customizer, tracker->customizer);
	POINTERS_EQUAL(NULL, tracker->listener);
	POINTERS_EQUAL(tracker, tracker->tracker);
	STRCMP_EQUAL("(objectClass=service)", tracker->filter);

	mock().expectOneCall("serviceTrackerCustomizer_destroy")
			.withParameter("customizer", customizer);

	serviceTracker_destroy(tracker);
	free(service);
}

TEST(service_tracker, createWithFilter) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt context = (bundle_context_pt) 0x123;
	char * filter = my_strdup("(objectClass=test)");
	status = serviceTracker_createWithFilter(context, filter, NULL, &tracker);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(context, tracker->context);
	POINTERS_EQUAL(NULL, tracker->customizer);
	POINTERS_EQUAL(NULL, tracker->listener);
	POINTERS_EQUAL(tracker, tracker->tracker);
	STRCMP_EQUAL("(objectClass=test)", tracker->filter);

	serviceTracker_destroy(tracker);
	free(filter);
}

TEST(service_tracker, createWithFilterWithCustomizer) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt context = (bundle_context_pt) 0x123;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	char * filter = my_strdup("(objectClass=test)");
	status = serviceTracker_createWithFilter(context, filter, customizer, &tracker);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(context, tracker->context);
	POINTERS_EQUAL(customizer, tracker->customizer);
	POINTERS_EQUAL(NULL, tracker->listener);
	POINTERS_EQUAL(tracker, tracker->tracker);
	STRCMP_EQUAL("(objectClass=test)", tracker->filter);

	mock().expectOneCall("serviceTrackerCustomizer_destroy")
			.withParameter("customizer", customizer);

	serviceTracker_destroy(tracker);
	free(filter);
}

TEST(service_tracker, destroy) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt context = (bundle_context_pt) 0x123;
	char * filter = my_strdup("(objectClass=test)");
	status = serviceTracker_createWithFilter(context, filter, NULL, &tracker);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	service_listener_pt listener = (service_listener_pt) calloc(1, sizeof(serviceListener));
	tracker->listener = listener;

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	status = serviceTracker_destroy(tracker);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	free(filter);
}

TEST(service_tracker, open) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * filter = my_strdup("(objectClass=service)");
	service_tracker_pt tracker = NULL;
	serviceTracker_createWithFilter(context, filter, NULL, &tracker);

	array_list_pt refs = NULL;
	arrayList_create(&refs);

	mock().strictOrder();
	mock()
		.expectOneCall("bundleContext_getServiceReferences")
		.withParameter("context", context)
		.withParameter("serviceName", (char *) NULL)
		.withParameter("filter", "(objectClass=service)")
		.withOutputParameterReturning("service_references", &refs, sizeof(refs));
	mock()
		.expectOneCall("bundleContext_addServiceListener")
		.withParameter("context", context)
		.withParameter("filter", "(objectClass=service)")
		.ignoreOtherParameters();
	serviceTracker_open(tracker);
	CHECK(tracker->listener != NULL);

	// No services should be found
	LONGS_EQUAL(0, arrayList_size(tracker->trackedServices));

	mock().expectOneCall("bundleContext_removeServiceListener")
			.withParameter("context", context)
			.ignoreOtherParameters();

	serviceTracker_destroy(tracker);
	free(filter);
}

TEST(service_tracker, open_withRefs) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt context = (bundle_context_pt) 0x123;
	char * filter = my_strdup("(objectClass=test)");
	status = serviceTracker_createWithFilter(context, filter, NULL, &tracker);
	LONGS_EQUAL(CELIX_SUCCESS, status);
	tracker->listener = NULL;

	array_list_pt refs = NULL;
	arrayList_create(&refs);
	service_reference_pt ref = (service_reference_pt) malloc(sizeof(*ref));
	arrayList_add(refs, ref);
	void *src = (void *) 0x345;

	mock()
		.expectOneCall("bundleContext_getServiceReferences")
		.withParameter("context", context)
		.withParameter("serviceName", (char *) NULL)
		.withParameter("filter", filter)
		.withOutputParameterReturning("service_references", &refs, sizeof(refs));
	mock()
		.expectOneCall("bundleContext_addServiceListener")
		.withParameter("context", context)
		.withParameter("filter", filter)
		.ignoreOtherParameters();
	mock()
		.expectOneCall("bundleContext_retainServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);
	mock()
		.expectOneCall("bundleContext_getService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("service_instance", &src, sizeof(src));
	serviceTracker_open(tracker);

	CHECK(tracker->listener != NULL);
	tracked_pt get_tracked = (tracked_pt) arrayList_get(tracker->trackedServices, 0);
	POINTERS_EQUAL(src, get_tracked->service);
	POINTERS_EQUAL(ref, get_tracked->reference);


	// One service should be found
	LONGS_EQUAL(1, arrayList_size(tracker->trackedServices));

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.ignoreOtherParameters();

	serviceTracker_destroy(tracker);
	free(ref);
	free(get_tracked);
	free(filter);
}

TEST(service_tracker, open_withRefsAndTracked) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	service_reference_pt ref = (service_reference_pt) 0x02;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	array_list_pt refs = NULL;
	arrayList_create(&refs);
	arrayList_add(refs, ref);

	mock()
		.expectOneCall("bundleContext_getServiceReferences")
		.withParameter("context", context)
		.withParameter("serviceName", (char *) NULL)
		.withParameter("filter", "(objectClass=service_name)")
		.withOutputParameterReturning("service_references", &refs, sizeof(refs));
	mock()
		.expectOneCall("bundleContext_addServiceListener")
		.withParameter("context", context)
		.withParameter("filter", "(objectClass=service_name)")
		.ignoreOtherParameters();
	mock()
		.expectOneCall("bundleContext_retainServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);
	bool equal = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_open(tracker);
	CHECK(tracker->listener != NULL);

	// One service should be found
	LONGS_EQUAL(1, arrayList_size(tracker->trackedServices));

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.ignoreOtherParameters();

	serviceTracker_destroy(tracker);
	free(entry);
	free(service);
}

TEST(service_tracker, close) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	service_reference_pt ref = (service_reference_pt) 0x02;

	tracker->listener = listener;

	entry->service = (void *) 0x03;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);
	bool equal = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.andReturnValue(CELIX_SUCCESS);
	bool result = true;
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("result", &result, sizeof(result))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_ungetServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);

	celix_status_t status;

	status = serviceTracker_close(tracker);
	LONGS_EQUAL(CELIX_SUCCESS, status);

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_destroy(tracker);
	free(service);
}

TEST(service_tracker, getServiceReference) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);
	service_reference_pt reference = (service_reference_pt) 0x02;
	service_reference_pt reference2 = (service_reference_pt) 0x03;
	service_reference_pt get_reference;
	tracked_pt tracked = (tracked_pt) malloc(sizeof(*tracked));
	tracked_pt tracked2 = (tracked_pt) malloc(sizeof(*tracked2));

	tracked->reference = reference;
	tracked2->reference = reference2;
	arrayList_add(tracker->trackedServices, tracked);
	arrayList_add(tracker->trackedServices, tracked2);

	get_reference = serviceTracker_getServiceReference(tracker);

	//test for either of the references
	if(get_reference != reference && get_reference != reference2){
		FAIL("unknown reference");
	}

	serviceTracker_destroy(tracker);
	free(service);
	free(tracked);
	free(tracked2);
}

TEST(service_tracker, getServiceReferenceNull) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	service_reference_pt reference = serviceTracker_getServiceReference(tracker);
	POINTERS_EQUAL(NULL, reference);

	serviceTracker_destroy(tracker);
	free(service);
}

TEST(service_tracker, getServiceReferences) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);
	service_reference_pt reference = (service_reference_pt) 0x02;
	service_reference_pt reference2 = (service_reference_pt) 0x03;
	service_reference_pt get_reference;
	tracked_pt tracked = (tracked_pt) malloc(sizeof(*tracked));
	tracked_pt tracked2 = (tracked_pt) malloc(sizeof(*tracked2));
	array_list_pt get_references;

	tracked->reference = reference;
	tracked2->reference = reference2;
	arrayList_add(tracker->trackedServices, tracked);
	arrayList_add(tracker->trackedServices, tracked2);

	get_references = serviceTracker_getServiceReferences(tracker);

	//test for the references, in no specific order
	get_reference = (service_reference_pt) arrayList_get(get_references, 0);
	if(get_reference == reference){
		get_reference = (service_reference_pt) arrayList_get(get_references, 1);
		POINTERS_EQUAL(reference2, get_reference);
	} else {
		POINTERS_EQUAL(reference2, get_reference);
		get_reference = (service_reference_pt) arrayList_get(get_references, 1);
		POINTERS_EQUAL(reference, get_reference);
	}

	arrayList_destroy(get_references);

	serviceTracker_destroy(tracker);
	free(service);
	free(tracked);
	free(tracked2);
}

TEST(service_tracker, getService) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	service_reference_pt ref = (service_reference_pt) 0x02;
	entry->reference = ref;
	void * actual_service = (void*) 0x32;
	entry->service = actual_service;
	arrayList_add(tracker->trackedServices, entry);
	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
	service_reference_pt ref2 = (service_reference_pt) 0x52;
	entry2->reference = ref2;
	arrayList_add(tracker->trackedServices, entry2);

	void *get_service = serviceTracker_getService(tracker);
	POINTERS_EQUAL(actual_service, get_service);

	serviceTracker_destroy(tracker);
	free(entry);
	free(entry2);
	free(service);
}

TEST(service_tracker, getServiceNull) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	void * get_service = serviceTracker_getService(tracker);
	POINTERS_EQUAL(NULL, get_service);

	serviceTracker_destroy(tracker);
	free(service);
}

TEST(service_tracker, getServices) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);
	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
	entry2->service = (void *) 0x32;
	service_reference_pt ref2 = (service_reference_pt) 0x52;
	entry2->reference = ref2;
	arrayList_add(tracker->trackedServices, entry2);

	array_list_pt services = serviceTracker_getServices(tracker);
	LONGS_EQUAL(2, arrayList_size(services));
	POINTERS_EQUAL(0x31, arrayList_get(services, 0));
	POINTERS_EQUAL(0x32, arrayList_get(services, 1));

	arrayList_destroy(services);

	serviceTracker_destroy(tracker);
	free(entry);
	free(entry2);
	free(service);
}

TEST(service_tracker, getServiceByReference) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	bool equal = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.andReturnValue(4);
		//.ignoreOtherParameters();
	void * get_service = serviceTracker_getServiceByReference(tracker, ref);
	POINTERS_EQUAL(0x31, get_service);

	serviceTracker_destroy(tracker);
	free(entry);
	free(service);
}

TEST(service_tracker, getServiceByReferenceNull) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	bool equal = false;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS)
		.withCallOrder(1);
	void * get_service = serviceTracker_getServiceByReference(tracker, ref);
	POINTERS_EQUAL(NULL, get_service);

	serviceTracker_destroy(tracker);
	free(entry);
	free(service);
}

TEST(service_tracker, serviceChangedRegistered) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	service_reference_pt ref = (service_reference_pt) 0x51;

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;
	event->reference = ref;

	mock()
		.expectOneCall("bundleContext_retainServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);
	void *src = (void *) 0x345;
	mock()
		.expectOneCall("bundleContext_getService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("service_instance", &src, sizeof(src))
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);

	tracked_pt get_tracked = (tracked_pt) arrayList_get(tracker->trackedServices, 0);
	POINTERS_EQUAL(src, get_tracked->service);
	POINTERS_EQUAL(ref, get_tracked->reference);

	//cleanup
	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_destroy(tracker);
	free(get_tracked);
	free(event);
	free(service);
}

TEST(service_tracker, serviceChangedModified) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED;
	event->reference = ref;

	mock()
		.expectOneCall("bundleContext_retainServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);
	bool equal = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_serviceChanged(listener, event);

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_destroy(tracker);
	free(entry);
	free(event);
	free(service);
}

TEST(service_tracker, serviceChangedUnregistering) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);

	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING;
	event->reference = ref;

	bool equal = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.andReturnValue(CELIX_SUCCESS);
	bool result = true;
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("result", &result, sizeof(result))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_ungetServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);

	serviceTracker_serviceChanged(listener, event);

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_destroy(tracker);
	free(event);
	free(service);
}

TEST(service_tracker, serviceChangedModifiedEndmatch) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	serviceTracker_create(context, service, NULL, &tracker);
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH;
	event->reference = ref;

	serviceTracker_serviceChanged(listener, event);

	//cleanup
	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_destroy(tracker);
	free(entry);
	free(event);
	free(service);
}

extern "C" {
	celix_status_t serviceDependency_addingService(void * handle, service_reference_pt reference, void **service) {
		*service = (void*) 0x45;
		return CELIX_SUCCESS;
	}

	celix_status_t serviceDependency_addedService(void * handle, service_reference_pt reference, void *service) {
		return CELIX_SUCCESS;
	}
}

TEST(service_tracker, serviceChangedRegisteredCustomizer) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	serviceTracker_create(context, service, customizer, &tracker);
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	service_reference_pt ref = (service_reference_pt) 0x51;

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;
	event->reference = ref;

	mock()
		.expectOneCall("bundleContext_retainServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);
	void * handle = (void*) 0x60;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("handle", &handle, sizeof(handle))
		.andReturnValue(CELIX_SUCCESS);
	void *function = (void *) serviceDependency_addingService;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getAddingFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &function, sizeof(function))
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("handle", &handle, sizeof(handle))
		.andReturnValue(CELIX_SUCCESS);
	void *function2 = (void *) serviceDependency_addedService;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getAddedFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &function2, sizeof(function))
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);

	tracked_pt get_tracked = (tracked_pt) arrayList_get(tracker->trackedServices, 0);
	POINTERS_EQUAL(0x45, get_tracked->service);
	POINTERS_EQUAL(ref, get_tracked->reference);

	//cleanup
	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("serviceTrackerCustomizer_destroy")
		.withParameter("customizer", customizer);

	serviceTracker_destroy(tracker);
	free(get_tracked);
	free(event);
	free(service);
}


extern "C" {
	celix_status_t serviceDependency_modifiedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
}

TEST(service_tracker, serviceChangedModifiedCustomizer) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	serviceTracker_create(context, service, customizer, &tracker);
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;
	//adding_callback_pt adding_func = NULL;
	//added_callback_pt added_func = NULL;

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED;
	event->reference = ref;

	bool equal = true;
	mock()
		.expectOneCall("bundleContext_retainServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	void * handle = (void*) 0x60;

/*	this branch is not covered here, unlike earlier faulty tests
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("handle", &handle, sizeof(handle))
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("serviceTrackerCustomizer_getAddingFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &adding_func, sizeof(adding_func))
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("serviceTrackerCustomizer_getAddedFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &added_func, sizeof(added_func));

	mock()
		.expectOneCall("bundleContext_getService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("service_instance", &entry->service, sizeof(entry->service));
*/

	mock()
			.expectOneCall("serviceTrackerCustomizer_getHandle")
			.withParameter("customizer", customizer)
			.withOutputParameterReturning("handle", &handle, sizeof(handle))
			.andReturnValue(CELIX_SUCCESS);


	void *function = (void *) serviceDependency_modifiedService;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getModifiedFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &function, sizeof(function))
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_serviceChanged(listener, event);

	//cleanup
	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("serviceTrackerCustomizer_destroy")
		.withParameter("customizer", customizer);

	serviceTracker_destroy(tracker);
	free(entry);
	free(event);
	free(service);
}

extern "C" {
	celix_status_t serviceDependency_removedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
}

TEST(service_tracker, serviceChangedUnregisteringCustomizer) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	serviceTracker_create(context, service, customizer, &tracker);
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING;
	event->reference = ref;

	bool equal = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.withOutputParameterReturning("equal", &equal, sizeof(equal))
		.andReturnValue(CELIX_SUCCESS);
	void * handle = (void*) 0x60;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("handle", &handle, sizeof(handle))
		.andReturnValue(CELIX_SUCCESS);
	void *function = (void*) serviceDependency_removedService;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getRemovedFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &function , sizeof(function))
		.andReturnValue(CELIX_SUCCESS);

	bool result = true;
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("result", &result, sizeof(result));
	mock()
		.expectOneCall("bundleContext_ungetServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);

	serviceTracker_serviceChanged(listener, event);

	//clean up
	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("serviceTrackerCustomizer_destroy")
		.withParameter("customizer", customizer);

	serviceTracker_destroy(tracker);
	free(event);
	free(service);
}

TEST(service_tracker, serviceChangedUnregisteringCustomizerNoFunc) {
	bundle_context_pt context= (bundle_context_pt) 0x01;
	char * service = my_strdup("service_name");
	service_tracker_pt tracker = NULL;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	serviceTracker_create(context, service, customizer, &tracker);
	service_listener_pt listener = (service_listener_pt) malloc(sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracker->trackedServices, entry);

	service_event_pt event = (service_event_pt) malloc(sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING;
	event->reference = ref;

	bool equals = true;
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.withOutputParameterReturning("equal", &equals, sizeof(equals));
	void * handle = (void*) 0x60;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("handle", &handle, sizeof(handle));
	void *function = NULL;
	mock()
		.expectOneCall("serviceTrackerCustomizer_getRemovedFunction")
		.withParameter("customizer", customizer)
		.withOutputParameterReturning("function", &function, sizeof(function));
	bool result = true;
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", context)
		.withParameter("reference", ref)
		.withOutputParameterReturning("result", &result, sizeof(result));
	mock()
		.expectOneCall("bundleContext_ungetServiceReference")
		.withParameter("context", context)
		.withParameter("reference", ref);

	serviceTracker_serviceChanged(listener, event);

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", context)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);

	mock()
		.expectOneCall("serviceTrackerCustomizer_destroy")
		.withParameter("customizer", customizer);

	serviceTracker_destroy(tracker);
	free(event);
	free(service);
}
