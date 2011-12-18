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
 *  Created on: Apr 20, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "service_tracker.h"
#include "bundle_context.h"
#include "constants.h"
#include "service_reference.h"

struct serviceTracker {
	BUNDLE_CONTEXT context;
	char * filter;

	FW_SERVICE_TRACKER fwTracker;
};

// #todo: Remove this, make SERVICE_TRACKER an ADT to keep "hidden" information
//ARRAY_LIST m_trackers;

void * addingService(FW_SERVICE_TRACKER, SERVICE_REFERENCE);
celix_status_t serviceTracker_track(FW_SERVICE_TRACKER, SERVICE_REFERENCE, SERVICE_EVENT);
celix_status_t serviceTracker_untrack(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference, SERVICE_EVENT event ATTRIBUTE_UNUSED);

celix_status_t serviceTracker_create(BUNDLE_CONTEXT context, char * service, SERVICE_TRACKER_CUSTOMIZER customizer, SERVICE_TRACKER *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	if (service == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		apr_pool_t *pool = NULL;
		//status = bundleContext_getMemoryPool(context, &pool);
		if (status == CELIX_SUCCESS) {
			int len = strlen(service) + strlen(OBJECTCLASS) + 4;
			//char *filter = apr_palloc(pool, sizeof(char) * len);
			char *filter = malloc(sizeof(char) * len);
			if (filter == NULL) {
				status = CELIX_ENOMEM;
			} else {
				strcpy(filter, "(");
				strcat(filter, OBJECTCLASS);
				strcat(filter, "=");
				strcat(filter, service);
				strcat(filter, ")\0");
				tracker_createWithFilter(context, filter, customizer, tracker);
			}
		}
	}


	return status;
}

celix_status_t tracker_createWithFilter(BUNDLE_CONTEXT context, char * filter, SERVICE_TRACKER_CUSTOMIZER customizer, SERVICE_TRACKER *tracker) {
	*tracker = (SERVICE_TRACKER) malloc(sizeof(**tracker));
	FW_SERVICE_TRACKER fw_tracker = (FW_SERVICE_TRACKER) malloc(sizeof(*fw_tracker));
	apr_pool_t *pool;
	bundleContext_getMemoryPool(context, &pool);

//	if (m_trackers == NULL) {
//		arrayList_create(pool, &m_trackers);
//	}
	(*tracker)->context = context;
	(*tracker)->filter = filter;

	fw_tracker->pool = pool;
	fw_tracker->tracker = *tracker;
	fw_tracker->tracked = NULL;
	arrayList_create(pool, &fw_tracker->tracked);
	fw_tracker->customizer = customizer;

	(*tracker)->fwTracker = fw_tracker;

//	arrayList_add(m_trackers, fw_tracker);

	return CELIX_SUCCESS;
}

celix_status_t serviceTracker_open(SERVICE_TRACKER tracker) {
	celix_status_t status = CELIX_SUCCESS;
	SERVICE_LISTENER listener = (SERVICE_LISTENER) malloc(sizeof(*listener));
//	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	ARRAY_LIST initial = NULL;

	status = bundleContext_getServiceReferences(tracker->context, NULL, tracker->filter, &initial);
	if (status == CELIX_SUCCESS) {
		SERVICE_REFERENCE initial_reference;
		unsigned int i;

		listener->pool = tracker->fwTracker->pool;
		listener->handle = tracker;
		listener->serviceChanged = (void *) tracker_serviceChanged;
		status = bundleContext_addServiceListener(tracker->context, listener, tracker->filter);
		if (status == CELIX_SUCCESS) {
			tracker->fwTracker->listener = listener;

			for (i = 0; i < arrayList_size(initial); i++) {
				initial_reference = (SERVICE_REFERENCE) arrayList_get(initial, i);
				serviceTracker_track(tracker->fwTracker, initial_reference, NULL);
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

	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	status = bundleContext_removeServiceListener(tracker->context, fwTracker->listener);
	if (status == CELIX_SUCCESS) {
		ARRAY_LIST refs = tracker_getServiceReferences(tracker);
		if (refs != NULL) {
			int i;
			for (i = 0; i < arrayList_size(refs); i++) {
				SERVICE_REFERENCE ref = arrayList_get(refs, i);
				status = serviceTracker_untrack(fwTracker, ref, NULL);
			}
		}
		arrayList_destroy(refs);
	}

	return status;
}

void tracker_destroy(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	bundleContext_removeServiceListener(tracker->context, fwTracker->listener);
//	arrayList_destroy(m_trackers);
	arrayList_destroy(fwTracker->tracked);
	free(fwTracker->listener);
	free(fwTracker);
	tracker = NULL;
	free(tracker);
}

SERVICE_REFERENCE tracker_getServiceReference(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		return tracked->reference;
	}
	return NULL;
}

ARRAY_LIST tracker_getServiceReferences(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	int size = arrayList_size(fwTracker->tracked);
	ARRAY_LIST references = NULL;
	arrayList_create(fwTracker->pool, &references);
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		arrayList_add(references, tracked->reference);
	}
	return references;
}

void * tracker_getService(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	TRACKED tracked;
	unsigned int i;
	if (fwTracker != NULL) {
		for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
			tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
			return tracked->service;
		}
	}
	return NULL;
}

