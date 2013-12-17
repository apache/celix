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
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestHarness_c.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
#include "service_tracker_private.h"
#include "service_reference_private.h"
}

int main(int argc, char** argv) {
	return RUN_ALL_TESTS(argc, argv);
}

TEST_GROUP(service_tracker) {
	apr_pool_t *pool;

	void setup(void) {
		apr_initialize();
		apr_pool_create(&pool, NULL);
	}

	void teardown() {
		apr_pool_destroy(pool);
		mock().checkExpectations();
		mock().clear();
	}
};

TEST(service_tracker, create) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x123;
	std::string service = "service";
	status = serviceTracker_create(pool, ctx, (char *) service.c_str(), NULL, &tracker);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(ctx, tracker->context);
	POINTERS_EQUAL(NULL, tracker->customizer);
	POINTERS_EQUAL(NULL, tracker->listener);
	POINTERS_EQUAL(pool, tracker->pool);
	POINTERS_EQUAL(tracker, tracker->tracker);
	STRCMP_EQUAL("(objectClass=service)", tracker->filter);
}

TEST(service_tracker, createWithFilter) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x123;
	std::string filter = "(objectClass=test)";
	status = serviceTracker_createWithFilter(pool, ctx, (char *) filter.c_str(), NULL, &tracker);

	LONGS_EQUAL(CELIX_SUCCESS, status);
	POINTERS_EQUAL(ctx, tracker->context);
	POINTERS_EQUAL(NULL, tracker->customizer);
	POINTERS_EQUAL(NULL, tracker->listener);
	POINTERS_EQUAL(pool, tracker->pool);
	POINTERS_EQUAL(tracker, tracker->tracker);
	STRCMP_EQUAL("(objectClass=test)", tracker->filter);
}

TEST(service_tracker, destroy) {
	celix_status_t status;
	service_tracker_pt tracker = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x123;
	std::string filter = "(objectClass=test)";
	status = serviceTracker_createWithFilter(pool, ctx, (char *) filter.c_str(), NULL, &tracker);
	service_listener_pt listener = (service_listener_pt) 0x20;
	tracker->listener = listener;

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", ctx)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);
}

TEST(service_tracker, open) {
	// Without initial services and no customizer
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	std::string filter = "(objectClass=service)";
	tracker->filter = (char *) filter.c_str();
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;

	array_list_pt refs = NULL;
	arrayList_create(&refs);

	mock().strictOrder();
	mock()
		.expectOneCall("bundleContext_getServiceReferences")
		.withParameter("context", ctx)
		.withParameter("serviceName", (char *) NULL)
		.withParameter("filter", "(objectClass=service)")
		.andOutputParameter("service_references", refs)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_addServiceListener")
		.withParameter("context", ctx)
		.withParameter("filter", "(objectClass=service)")
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_open(tracker);
	CHECK(tracker->listener != NULL);

	// No services should be found
	LONGS_EQUAL(0, arrayList_size(tracker->tracked));
}

TEST(service_tracker, open_withRefs) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	tracker->customizer = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	std::string filter = "(objectClass=service)";
	tracker->filter = (char *) filter.c_str();
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
//	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
//	entry->service = (void *) 0x31;
//	service_reference_pt ref = (service_reference_pt) 0x51;
//	entry->reference = ref;
//	arrayList_add(tracked, entry);
//	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
//	entry2->service = (void *) 0x32;
//	service_reference_pt ref2 = (service_reference_pt) 0x52;
//	entry2->reference = ref2;
//	arrayList_add(tracked, entry2);

	array_list_pt refs = NULL;
	arrayList_create(&refs);
	service_reference_pt ref = (service_reference_pt) apr_palloc(pool, sizeof(*ref));
	arrayList_add(refs, ref);
	void *src = (void *) 0x345;

//	mock().strictOrder();
	mock()
		.expectOneCall("bundleContext_getServiceReferences")
		.withParameter("context", ctx)
		.withParameter("serviceName", (char *) NULL)
		.withParameter("filter", "(objectClass=service)")
		.andOutputParameter("service_references", refs)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_addServiceListener")
		.withParameter("context", ctx)
		.withParameter("filter", "(objectClass=service)")
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_getService")
		.withParameter("context", ctx)
		.withParameter("reference", ref)
		.andOutputParameter("service_instance", src)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_open(tracker);
	CHECK(tracker->listener != NULL);

	// One service should be found
	LONGS_EQUAL(1, arrayList_size(tracker->tracked));
}

