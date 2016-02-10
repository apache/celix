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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <service_reference_private.h>
#include <framework_private.h>
#include <assert.h>

#include "service_tracker_private.h"
#include "bundle_context.h"
#include "constants.h"
#include "service_reference.h"
#include "celix_log.h"

static celix_status_t serviceTracker_invokeAddingService(service_tracker_pt tracker, service_reference_pt reference,
                                                         void **service);
static celix_status_t serviceTracker_track(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static celix_status_t serviceTracker_invokeAddService(service_tracker_pt tracker, service_reference_pt ref, void *service);
static celix_status_t serviceTracker_invokeModifiedService(service_tracker_pt tracker, service_reference_pt ref, void *service);

static celix_status_t serviceTracker_invokeRemovingService(service_tracker_pt tracker, service_reference_pt ref,
                                                           void *service);

celix_status_t serviceTracker_create(bundle_context_pt context, char * service, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	if (service == NULL || *tracker != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		if (status == CELIX_SUCCESS) {
			char filter[512];
			snprintf(filter, sizeof(filter), "(%s=%s)", OSGI_FRAMEWORK_OBJECTCLASS, service);
            serviceTracker_createWithFilter(context, filter, customizer, tracker);
		}
	}

	framework_logIfError(logger, status, NULL, "Cannot create service tracker");

	return status;
}

celix_status_t serviceTracker_createWithFilter(bundle_context_pt context, char * filter, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	*tracker = (service_tracker_pt) malloc(sizeof(**tracker));
	if (!*tracker) {
		status = CELIX_ENOMEM;
	} else {
		(*tracker)->context = context;
		(*tracker)->filter = strdup(filter);

		(*tracker)->tracker = *tracker;
        celixThreadRwlock_create(&(*tracker)->lock, NULL);
		(*tracker)->trackedServices = NULL;
		arrayList_create(&(*tracker)->trackedServices);
		(*tracker)->customizer = customizer;
		(*tracker)->listener = NULL;
	}

	framework_logIfError(logger, status, NULL, "Cannot create service tracker [filter=%s]", filter);

	return status;
}

celix_status_t serviceTracker_destroy(service_tracker_pt tracker) {
	if (tracker->listener != NULL) {
		bundleContext_removeServiceListener(tracker->context, tracker->listener);
	}
	if (tracker->customizer != NULL) {
	    serviceTrackerCustomizer_destroy(tracker->customizer);
	}

    celixThreadRwlock_writeLock(&tracker->lock);
	arrayList_destroy(tracker->trackedServices);
    celixThreadRwlock_unlock(&tracker->lock);


	if (tracker->listener != NULL) {
		free (tracker->listener);
	}

    celixThreadRwlock_destroy(&tracker->lock);

	free(tracker->filter);
	free(tracker);

	return CELIX_SUCCESS;
}

celix_status_t serviceTracker_open(service_tracker_pt tracker) {
	service_listener_pt listener;
	array_list_pt initial = NULL;
	celix_status_t status = CELIX_SUCCESS;
	listener = (service_listener_pt) malloc(sizeof(*listener));
	
	status = bundleContext_getServiceReferences(tracker->context, NULL, tracker->filter, &initial);
	if (status == CELIX_SUCCESS) {
		service_reference_pt initial_reference;
		unsigned int i;

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

	framework_logIfError(logger, status, NULL, "Cannot open tracker");

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

	framework_logIfError(logger, status, NULL, "Cannot close tracker");

	return status;
}

service_reference_pt serviceTracker_getServiceReference(service_tracker_pt tracker) {
	tracked_pt tracked;
    service_reference_pt result = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->lock);
	for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
		result = tracked->reference;
        break;
	}
    celixThreadRwlock_unlock(&tracker->lock);

	return result;
}

array_list_pt serviceTracker_getServiceReferences(service_tracker_pt tracker) {
	tracked_pt tracked;
	unsigned int i;
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadRwlock_readLock(&tracker->lock);
	for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
		arrayList_add(references, tracked->reference);
	}
    celixThreadRwlock_unlock(&tracker->lock);

	return references;
}

void *serviceTracker_getService(service_tracker_pt tracker) {
	tracked_pt tracked;
    void *service = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
		service = tracked->service;
        break;
	}
    celixThreadRwlock_unlock(&tracker->lock);

    return service;
}

array_list_pt serviceTracker_getServices(service_tracker_pt tracker) {
	tracked_pt tracked;
	unsigned int i;
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
		arrayList_add(references, tracked->service);
	}
    celixThreadRwlock_unlock(&tracker->lock);

    return references;
}

void *serviceTracker_getServiceByReference(service_tracker_pt tracker, service_reference_pt reference) {
	tracked_pt tracked;
    void *service = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->lock);
	for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		bool equals = false;
		tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
		serviceReference_equals(reference, tracked->reference, &equals);
		if (equals) {
			service = tracked->service;
            break;
		}
	}
    celixThreadRwlock_unlock(&tracker->lock);

	return service;
}

