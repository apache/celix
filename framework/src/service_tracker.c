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
#include "service_tracker_customizer_private.h"

static celix_status_t serviceTracker_track(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static celix_status_t serviceTracker_invokeAddingService(celix_service_tracker_t *tracker, service_reference_pt ref, void **svcOut);
static celix_status_t serviceTracker_invokeAddService(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeModifiedService(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeRemovingService(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static inline void tracked_increaseUse(celix_tracked_entry_t *tracked);
static inline void tracked_decreaseUse(celix_tracked_entry_t *tracked);

celix_status_t serviceTracker_create(bundle_context_pt context, const char * service, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
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

celix_status_t serviceTracker_createWithFilter(bundle_context_pt context, const char * filter, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;
	*tracker = (service_tracker_pt) calloc(1, sizeof(**tracker));
	if (!*tracker) {
		status = CELIX_ENOMEM;
	} else {
		(*tracker)->context = context;
		(*tracker)->filter = strdup(filter);

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
	
	status = bundleContext_getServiceReferences(tracker->context, NULL, tracker->filter, &initial); //REF COUNT to 1
	if (status == CELIX_SUCCESS && listener != NULL) {
		service_reference_pt initial_reference;
		unsigned int i;

		listener->handle = tracker;
		listener->serviceChanged = (void *) serviceTracker_serviceChanged;
		status = bundleContext_addServiceListener(tracker->context, listener, tracker->filter);
		if (status == CELIX_SUCCESS) {
			tracker->listener = listener;

			for (i = 0; i < arrayList_size(initial); i++) {
				initial_reference = (service_reference_pt) arrayList_get(initial, i);
				serviceTracker_track(tracker, initial_reference, NULL); //REF COUNT to 2
                bundleContext_ungetServiceReference(tracker->context, initial_reference); //REF COUNT to 1
			}

			arrayList_destroy(initial);

			initial = NULL;
		}
	}

	if(status != CELIX_SUCCESS && listener != NULL){
		free(listener);
	}

	framework_logIfError(logger, status, NULL, "Cannot open tracker");

	return status;
}

celix_status_t serviceTracker_close(service_tracker_pt tracker) {
	celix_status_t status = CELIX_SUCCESS;

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
    if (status == CELIX_SUCCESS) {
        status = bundleContext_removeServiceListener(tracker->context, tracker->listener);
        if(status == CELIX_SUCCESS) {
            free(tracker->listener);
            tracker->listener = NULL;
        }
    }
	framework_logIfError(logger, status, NULL, "Cannot close tracker");

	return status;
}

service_reference_pt serviceTracker_getServiceReference(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked
	celix_tracked_entry_t *tracked;
    service_reference_pt result = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->lock);
	for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
		result = tracked->reference;
        break;
	}
    celixThreadRwlock_unlock(&tracker->lock);

	return result;
}

array_list_pt serviceTracker_getServiceReferences(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked
    celix_tracked_entry_t *tracked;
	unsigned int i;
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadRwlock_readLock(&tracker->lock);
	for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
		arrayList_add(references, tracked->reference);
	}
    celixThreadRwlock_unlock(&tracker->lock);

	return references;
}

void *serviceTracker_getService(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked
    celix_tracked_entry_t* tracked;
    void *service = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
		service = tracked->service;
        break;
	}
    celixThreadRwlock_unlock(&tracker->lock);

    return service;
}

array_list_pt serviceTracker_getServices(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked, also make locked variant
    celix_tracked_entry_t *tracked;
	unsigned int i;
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
		arrayList_add(references, tracked->service);
	}
    celixThreadRwlock_unlock(&tracker->lock);

    return references;
}

void *serviceTracker_getServiceByReference(service_tracker_pt tracker, service_reference_pt reference) {
    //TODO deprecated warning -> not locked
    celix_tracked_entry_t *tracked;
    void *service = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->lock);
	for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
		bool equals = false;
		tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
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

    celix_tracked_entry_t *found = NULL;
    unsigned int i;
    
    bundleContext_retainServiceReference(tracker->context, reference);

    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
        bool equals = false;
        celix_tracked_entry_t *visit = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
        serviceReference_equals(reference, visit->reference, &equals);
        if (equals) {
            found = visit;
            tracked_increaseUse(found);
            break;
        }
    }
    celixThreadRwlock_unlock(&tracker->lock);

    if (found != NULL) {
        status = serviceTracker_invokeModifiedService(tracker, found);
        tracked_decreaseUse(found);
    } else if (status == CELIX_SUCCESS && found == NULL) {
        void *service = NULL;
        status = serviceTracker_invokeAddingService(tracker, reference, &service);
        if (status == CELIX_SUCCESS && service != NULL) {
            celix_tracked_entry_t *tracked = (celix_tracked_entry_t*) calloc(1, sizeof (*tracked));
            assert(reference != NULL);
            tracked->reference = reference;
            tracked->service = service;
            service_registration_t *reg = NULL;
            properties_t *props = NULL;
            bundle_t *bnd = NULL;
            serviceReference_getProperty(reference, OSGI_FRAMEWORK_OBJECTCLASS, &tracked->serviceName);
            serviceReference_getBundle(reference, &bnd);
            serviceReference_getServiceRegistration(reference, &reg);
            if (reg != NULL) {
                serviceRegistration_getProperties(reg, &props);
            }
            tracked->properties = props;
            tracked->serviceOwner = bnd;
            tracked->useCount = 1; //invoke add

            celixThreadMutex_create(&tracked->mutex, NULL);
            celixThreadCondition_init(&tracked->useCond, NULL);

            celixThreadRwlock_writeLock(&tracker->lock);
            arrayList_add(tracker->trackedServices, tracked);
            celixThreadRwlock_unlock(&tracker->lock);

            serviceTracker_invokeAddService(tracker, tracked);
            tracked_decreaseUse(tracked);
        }
    }

    framework_logIfError(logger, status, NULL, "Cannot track reference");

    return status;
}

