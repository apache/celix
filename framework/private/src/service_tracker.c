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

#include "service_tracker.h"
#include "bundle_context.h"
#include "constants.h"
#include "service_reference.h"

struct serviceTracker {
	BUNDLE_CONTEXT context;
	char * filter;

	apr_pool_t *pool;
	SERVICE_TRACKER tracker;
	SERVICE_TRACKER_CUSTOMIZER customizer;
	SERVICE_LISTENER listener;
	ARRAY_LIST tracked;
};

struct tracked {
	SERVICE_REFERENCE reference;
	void * service;
};

typedef struct tracked * TRACKED;

static apr_status_t serviceTracker_destroy(void *trackerP);
static void * serviceTracker_addingService(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference);
static celix_status_t serviceTracker_track(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference, SERVICE_EVENT event);
static celix_status_t serviceTracker_untrack(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference, SERVICE_EVENT event);

celix_status_t serviceTracker_create(apr_pool_t *pool, BUNDLE_CONTEXT context, char * service, SERVICE_TRACKER_CUSTOMIZER customizer, SERVICE_TRACKER *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	if (service == NULL || *tracker != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		if (status == CELIX_SUCCESS) {
			int len = strlen(service) + strlen(OBJECTCLASS) + 4;
			char *filter = apr_pstrcat(pool, "(", OBJECTCLASS, "=", service, ")", NULL);
			if (filter == NULL) {
				status = CELIX_ENOMEM;
			} else {
				serviceTracker_createWithFilter(pool, context, filter, customizer, tracker);
			}
		}
	}


	return status;
}

celix_status_t serviceTracker_createWithFilter(apr_pool_t *pool, BUNDLE_CONTEXT context, char * filter, SERVICE_TRACKER_CUSTOMIZER customizer, SERVICE_TRACKER *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	*tracker = (SERVICE_TRACKER) apr_palloc(pool, sizeof(**tracker));
	if (!*tracker) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *tracker, serviceTracker_destroy);

		(*tracker)->context = context;
		(*tracker)->filter = apr_pstrdup(pool,filter);

		(*tracker)->pool = pool;
		(*tracker)->tracker = *tracker;
		(*tracker)->tracked = NULL;
		arrayList_create(pool, &(*tracker)->tracked);
		(*tracker)->customizer = customizer;
		(*tracker)->listener = NULL;
	}

	return status;
}

apr_status_t serviceTracker_destroy(void *trackerP) {
	SERVICE_TRACKER tracker = trackerP;
	bundleContext_removeServiceListener(tracker->context, tracker->listener);
	arrayList_destroy(tracker->tracked);

	return APR_SUCCESS;
}

celix_status_t serviceTracker_open(SERVICE_TRACKER tracker) {
	SERVICE_LISTENER listener;
	ARRAY_LIST initial = NULL;
	celix_status_t status = CELIX_SUCCESS;
	listener = (SERVICE_LISTENER) apr_palloc(tracker->pool, sizeof(*listener));
	
	status = bundleContext_getServiceReferences(tracker->context, NULL, tracker->filter, &initial);
	if (status == CELIX_SUCCESS) {
		SERVICE_REFERENCE initial_reference;
		unsigned int i;

		listener->pool = tracker->pool;
		listener->handle = tracker;
		listener->serviceChanged = (void *) serviceTracker_serviceChanged;
		status = bundleContext_addServiceListener(tracker->context, listener, tracker->filter);
		if (status == CELIX_SUCCESS) {
			tracker->listener = listener;

			for (i = 0; i < arrayList_size(initial); i++) {
				initial_reference = (SERVICE_REFERENCE) arrayList_get(initial, i);
				serviceTracker_track(tracker, initial_reference, NULL);
			}
			arrayList_clear(initial);
			arrayList_destroy(initial);

			initial = NULL;
		}
	}

	return status;
}

