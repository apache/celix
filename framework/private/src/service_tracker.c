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

ARRAY_LIST m_trackers;

void * addingService(FW_SERVICE_TRACKER, SERVICE_REFERENCE);
void track(FW_SERVICE_TRACKER, SERVICE_REFERENCE, SERVICE_EVENT);
void untrack(FW_SERVICE_TRACKER, SERVICE_REFERENCE, SERVICE_EVENT);

SERVICE_TRACKER tracker_create(BUNDLE_CONTEXT context, char * className, SERVICE_TRACKER_CUSTOMIZER customizer) {
	SERVICE_TRACKER tracker = (SERVICE_TRACKER) malloc(sizeof(*tracker));
	FW_SERVICE_TRACKER fw_tracker = (FW_SERVICE_TRACKER) malloc(sizeof(*fw_tracker));
	if (m_trackers == NULL) {
		m_trackers = arrayList_create();
	}
	tracker->context = context;
	tracker->className = className;

	fw_tracker->tracker = tracker;
	fw_tracker->tracked = arrayList_create();
	if (customizer != NULL) {
		fw_tracker->customizer = customizer;
	}

	arrayList_add(m_trackers, fw_tracker);

	return tracker;
}

void tracker_open(SERVICE_TRACKER tracker) {
	SERVICE_LISTENER listener = (SERVICE_LISTENER) malloc(sizeof(*listener));
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);

	ARRAY_LIST initial = getServiceReferences(tracker->context, tracker->className, NULL);
	SERVICE_REFERENCE initial_reference;
	unsigned int i;
	int len = strlen(tracker->className);
	len += strlen(OBJECTCLASS);
	len += 4;
	char filter[len];
	strcpy(filter, "(");
	strcat(filter, OBJECTCLASS);
	strcat(filter, "=");
	strcat(filter, tracker->className);
	strcat(filter, ")\0");

	listener->serviceChanged = (void *) tracker_serviceChanged;
	addServiceListener(tracker->context, listener, filter);
	fwTracker->listener = listener;

	for (i = 0; i < arrayList_size(initial); i++) {
		initial_reference = (SERVICE_REFERENCE) arrayList_get(initial, i);
		track(fwTracker, initial_reference, NULL);
	}
	arrayList_clear(initial);
	free(initial);

	initial = NULL;
}

void tracker_close(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	removeServiceListener(tracker->context, fwTracker->listener);

	ARRAY_LIST refs = tracker_getServiceReferences(tracker);
	if (refs != NULL) {
		int i;
		for (i = 0; i < arrayList_size(refs); i++) {
			SERVICE_REFERENCE ref = arrayList_get(refs, i);
			untrack(fwTracker, ref, NULL);
		}
	}
}

void tracker_destroy(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	removeServiceListener(tracker->context, fwTracker->listener);
	free(fwTracker->listener);
	free(fwTracker->customizer);
	tracker = NULL;
}

SERVICE_REFERENCE tracker_getServiceReference(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		return tracked->reference;
	}
	return NULL;
}

ARRAY_LIST tracker_getServiceReferences(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	int size = arrayList_size(fwTracker->tracked);
	ARRAY_LIST references = arrayList_create();
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		arrayList_add(references, tracked->reference);
	}
	return references;
}

void * tracker_getService(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
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
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	int size = arrayList_size(fwTracker->tracked);
	ARRAY_LIST references = arrayList_create();
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		arrayList_add(references, tracked->service);
	}
	return references;
}

void * tracker_getServiceByReference(SERVICE_TRACKER tracker, SERVICE_REFERENCE reference) {
	FW_SERVICE_TRACKER fwTracker = findFwServiceTracker(tracker);
	TRACKED tracked;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		if (tracked->reference == reference) {
			return tracked->service;
		}
	}
	return NULL;
}

void tracker_serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	FW_SERVICE_TRACKER fwTracker;
	unsigned int i;
	for (i = 0; i < arrayList_size(m_trackers); i++) {
		fwTracker = (FW_SERVICE_TRACKER) arrayList_get(m_trackers, i);
		if (fwTracker->listener == listener) {
			switch (event->type) {
				case REGISTERED:
				case MODIFIED:
					track(fwTracker, event->reference, event);
					break;
				case UNREGISTERING:
					untrack(fwTracker, event->reference, event);
					break;
				case MODIFIED_ENDMATCH:
					break;
			}
			break;
		}
	}
}

void track(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference, SERVICE_EVENT event ATTRIBUTE_UNUSED) {
	TRACKED tracked = NULL;
	int found = -1;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		if (tracked->reference == reference) {
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
}

void * addingService(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference) {
	if (fwTracker->customizer != NULL) {
		return fwTracker->customizer->addingService(fwTracker->customizer->handle, reference);
	} else {
		return bundleContext_getService(fwTracker->tracker->context, reference);
	}
}

void untrack(FW_SERVICE_TRACKER fwTracker, SERVICE_REFERENCE reference, SERVICE_EVENT event ATTRIBUTE_UNUSED) {
	TRACKED tracked = NULL;
	unsigned int i;
	for (i = 0; i < arrayList_size(fwTracker->tracked); i++) {
		tracked = (TRACKED) arrayList_get(fwTracker->tracked, i);
		if (tracked->reference == reference) {
			if (tracked != NULL) {
				arrayList_remove(fwTracker->tracked, i);
				//ungetService(fwTracker->tracker->context, reference);
			}
			if (fwTracker->customizer != NULL) {
				fwTracker->customizer->removedService(fwTracker->customizer->handle, reference, tracked->service);
			} else {
				bundleContext_ungetService(fwTracker->tracker->context, reference);
			}
			break;
		}
	}
}

FW_SERVICE_TRACKER findFwServiceTracker(SERVICE_TRACKER tracker) {
	FW_SERVICE_TRACKER fwTracker;
	unsigned int i;
	if (m_trackers != NULL) {
		for (i = 0; i < arrayList_size(m_trackers); i++) {
			fwTracker = (FW_SERVICE_TRACKER) arrayList_get(m_trackers, i);
			if (fwTracker->tracker == tracker) {
				return fwTracker;
			}
		}
	}
	return NULL;
}