static celix_status_t serviceTracker_invokeModifiedService(service_tracker_pt tracker, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;
    if (tracker->customizer != NULL) {
        void *handle = NULL;
        modified_callback_pt function = NULL;

        serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
        serviceTrackerCustomizer_getModifiedFunction(tracker->customizer, &function);

        if (function != NULL) {
            function(handle, tracked->reference, tracked->service);
        }
        if (tracker->modified != NULL) {
            tracker->modified(tracker->callbackHandle, tracked->service, tracked->properties, tracked->serviceOwner);
        }
    }
    return status;
}


static celix_status_t serviceTracker_invokeAddService(service_tracker_pt tracker, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;
    if (tracker->customizer != NULL) {
        void *handle = NULL;
        added_callback_pt function = NULL;

        serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
        serviceTrackerCustomizer_getAddedFunction(tracker->customizer, &function);
        if (function != NULL) {
            function(handle, tracked->reference, tracked->service);
        }
        if (tracker->add != NULL) {
            tracker->add(tracker->callbackHandle, tracked->service, tracked->properties, tracked->serviceOwner);
        }
    }
    return status;
}

static celix_status_t serviceTracker_invokeAddingService(celix_service_tracker_t *tracker, service_reference_pt ref, void **svcOut) {
	celix_status_t status = CELIX_SUCCESS;

    if (tracker->customizer != NULL) {
        void *handle = NULL;
        adding_callback_pt function = NULL;

        status = serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);

        if (status == CELIX_SUCCESS) {
            status = serviceTrackerCustomizer_getAddingFunction(tracker->customizer, &function);
        }

        if (status == CELIX_SUCCESS && function != NULL) {
            status = function(handle, ref, svcOut);
        } else if (status == CELIX_SUCCESS) {
            status = bundleContext_getService(tracker->context, ref, svcOut);
        }
    } else {
        status = bundleContext_getService(tracker->context, ref, svcOut);
    }

    framework_logIfError(logger, status, NULL, "Cannot handle addingService");

    return status;
}

static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;
    celix_tracked_entry_t *remove = NULL;
    unsigned int i;

    celixThreadRwlock_writeLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
        bool equals;
        celix_tracked_entry_t *tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
        serviceReference_equals(reference, tracked->reference, &equals);
        if (equals) {
            remove = tracked;
            //remove from trackedServices to prevent getting this service, but don't destroy yet, can be in use
            arrayList_remove(tracker->trackedServices, i);
            break;
        }
    }
    celixThreadRwlock_unlock(&tracker->lock);

    serviceTracker_invokeRemovingService(tracker, remove);

    celixThreadMutex_lock(&remove->mutex);
    while (remove->useCount > 0) {
        celixThreadCondition_wait(&remove->useCond, &remove->mutex);
    }
    celixThreadMutex_unlock(&remove->mutex);

    //use count == 0, tracked entry is removed from trackedServices so there is no way it can be used again ->
    //safe to destroy.

    bundleContext_ungetServiceReference(tracker->context, reference);
    celixThreadMutex_destroy(&remove->mutex);
    celixThreadCondition_destroy(&remove->useCond);
    free(remove);

    framework_logIfError(logger, status, NULL, "Cannot untrack reference");

    return status;
}

