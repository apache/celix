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
#include "bundle_context_private.h"
#include "celix_array_list.h"

static celix_status_t serviceTracker_track(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static celix_status_t serviceTracker_untrack(service_tracker_pt tracker, service_reference_pt reference, service_event_pt event);
static void serviceTracker_untrackTracked(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeAddingService(celix_service_tracker_t *tracker, service_reference_pt ref, void **svcOut);
static celix_status_t serviceTracker_invokeAddService(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeModifiedService(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeRemovingService(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static void serviceTracker_checkAndInvokeSetService(void *handle, void *highestSvc, const properties_t *props, const bundle_t *bnd);
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

		celixThreadMutex_create(&(*tracker)->mutex, NULL);
        (*tracker)->currentHighestServiceId = -1;
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

	//put all tracked entries in tmp array list, so that the untrack (etc) calls are not blocked.
	int i;
    celixThreadRwlock_writeLock(&tracker->lock);
    fw_removeServiceListener(tracker->context->framework, tracker->context->bundle, tracker->listener); //remove in lock, to ensure no new tracked entry is added
    size_t size = celix_arrayList_size(tracker->trackedServices);
    celix_tracked_entry_t* trackedEntries[size];
    for (i = 0; i < arrayList_size(tracker->trackedServices); i++) {
        trackedEntries[i] = (celix_tracked_entry_t*)arrayList_get(tracker->trackedServices, i);
    }
    arrayList_clear(tracker->trackedServices);
    celixThreadRwlock_unlock(&tracker->lock);

    //loop trough tracked entries an untrack
    for (i = 0; i < size; i++) {
        serviceTracker_untrackTracked(tracker, trackedEntries[i]);
    }

    free(tracker->listener);
    tracker->listener = NULL;

	return CELIX_SUCCESS;
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
            celix_serviceTracker_useHighestRankingService(tracker, tracked->serviceName, tracker, NULL, NULL, serviceTracker_checkAndInvokeSetService);
            tracked_decreaseUse(tracked);
        }
    }

    framework_logIfError(logger, status, NULL, "Cannot track reference");

    return status;
}

static void serviceTracker_checkAndInvokeSetService(void *handle, void *highestSvc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    celix_service_tracker_t *tracker = handle;
    bool update = false;
    long svcId = -1;
    if (highestSvc == NULL) {
        //no services available anymore -> unset == call with NULL
        update = true;
    } else {
        svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);
    }
    if (svcId > 0) {
        celixThreadMutex_lock(&tracker->mutex);
        if (tracker->currentHighestServiceId != svcId) {
            tracker->currentHighestServiceId = svcId;
            update = true;
            //update
        }
        celixThreadMutex_unlock(&tracker->mutex);
    }
    if (update) {
        void *h = tracker->callbackHandle;
        if (tracker->set != NULL) {
            tracker->set(h, highestSvc);
        }
        if (tracker->setWithProperties != NULL) {
            tracker->setWithProperties(h, highestSvc, props);
        }
        if (tracker->setWithOwner != NULL) {
            tracker->setWithOwner(h, highestSvc, props, bnd);
        }
    }
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
    }
    void *handle = tracker->callbackHandle;
    if (tracker->modified != NULL) {
        tracker->modified(handle, tracked->service);
    }
    if (tracker->modifiedWithProperties != NULL) {
        tracker->modifiedWithProperties(handle, tracked->service, tracked->properties);
    }
    if (tracker->modifiedWithOwner != NULL) {
        tracker->modifiedWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
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
    }
    void *handle = tracker->callbackHandle;
    if (tracker->add != NULL) {
        tracker->add(handle, tracked->service);
    }
    if (tracker->addWithProperties != NULL) {
        tracker->addWithProperties(handle, tracked->service, tracked->properties);
    }
    if (tracker->addWithOwner != NULL) {
        tracker->addWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
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
    unsigned int size;
    const char *serviceName = NULL;

    celixThreadRwlock_writeLock(&tracker->lock);
    size = arrayList_size(tracker->trackedServices);
    for (i = 0; i < size; i++) {
        bool equals;
        celix_tracked_entry_t *tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
        serviceName = tracked->serviceName;
        serviceReference_equals(reference, tracked->reference, &equals);
        if (equals) {
            remove = tracked;
            //remove from trackedServices to prevent getting this service, but don't destroy yet, can be in use
            arrayList_remove(tracker->trackedServices, i);
            break;
        }
    }
    size = arrayList_size(tracker->trackedServices); //updated size
    celixThreadRwlock_unlock(&tracker->lock);

    serviceTracker_untrackTracked(tracker, remove);
    if (size == 0) {
        serviceTracker_checkAndInvokeSetService(tracker, NULL, NULL, NULL);
    } else {
        celix_serviceTracker_useHighestRankingService(tracker, serviceName, tracker, NULL, NULL, serviceTracker_checkAndInvokeSetService);
    }

    framework_logIfError(logger, status, NULL, "Cannot untrack reference");

    return status;
}

static void serviceTracker_untrackTracked(celix_service_tracker_t *tracker, celix_tracked_entry_t *tracked) {
    if (tracked != NULL) {
        serviceTracker_invokeRemovingService(tracker, tracked);
        celixThreadMutex_lock(&tracked->mutex);
        while (tracked->useCount > 0) {
            celixThreadCondition_wait(&tracked->useCond, &tracked->mutex);
        }
        celixThreadMutex_unlock(&tracked->mutex);

        //use count == 0, tracked entry is removed from trackedServices so there is no way it can be used again ->
        //safe to destroy.

        bundleContext_ungetServiceReference(tracker->context, tracked->reference);
        celixThreadMutex_destroy(&tracked->mutex);
        celixThreadCondition_destroy(&tracked->useCond);
        free(tracked);
    }
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
    }

    void *handle = tracker->callbackHandle;
    if (tracker->remove != NULL) {
        tracker->remove(handle, tracked->service);
    }
    if (tracker->addWithProperties != NULL) {
        tracker->removeWithProperties(handle, tracked->service, tracked->properties);
    }
    if (tracker->removeWithOwner != NULL) {
        tracker->removeWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
    }

    if (status == CELIX_SUCCESS) {
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
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.filter.filter = filter;
    opts.filter.versionRange = versionRange;
    return celix_serviceTracker_createWithOptions(ctx, &opts);
}

celix_service_tracker_t* celix_serviceTracker_createWithOptions(
        bundle_context_t *ctx,
        const celix_service_tracking_options_t *opts
) {
    celix_service_tracker_t *tracker = NULL;
    if (ctx != NULL && opts != NULL && opts->filter.serviceName != NULL) {
        tracker = calloc(1, sizeof(*tracker));
        if (tracker != NULL) {
            tracker->context = ctx;
            celixThreadRwlock_create(&tracker->lock, NULL);
            tracker->trackedServices = celix_arrayList_create();
            tracker->listener = NULL;

            //setting callbacks
            tracker->callbackHandle = opts->callbackHandle;
            tracker->set = opts->set;
            tracker->add = opts->add;
            tracker->remove = opts->remove;
            tracker->setWithProperties = opts->setWithProperties;
            tracker->addWithProperties = opts->addWithProperties;
            tracker->removeWithProperties = opts->removeWithProperties;
            tracker->setWithOwner = opts->setWithOwner;
            tracker->addWithOwner = opts->addWithOwner;
            tracker->removeWithOwner = opts->removeWithOwner;

            //highest service state
            celixThreadMutex_create(&tracker->mutex, NULL);
            tracker->currentHighestServiceId = -1;

            //setting lang
            const char *lang = opts->filter.serviceLanguage;
            if (lang == NULL || strncmp("", lang, 1) == 0) {
                lang = CELIX_FRAMEWORK_SERVICE_C_LANGUAGE;
            }

            //setting filter
            if (opts->filter.filter != NULL && opts->filter.versionRange != NULL) {
                //TODO version range
                asprintf(&tracker->filter, "&((%s=%s)(%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang, opts->filter.filter);
            } else if (opts->filter.versionRange != NULL) {
                //TODO version range
                asprintf(&tracker->filter, "&((%s=%s)(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang);
            } else if (opts->filter.filter != NULL) {
                asprintf(&tracker->filter, "(&(%s=%s)(%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang, opts->filter.filter);
            } else {
                asprintf(&tracker->filter, "(&(%s=%s)(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang);
            }

            //TODO open on other thread?
            serviceTracker_open(tracker);
        }
    } else {
        if (opts != NULL && opts->filter.serviceName == NULL) {
            framework_log(logger, OSGI_FRAMEWORK_LOG_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__,
                          "Error incorrect arguments. Missing service name.");
        } else {
            framework_log(logger, OSGI_FRAMEWORK_LOG_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error incorrect arguments. Required context (%p) or opts (%p) is NULL", ctx, opts);
        }
    }
    return tracker;
}

void celix_serviceTracker_destroy(celix_service_tracker_t *tracker) {
    if (tracker != NULL) {
        serviceTracker_close(tracker);
        serviceTracker_destroy(tracker);
    }
}


bool celix_serviceTracker_useHighestRankingService(
        celix_service_tracker_t *tracker,
        const char *serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc),
        void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
        void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)) {
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
        if (use != NULL) {
            use(callbackHandle, highest->service);
        }
        if (useWithProperties != NULL) {
            useWithProperties(callbackHandle, highest->service, highest->properties);
        }
        if (useWithOwner != NULL) {
            useWithOwner(callbackHandle, highest->service, highest->properties, highest->serviceOwner);
        }
        called = true;
        tracked_decreaseUse(highest);
    }

    return called;
}

void celix_serviceTracker_useServices(
        service_tracker_t *tracker,
        const char* serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc),
        void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
        void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)) {
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
        if (use != NULL) {
            use(callbackHandle, entry->service);
        }
        if (useWithProperties != NULL) {
            useWithProperties(callbackHandle, entry->service, entry->properties);
        }
        if (useWithOwner != NULL) {
            useWithOwner(callbackHandle, entry->service, entry->properties, entry->serviceOwner);
        }

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