celix_status_t serviceTracker_close(SERVICE_TRACKER tracker) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleContext_removeServiceListener(tracker->context, tracker->listener);
	if (status == CELIX_SUCCESS) {
		ARRAY_LIST refs = serviceTracker_getServiceReferences(tracker);
		if (refs != NULL) {
			int i;
			for (i = 0; i < arrayList_size(refs); i++) {
				SERVICE_REFERENCE ref = arrayList_get(refs, i);
				status = serviceTracker_untrack(tracker, ref, NULL);
			}
		}
		arrayList_destroy(refs);
	}

	return status;
}

SERVICE_REFERENCE serviceTracker_getServiceReference(SERVICE_TRACKER tracker) {
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		return tracked->reference;
	}
	return NULL;
}

ARRAY_LIST serviceTracker_getServiceReferences(SERVICE_TRACKER tracker) {
	TRACKED tracked;
	unsigned int i;
	int size = arrayList_size(tracker->tracked);
	ARRAY_LIST references = NULL;
	arrayList_create(tracker->pool, &references);
	
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		arrayList_add(references, tracked->reference);
	}
	return references;
}

void *serviceTracker_getService(SERVICE_TRACKER tracker) {
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		return tracked->service;
	}
	return NULL;
}

ARRAY_LIST serviceTracker_getServices(SERVICE_TRACKER tracker) {
	TRACKED tracked;
	unsigned int i;
	int size = arrayList_size(tracker->tracked);
	ARRAY_LIST references = NULL;
	arrayList_create(tracker->pool, &references);
	
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		arrayList_add(references, tracked->service);
	}
	return references;
}

void *serviceTracker_getServiceByReference(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference) {
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		bool equals;
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			return tracked->service;
		}
	}
	return NULL;
}

void serviceTracker_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	SERVICE_TRACKER tracker = listener->handle;
	switch (event->type) {
		case REGISTERED:
		case MODIFIED:
			serviceTracker_track(tracker, event->reference, event);
			break;
		case UNREGISTERING:
			serviceTracker_untrack(tracker, event->reference, event);
			break;
		case MODIFIED_ENDMATCH:
			break;
	}
}

celix_status_t serviceTracker_track(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference, SERVICE_EVENT event) {
	celix_status_t status = CELIX_SUCCESS;

	TRACKED tracked = NULL;
	int found = -1;
	unsigned int i;
	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		bool equals;
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			found = 0;
			break;
		}
	}

	if (found) {
		void * service = serviceTracker_addingService(tracker, reference);
		if (service != NULL) {
			tracked = (TRACKED) malloc(sizeof(*tracked));
			tracked->reference = reference;
			tracked->service = service;
			arrayList_add(tracker->tracked, tracked);
			if (tracker->customizer != NULL) {
				tracker->customizer->addedService(tracker->customizer->handle, reference, service);
			}
		}

	} else {
		if (tracker->customizer != NULL) {
			tracker->customizer->modifiedService(tracker->customizer->handle, reference, tracked->service);
		}
	}

	return status;
}

void * serviceTracker_addingService(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference) {
    void *svc = NULL;

    if (tracker->customizer != NULL) {
    	tracker->customizer->addingService(tracker->customizer->handle, reference, &svc);
	} else {
		bundleContext_getService(tracker->context, reference, &svc);
	}

    return svc;
}

celix_status_t serviceTracker_untrack(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference, SERVICE_EVENT event) {
	celix_status_t status = CELIX_SUCCESS;
	TRACKED tracked = NULL;
	unsigned int i;
	bool result = NULL;

	for (i = 0; i < arrayList_size(tracker->tracked); i++) {
		bool equals;
		tracked = (TRACKED) arrayList_get(tracker->tracked, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			if (tracked != NULL) {
				arrayList_remove(tracker->tracked, i);
				status = bundleContext_ungetService(tracker->context, reference, &result);
			}
			if (status == CELIX_SUCCESS) {
				if (tracker->customizer != NULL) {
					tracker->customizer->removedService(tracker->customizer->handle, reference, tracked->service);
				} else {
					status = bundleContext_ungetService(tracker->tracker->context, reference, &result);
				}
				free(tracked);
				break;
			}
		}
	}

	return status;
}