TEST(service_tracker, open_withRefsAndTracked) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	tracker->customizer = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	std::string filter = "(objectClass=service)";
	tracker->filter = (char *) filter.c_str();
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	array_list_pt refs = NULL;
	arrayList_create(&refs);
	arrayList_add(refs, ref);
	void *src = (void *) 0x345;

//	mock().strictOrder();
	mock()
		.expectOneCall("bundleContext_getServiceReferences")
		.withParameter("context", ctx)
		.withParameter("serviceName", (char *) NULL)
		.withParameter("filter", "(objectClass=service)")
		.andOutputParameter("service_references", refs)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_addServiceListener")
		.withParameter("context", ctx)
		.withParameter("filter", "(objectClass=service)")
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.andOutputParameter("equal", true)
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_open(tracker);
	CHECK(tracker->listener != NULL);

	// One service should be found
	LONGS_EQUAL(1, arrayList_size(tracker->tracked));
}

TEST(service_tracker, close) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->customizer = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x345;
	tracker->context = ctx;
	// new tracker->listener
	service_listener_pt listener = (service_listener_pt) 0x42;
	tracker->listener = (service_listener_pt) listener;
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	mock()
		.expectOneCall("bundleContext_removeServiceListener")
		.withParameter("context", ctx)
		.withParameter("listener", listener)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.andOutputParameter("equal", true)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", ctx)
		.withParameter("reference", ref)
		.andOutputParameter("result", true)
		.andReturnValue(CELIX_SUCCESS);

	serviceTracker_close(tracker);
}

TEST(service_tracker, getServiceReference) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);
	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
	entry2->service = (void *) 0x32;
	service_reference_pt ref2 = (service_reference_pt) 0x52;
	entry2->reference = ref2;
	arrayList_add(tracked, entry2);

	service_reference_pt reference = serviceTracker_getServiceReference(tracker);
	POINTERS_EQUAL(ref, reference);
}

TEST(service_tracker, getServiceReferenceNull) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;

	service_reference_pt reference = serviceTracker_getServiceReference(tracker);
	POINTERS_EQUAL(NULL, reference);
}

TEST(service_tracker, getServiceReferences) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);
	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
	entry2->service = (void *) 0x32;
	service_reference_pt ref2 = (service_reference_pt) 0x52;
	entry2->reference = ref2;
	arrayList_add(tracked, entry2);

	array_list_pt references = serviceTracker_getServiceReferences(tracker);
	LONGS_EQUAL(2, arrayList_size(references));
	POINTERS_EQUAL(ref, arrayList_get(references, 0));
	POINTERS_EQUAL(ref2, arrayList_get(references, 1));
}

TEST(service_tracker, getService) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);
	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
	entry2->service = (void *) 0x32;
	service_reference_pt ref2 = (service_reference_pt) 0x52;
	entry2->reference = ref2;
	arrayList_add(tracked, entry2);

	void *service = serviceTracker_getService(tracker);
	POINTERS_EQUAL(0x31, service);
}

TEST(service_tracker, getServiceNull) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;

	void *service = serviceTracker_getService(tracker);
	POINTERS_EQUAL(NULL, service);
}

TEST(service_tracker, getServices) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);
	tracked_pt entry2 = (tracked_pt) malloc(sizeof(*entry));
	entry2->service = (void *) 0x32;
	service_reference_pt ref2 = (service_reference_pt) 0x52;
	entry2->reference = ref2;
	arrayList_add(tracked, entry2);

	array_list_pt services = serviceTracker_getServices(tracker);
	LONGS_EQUAL(2, arrayList_size(services));
	POINTERS_EQUAL(0x31, arrayList_get(services, 0));
	POINTERS_EQUAL(0x32, arrayList_get(services, 1));
}

TEST(service_tracker, getServiceByReference) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.andOutputParameter("equal", true)
		.ignoreOtherParameters()
		.andReturnValue(4)
		.withCallOrder(1);
	void *service = serviceTracker_getServiceByReference(tracker, ref);
	POINTERS_EQUAL(0x31, service);
}

TEST(service_tracker, getServiceByReferenceNull) {
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.andOutputParameter("equal", false)
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS)
		.withCallOrder(1);
	void *service = serviceTracker_getServiceByReference(tracker, ref);
	POINTERS_EQUAL(NULL, service);
}

TEST(service_tracker, serviceChangedRegistered) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	tracker->customizer = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;

	service_reference_pt ref = (service_reference_pt) 0x51;
	void *src = (void *) 0x345;

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;
	event->reference = ref;

	mock()
		.expectOneCall("bundleContext_getService")
		.withParameter("context", ctx)
		.withParameter("reference", ref)
		.andOutputParameter("service_instance", src)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}