static celix_status_t serviceTracker_invokeRemovingService(service_tracker_pt tracker, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;
    bool ungetSuccess = true;
    if (tracker->customizer != NULL) {
        void *handle = NULL;
        removed_callback_pt function = NULL;

        serviceTrackerCustomizer_getHandle(tracker->customizer, &handle);
        serviceTrackerCustomizer_getRemovedFunction(tracker->customizer, &function);

        if (function != NULL) {
            status = function(handle, tracked->reference, tracked->service);
        }
        if (tracker->remove != NULL) {
            tracker->remove(tracker->callbackHandle, tracked->service, tracked->properties, tracked->serviceOwner);
        }
        if (status == CELIX_SUCCESS) {
            status = bundleContext_ungetService(tracker->context, tracked->reference, &ungetSuccess);
        }
    } else {
        status = bundleContext_ungetService(tracker->context, tracked->reference, &ungetSuccess);
    }

    if (!ungetSuccess) {
        framework_log(logger, OSGI_FRAMEWORK_LOG_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error ungetting service");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}



/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/


celix_service_tracker_t* celix_serviceTracker_create(
        bundle_context_t *ctx,
        const char *serviceName,
        const char *versionRange,
        const char *filter) {
    celix_service_tracker_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.serviceName = serviceName;
    opts.filter = filter;
    opts.versionRange = versionRange;
    return celix_serviceTracker_createWithOptions(ctx, &opts);
}

celix_service_tracker_t* celix_serviceTracker_createWithOptions(
        bundle_context_t *ctx,
        const celix_service_tracker_options_t *opts
) {
    celix_service_tracker_t *tracker = NULL;
    if (ctx != NULL && opts != NULL && opts->serviceName != NULL) {
        tracker = calloc(1, sizeof(*tracker));
        if (tracker != NULL) {
            tracker->context = ctx;
            celixThreadRwlock_create(&tracker->lock, NULL);
            tracker->trackedServices = celix_arrayList_create();
            tracker->listener = NULL;

            tracker->callbackHandle = opts->callbackHandle;
            tracker->add = opts->add;
            tracker->remove = opts->remove;
            tracker->modified = opts->modified;

            if (opts->filter != NULL && opts->versionRange != NULL) {
                //TODO version range
                asprintf(&tracker->filter, "&((%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->serviceName, opts->filter);
            } else if (opts->versionRange != NULL) {
                //TODO version range
                asprintf(&tracker->filter, "(%s=%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->serviceName);
            } else if (opts->filter != NULL) {
                asprintf(&tracker->filter, "(&(%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->serviceName, opts->filter);
            } else {
                asprintf(&tracker->filter, "(%s=%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->serviceName);
            }

        }
    } else {
        framework_log(logger, OSGI_FRAMEWORK_LOG_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error incorrect arguments");
    }
    return tracker;
}



bool celix_serviceTracker_useHighestRankingService(
        celix_service_tracker_t *tracker,
        const char *serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc, const properties_t *props, const bundle_t *owner)) {
    bool called = false;
    celix_tracked_entry_t *tracked = NULL;
    celix_tracked_entry_t *highest = NULL;
    long highestRank = 0;
    unsigned int i;

    //first lock tracker and get highest tracked entry
    celixThreadRwlock_readLock(&tracker->lock);
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
        tracked = (celix_tracked_entry_t *) arrayList_get(tracker->trackedServices, i);
        if (serviceName != NULL && tracked->serviceName != NULL && strncmp(tracked->serviceName, serviceName, 10*1024) == 0) {
            const char *val = properties_getWithDefault(tracked->properties, OSGI_FRAMEWORK_SERVICE_RANKING, "0");
            long rank = strtol(val, NULL, 10);
            if (highest == NULL || rank > highestRank) {
                highest = tracked;
            }
        }
    }
    if (highest != NULL) {
        //highest found lock tracked entry and increase use count
        tracked_increaseUse(highest);
    }
    //unlock tracker so that the tracked entry can be removed from the trackedServices list if unregistered.
    celixThreadRwlock_unlock(&tracker->lock);

    if (highest != NULL) {
        //got service, call, decrease use count an signal useCond after.
        use(callbackHandle, highest->service, highest->properties, highest->serviceOwner);
        called = true;
        tracked_decreaseUse(highest);
    }

    return called;
}

void celix_serviceTracker_useServices(
        service_tracker_t *tracker,
        const char* serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc, const properties_t *props, const bundle_t *owner)) {
    int i;

    //first lock tracker, get tracked entries and increase use count
    celixThreadRwlock_readLock(&tracker->lock);
    size_t size = celix_arrayList_size(tracker->trackedServices);
    celix_tracked_entry_t* entries[size];
    for (i = 0; i < size; i++) {
        celix_tracked_entry_t *tracked = (celix_tracked_entry_t *) arrayList_get(tracker->trackedServices, i);
        tracked_increaseUse(tracked);
        entries[i] = tracked;
    }
    //unlock tracker so that the tracked entry can be removed from the trackedServices list if unregistered.
    celixThreadRwlock_unlock(&tracker->lock);

    //then use entries and decrease use count
    for (i = 0; i < size; i++) {
        celix_tracked_entry_t *entry = entries[i];
        //got service, call, decrease use count an signal useCond after.
        use(callbackHandle, entry->service, entry->properties, entry->serviceOwner);

        tracked_decreaseUse(entry);
    }
}

static inline void tracked_increaseUse(celix_tracked_entry_t *tracked) {
    celixThreadMutex_lock(&tracked->mutex);
    tracked->useCount += 1;
    celixThreadMutex_unlock(&tracked->mutex);
}

static inline void tracked_decreaseUse(celix_tracked_entry_t *tracked) {
    celixThreadMutex_lock(&tracked->mutex);
    tracked->useCount -= 1;
    if (tracked->useCount == 0) {
        celixThreadCondition_signal(&tracked->useCond);
    }
    celixThreadMutex_unlock(&tracked->mutex);
}