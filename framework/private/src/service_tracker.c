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
 * service_tracker.c
 *
 *  \date       Apr 20, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <apr_strings.h>

#include "service_tracker_private.h"
#include "bundle_context.h"
#include "constants.h"
#include "service_reference.h"
#include "celix_log.h"

static apr_status_t serviceTracker_destroy(void *trackerP);
static celix_status_t serviceTracker_addingService(service_tracker_pt tracker, service_reference_pt reference, void **service);
static celix_status_t serviceTracker_track(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);

celix_status_t serviceTracker_create(apr_pool_t *pool, bundle_context_pt context, char * service, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	if (service == NULL || *tracker != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		if (status == CELIX_SUCCESS) {
			int len = strlen(service) + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + 4;
			char *filter = apr_pstrcat(pool, "(", OSGI_FRAMEWORK_OBJECTCLASS, "=", service, ")", NULL);
			if (filter == NULL) {
				status = CELIX_ENOMEM;
			} else {
				serviceTracker_createWithFilter(pool, context, filter, customizer, tracker);
			}
		}
	}

	framework_logIfError(status, NULL, "Cannot create service tracker");

	return status;
}

celix_status_t serviceTracker_createWithFilter(apr_pool_t *pool, bundle_context_pt context, char * filter, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	*tracker = (service_tracker_pt) apr_palloc(pool, sizeof(**tracker));
	if (!*tracker) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *tracker, serviceTracker_destroy);

		(*tracker)->context = context;
		(*tracker)->filter = apr_pstrdup(pool,filter);

		(*tracker)->pool = pool;
		(*tracker)->tracker = *tracker;
		(*tracker)->tracked = NULL;
		arrayList_create(&(*tracker)->tracked);
		(*tracker)->customizer = customizer;
		(*tracker)->listener = NULL;
	}

	framework_logIfError(status, NULL, "Cannot create service tracker [filter=%s]", filter);

	return status;
}

apr_status_t serviceTracker_destroy(void *trackerP) {
	service_tracker_pt tracker = (service_tracker_pt) trackerP;
	if (tracker->listener != NULL) {
		bundleContext_removeServiceListener(tracker->context, tracker->listener);
	}
	arrayList_destroy(tracker->tracked);

	return APR_SUCCESS;
}

celix_status_t serviceTracker_open(service_tracker_pt tracker) {
	service_listener_pt listener;
	array_list_pt initial = NULL;
	celix_status_t status = CELIX_SUCCESS;
	listener = (service_listener_pt) apr_palloc(tracker->pool, sizeof(*listener));
	
	status = bundleContext_getServiceReferences(tracker->context, NULL, tracker->filter, &initial);
	if (status == CELIX_SUCCESS) {
		service_reference_pt initial_reference;
		unsigned int i;

		listener->pool = tracker->pool;
		listener->handle = tracker;
		listener->serviceChanged = (void *) serviceTracker_serviceChanged;
		status = bundleContext_addServiceListener(tracker->context, listener, tracker->filter);
		if (status == CELIX_SUCCESS) {
			tracker->listener = listener;

			for (i = 0; i < arrayList_size(initial); i++) {
				initial_reference = (service_reference_pt) arrayList_get(initial, i);
				serviceTracker_track(tracker, initial_reference, NULL);
			}
			arrayList_clear(initial);
			arrayList_destroy(initial);

			initial = NULL;
		}
	}

	framework_logIfError(status, NULL, "Cannot open tracker");

	return status;
}

celix_status_t serviceTracker_close(service_tracker_pt tracker) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleContext_removeServiceListener(tracker->context, tracker->listener);
	if (status == CELIX_SUCCESS) {
		array_list_pt refs = serviceTracker_getServiceReferences(tracker);
		if (refs != NULL) {
			unsigned int i;
			for (i = 0; i < arrayList_size(refs); i++) {
				service_reference_pt ref = (service_reference_pt) arrayList_get(refs, i);
				status = serviceTracker_untrack(tracker, ref, NULL);
			}
		}
		arrayList_destroy(refs);
	}

	framework_logIfError(status, NULL, "Cannot close tracker");

	return status;
}

service_reference_pt serviceTracker_getServiceReference(service_tracker_pt tracker) {
	tracked_pt tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		return tracked->reference;
	}
	return NULL;
}