void serviceTracker_serviceChanged(service_listener_pt listener, service_event_pt event) {
	service_tracker_pt tracker = listener->handle;
	switch (event->type) {
		case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
			serviceTracker_track(tracker, event->reference, event);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED:
			serviceTracker_track(tracker, event->reference, event);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
			serviceTracker_untrack(tracker, event->reference, event);
			break;
		case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH:
            //TODO
			break;
	}
}

static celix_status_t serviceTracker_track(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

    tracked_pt tracked = NULL;
    bool found = false;
    unsigned int i;
    
    bundleContext_retainServiceReference(tracker->context, reference);

    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
        bool equals = false;
        tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
        status = serviceReference_equals(reference, tracked->reference, &equals);
        if (status != CELIX_SUCCESS) {
            break;
        }
        if (equals) {
            found = true;
            break;
        }
    }
    celixThreadRwlock_unlock(&tracker->lock);

    if (status == CELIX_SUCCESS && !found /*new*/) {
        void * service = NULL;
        status = serviceTracker_invokeAddingService(tracker, reference, &service);
        if (status == CELIX_SUCCESS) {
            if (service != NULL) {
                tracked = (tracked_pt) calloc(1, sizeof (*tracked));
                assert(reference != NULL);
                tracked->reference = reference;
                tracked->service = service;

                celixThreadRwlock_writeLock(&tracker->lock);
                arrayList_add(tracker->trackedServices, tracked);
                celixThreadRwlock_unlock(&tracker->lock);

                serviceTracker_invokeAddService(tracker, reference, service);
            }
        }

    } else {
        status = serviceTracker_invokeModifiedService(tracker, reference, tracked->service);
    }

    framework_logIfError(logger, status, NULL, "Cannot track reference");

    return status;
}

static celix_status_t serviceTracker_invokeModifiedService(service_tracker_pt tracker, service_reference_pt ref, void *service) {
    celix_status_t status = CELIX_SUCCESS;
    if (tracker->customizer != NULL) {
        void *handle = NULL;
        modified_callback_pt function = NULL;

        serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
        serviceTrackerCustomizer_getModifiedFunction(tracker->customizer, &function);

        if (function != NULL) {
            function(handle, ref, service);
        }
    }
    return status;
}

static celix_status_t serviceTracker_invokeAddService(service_tracker_pt tracker, service_reference_pt ref, void *service) {
    celix_status_t status = CELIX_SUCCESS;
    if (tracker->customizer != NULL) {
        void *handle = NULL;
        added_callback_pt function = NULL;

        serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
        serviceTrackerCustomizer_getAddedFunction(tracker->customizer, &function);
        if (function != NULL) {
            function(handle, ref, service);
        }
    }
    return status;
}

static celix_status_t serviceTracker_invokeAddingService(service_tracker_pt tracker, service_reference_pt reference,
                                                          void **service) {
	celix_status_t status = CELIX_SUCCESS;

    if (tracker->customizer != NULL) {
    	void *handle = NULL;
		adding_callback_pt function = NULL;

		status =  serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);

        if (status == CELIX_SUCCESS) {
            status = serviceTrackerCustomizer_getAddingFunction(tracker->customizer, &function);
        }

		if (status == CELIX_SUCCESS) {
            if (function != NULL) {
                status = function(handle, reference, service);
            } else {
                status = bundleContext_getService(tracker->context, reference, service);
            }
		}
	} else {
        status = bundleContext_getService(tracker->context, reference, service);
    }

    framework_logIfError(logger, status, NULL, "Cannot handle addingService");

    return status;
}

static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;
    tracked_pt tracked = NULL;
    unsigned int i;
    bool found = false;

    celixThreadRwlock_writeLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
        bool equals;
        tracked = (tracked_pt) arrayList_get(tracker->trackedServices, i);
        serviceReference_equals(reference, tracked->reference, &equals);
        if (equals) {
            found = true;
            arrayList_remove(tracker->trackedServices, i);
            break;
        }
    }
    celixThreadRwlock_unlock(&tracker->lock);

    if (found && tracked != NULL) {
        serviceTracker_invokeRemovingService(tracker, tracked->reference, tracked->service);
        free(tracked);
        bundleContext_ungetServiceReference(tracker->context, reference);
    }
   
    framework_logIfError(logger, status, NULL, "Cannot untrack reference");

    return status;
}

static celix_status_t serviceTracker_invokeRemovingService(service_tracker_pt tracker, service_reference_pt ref,  void *service) {
    celix_status_t status = CELIX_SUCCESS;
    bool ungetSuccess = true;
    if (tracker->customizer != NULL) {
        void *handle = NULL;
        removed_callback_pt function = NULL;

        serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
        serviceTrackerCustomizer_getRemovedFunction(tracker->customizer, &function);

        if (function != NULL) {
            status = function(handle, ref, service);
        }
        if (status == CELIX_SUCCESS) {
            status = bundleContext_ungetService(tracker->context, ref, &ungetSuccess);
        }
    } else {
        status = bundleContext_ungetService(tracker->context, ref, &ungetSuccess);
    }

    if (!ungetSuccess) {
        framework_log(logger, OSGI_FRAMEWORK_LOG_ERROR, __FUNCTION__, __FILE__, __LINE__, "Error ungetting service");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}