TEST(service_tracker, serviceChangedModified) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	tracker->customizer = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED;
	event->reference = ref;

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.andOutputParameter("equal", true)
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}

TEST(service_tracker, serviceChangedUnregistering) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	tracker->customizer = NULL;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING;
	event->reference = ref;

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.andOutputParameter("equal", true)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", ctx)
		.withParameter("reference", ref)
		.andOutputParameter("result", true)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}

TEST(service_tracker, serviceChangedModifiedEndmatch) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;

	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH;
	event->reference = ref;

	serviceTracker_serviceChanged(listener, event);
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
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	tracker->customizer = customizer;

	service_reference_pt ref = (service_reference_pt) 0x51;
	void *src = (void *) 0x345;

	void * handle = (void*) 0x60;

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;
	event->reference = ref;

	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.andOutputParameter("handle", handle)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getAddingFunction")
		.withParameter("customizer", customizer)
		.andOutputParameter("function", (void *) serviceDependency_addingService)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.andOutputParameter("handle", handle)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getAddedFunction")
		.withParameter("customizer", customizer)
		.andOutputParameter("function", (void *) serviceDependency_addedService)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}


extern "C" {
	celix_status_t serviceDependency_modifiedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
}

TEST(service_tracker, serviceChangedModifiedCustomizer) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	tracker->customizer = customizer;

	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED;
	event->reference = ref;

	void * handle = (void*) 0x60;

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.andOutputParameter("equal", true)
		.ignoreOtherParameters()
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.andOutputParameter("handle", handle)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getModifiedFunction")
		.withParameter("customizer", customizer)
		.andOutputParameter("function", (void *) serviceDependency_modifiedService)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}

extern "C" {
	celix_status_t serviceDependency_removedService(void * handle, service_reference_pt reference, void * service) {
		return CELIX_SUCCESS;
	}
}

TEST(service_tracker, serviceChangedUnregisteringCustomizer) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	tracker->customizer = customizer;

	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING;
	event->reference = ref;

	void * handle = (void*) 0x60;

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.andOutputParameter("equal", true)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.andOutputParameter("handle", handle)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getRemovedFunction")
		.withParameter("customizer", customizer)
		.andOutputParameter("function", (void *) serviceDependency_removedService)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}

TEST(service_tracker, serviceChangedUnregisteringCustomizerNoFunc) {
	// With one initial service
	// new tracker
	service_tracker_pt tracker = (service_tracker_pt) apr_palloc(pool, sizeof(*tracker));
	tracker->pool = pool;
	bundle_context_pt ctx = (bundle_context_pt) 0x10;
	tracker->context = ctx;
	service_listener_pt listener = (service_listener_pt) apr_palloc(pool, sizeof(*listener));
	tracker->listener = listener;
	listener->handle = tracker;
	service_tracker_customizer_pt customizer = (service_tracker_customizer_pt) 0x20;
	tracker->customizer = customizer;

	// new tracker->tracked
	array_list_pt tracked = NULL;
	arrayList_create(&tracked);
	tracker->tracked = tracked;
//	// add tracked to tracker->tracked
	tracked_pt entry = (tracked_pt) malloc(sizeof(*entry));
	entry->service = (void *) 0x31;
	service_reference_pt ref = (service_reference_pt) 0x51;
	entry->reference = ref;
	arrayList_add(tracked, entry);

	service_event_pt event = (service_event_pt) apr_palloc(pool, sizeof(*event));
	event->type = OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING;
	event->reference = ref;

	void * handle = (void*) 0x60;

	mock()
		.expectOneCall("serviceReference_equals")
		.withParameter("reference", ref)
		.withParameter("compareTo", ref)
		.andOutputParameter("equal", true)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getHandle")
		.withParameter("customizer", customizer)
		.andOutputParameter("handle", handle)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("serviceTrackerCustomizer_getRemovedFunction")
		.withParameter("customizer", customizer)
		.andOutputParameter("function", (void *) NULL)
		.andReturnValue(CELIX_SUCCESS);
	mock()
		.expectOneCall("bundleContext_ungetService")
		.withParameter("context", ctx)
		.withParameter("reference", ref)
		.andOutputParameter("result", true)
		.andReturnValue(CELIX_SUCCESS);
	serviceTracker_serviceChanged(listener, event);
}