array_list_pt serviceTracker_getServiceReferences(service_tracker_pt tracker) {
	tracked_pt tracked;
	unsigned int i;
	int size = arrayList_size(tracker->tracked);
	array_list_pt references = NULL;
	arrayList_create(&references);
	
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		arrayList_add(references, tracked->reference);
	}
	return references;
}

void *serviceTracker_getService(service_tracker_pt tracker) {
	tracked_pt tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		return tracked->service;
	}
	return NULL;
}

array_list_pt serviceTracker_getServices(service_tracker_pt tracker) {
	tracked_pt tracked;
	unsigned int i;
	int size = arrayList_size(tracker->tracked);
	array_list_pt references = NULL;
	arrayList_create(&references);
	
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		arrayList_add(references, tracked->service);
	}
	return references;
}

void *serviceTracker_getServiceByReference(service_tracker_pt tracker, service_reference_pt reference) {
	tracked_pt tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		bool equals;
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			return tracked->service;
		}
	}
	return NULL;
}

void serviceTracker_serviceChanged(service_listener_pt listener, service_event_pt event) {
	service_tracker_pt tracker = listener->handle;
	switch (event->type) {
		case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
		case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED:
			serviceTracker_track(tracker, event->reference, event);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
			serviceTracker_untrack(tracker, event->reference, event);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH:
			break;
	}
}

static celix_status_t serviceTracker_track(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	tracked_pt tracked = NULL;
	int found = -1;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		bool equals;
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			found = 0;
			break;
		}
	}

	if (found) {
		void * service = NULL;
		status = serviceTracker_addingService(tracker, reference, &service);
		if (status == CELIX_SUCCESS) {
			if (service != NULL) {
				tracked = (tracked_pt) malloc(sizeof(*tracked));
				tracked->reference = reference;
				tracked->service = service;
				arrayList_add(tracker->tracked, tracked);
				if (tracker->customizer != NULL) {
					void *handle = NULL;
					added_callback_pt function = NULL;

					serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
					serviceTrackerCustomizer_getAddedFunction(tracker->customizer, &function);
					if (function != NULL) {
						function(handle, reference, service);
					}
				}
			}
		}
	} else {
		if (tracker->customizer != NULL) {
			void *handle = NULL;
			modified_callback_pt function = NULL;

			serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
			serviceTrackerCustomizer_getModifiedFunction(tracker->customizer, &function);

			if (function != NULL) {
				function(handle, reference, tracked->service);
			}
		}
	}

	framework_logIfError(status, NULL, "Cannot track reference");

	return status;
}

static celix_status_t  serviceTracker_addingService(service_tracker_pt tracker, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;

    if (tracker->customizer != NULL) {
    	void *handle = NULL;
		adding_callback_pt function = NULL;

		status =  serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
		if (status == CELIX_SUCCESS) {
			status = serviceTrackerCustomizer_getAddingFunction(tracker->customizer, &function);
			if (status == CELIX_SUCCESS) {
				if (function != NULL) {
					status = function(handle, reference, service);
				}
			}
		} else {
			status = bundleContext_getService(tracker->context, reference, service);
		}
	} else {
		status = bundleContext_getService(tracker->context, reference, service);
	}

    framework_logIfError(status, NULL, "Cannot handle addingService");

    return status;
}

static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	tracked_pt tracked = NULL;
	unsigned int i;
	bool result = false;

	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		bool equals;
		tracked = (tracked_pt) arrayList_get(tracker->tracked, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			arrayList_remove(tracker->tracked, i);
			if (status == CELIX_SUCCESS) {
				if (tracker->customizer != NULL) {
					void *handle = NULL;
					removed_callback_pt function = NULL;

					serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
					serviceTrackerCustomizer_getRemovedFunction(tracker->customizer, &function);

					if (function != NULL) {
						status = function(handle, reference, tracked->service);
					} else {
						status = bundleContext_ungetService(tracker->context, reference, &result);
					}
				} else {
					status = bundleContext_ungetService(tracker->context, reference, &result);
				}
				free(tracked);
				break;
			}
		}
	}

	framework_logIfError(status, NULL, "Cannot untrack reference");

	return status;
}