ARRAY_LIST tracker_getServices(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	int size = arrayList_size(fwTracker->tracked);
	ARRAY_LIST references = NULL;
	arrayList_create(fwTracker->pool, &references);
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		arrayList_add(references, tracked->service);
	}
	return references;
}

void * tracker_getServiceByReference(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference) {
	FW_SERVICE_TRACKER fwTracker = tracker->fwTracker;
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		bool equals;
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			return tracked->service;
		}
	}
	return NULL;
}

void tracker_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	SERVICE_TRACKER tracker = listener->handle;
	switch (event->type) {
		case REGISTERED:
		case MODIFIED:
			serviceTracker_track(tracker->fwTracker, event->reference, event);
			break;
		case UNREGISTERING:
			serviceTracker_untrack(tracker->fwTracker, event->reference, event);
			break;
		case MODIFIED_ENDMATCH:
			break;
	}
}

celix_status_t serviceTracker_track(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference, SERVICE_EVENT event ATTRIBUTE_UNUSED) {
	celix_status_t status = CELIX_SUCCESS;

	TRACKED tracked = NULL;
	int found = -1;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		bool equals;
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			found = 0;
			break;
		}
	}

	if (found) {
		void * service = addingService(fwTracker, reference);
		if (service != NULL) {
			tracked = (TRACKED) malloc(sizeof(*tracked));
			tracked->reference = reference;
			tracked->service = service;
			arrayList_add(fwTracker->tracked, tracked);
			if (fwTracker->customizer != NULL) {
				fwTracker->customizer->addedService(fwTracker->customizer->handle, reference, service);
			}
		}

	} else {
		if (fwTracker->customizer != NULL) {
			fwTracker->customizer->modifiedService(fwTracker->customizer->handle, reference, tracked->service);
		}
	}

	return status;
}

void * addingService(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference) {
    void *svc = NULL;

    if (fwTracker->customizer != NULL) {
		fwTracker->customizer->addingService(fwTracker->customizer->handle, reference, &svc);
	} else {
		bundleContext_getService(fwTracker->tracker->context, reference, &svc);
	}

    return svc;
}

celix_status_t serviceTracker_untrack(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference, SERVICE_EVENT event ATTRIBUTE_UNUSED) {
	celix_status_t status = CELIX_SUCCESS;
	TRACKED tracked = NULL;
	unsigned int i;
	bool result = NULL;

	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		bool equals;
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			if (tracked != NULL) {
				arrayList_remove(fwTracker->tracked, i);
				status = bundleContext_ungetService(fwTracker->tracker->context, reference, &result);
			}
			if (status == CELIX_SUCCESS) {
				if (fwTracker->customizer != NULL) {
					fwTracker->customizer->removedService(fwTracker->customizer->handle, reference, tracked->service);
				} else {
					status = bundleContext_ungetService(fwTracker->tracker->context, reference, &result);
				}
				free(tracked);
				break;
			}
		}
	}

	return status;
}

//FW_SERVICE_TRACKER findFwServiceTracker(SERVICE_TRACKER tracker) {
//	FW_SERVICE_TRACKER fwTracker;
//	unsigned int i;
//	if (m_trackers != NULL) {
//		for (i = 0; i < arrayList_size(m_trackers); i++) {
//			fwTracker = (FW_SERVICE_TRACKER) arrayList_get(m_trackers, i);
//			if (fwTracker->tracker == tracker) {
//				return fwTracker;
//			}
//		}
//	}
//	return NULL;
//}
