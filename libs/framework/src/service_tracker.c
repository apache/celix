/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "service_reference_private.h"
#include "framework_private.h"
#include <assert.h>
#include <unistd.h>
#include <celix_api.h>
#include <limits.h>

#include "service_tracker_private.h"
#include "bundle_context.h"
#include "celix_constants.h"
#include "service_reference.h"
#include "celix_log.h"
#include "bundle_context_private.h"
#include "celix_array_list.h"

static celix_status_t serviceTracker_track(service_tracker_t *tracker, service_reference_pt reference, celix_service_event_t *event);
static celix_status_t serviceTracker_untrack(service_tracker_t *tracker, service_reference_pt reference);
static void serviceTracker_untrackTracked(service_tracker_t *tracker, celix_tracked_entry_t *tracked, int trackedSize, bool set);
static celix_status_t serviceTracker_invokeAddingService(service_tracker_t *tracker, service_reference_pt ref, void **svcOut);
static celix_status_t serviceTracker_invokeAddService(service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeRemovingService(service_tracker_t *tracker, celix_tracked_entry_t *tracked);
static void serviceTracker_checkAndInvokeSetService(void *handle, void *highestSvc, const properties_t *props, const bundle_t *bnd);

static void serviceTracker_serviceChanged(void *handle, celix_service_event_t *event);


static inline celix_tracked_entry_t* tracked_create(service_reference_pt ref, void *svc, celix_properties_t *props, celix_bundle_t *bnd) {
    celix_tracked_entry_t *tracked = calloc(1, sizeof(*tracked));
    tracked->reference = ref;
    tracked->service = svc;
    tracked->serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    tracked->serviceRanking = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, 0);
    tracked->properties = props;
    tracked->serviceOwner = bnd;
    tracked->serviceName = celix_properties_get(props, OSGI_FRAMEWORK_OBJECTCLASS, "Error");

    tracked->useCount = 1;
    celixThreadMutex_create(&tracked->mutex, NULL);
    celixThreadCondition_init(&tracked->useCond, NULL);
    return tracked;
}

static inline void tracked_retain(celix_tracked_entry_t *tracked) {
    celixThreadMutex_lock(&tracked->mutex);
    tracked->useCount += 1;
    celixThreadMutex_unlock(&tracked->mutex);
}

static inline void tracked_release(celix_tracked_entry_t *tracked) {
    celixThreadMutex_lock(&tracked->mutex);
    assert(tracked->useCount > 0);
    tracked->useCount -= 1;
    celixThreadCondition_signal(&tracked->useCond);
    celixThreadMutex_unlock(&tracked->mutex);
}

static inline void tracked_waitAndDestroy(celix_tracked_entry_t *tracked) {
    celixThreadMutex_lock(&tracked->mutex);
    while (tracked->useCount != 0) {
        celixThreadCondition_wait(&tracked->useCond, &tracked->mutex);
    }
    celixThreadMutex_unlock(&tracked->mutex);

    //destroy
    celixThreadMutex_destroy(&tracked->mutex);
    celixThreadCondition_destroy(&tracked->useCond);
    free(tracked);
}

celix_status_t serviceTracker_create(bundle_context_pt context, const char * service, service_tracker_customizer_pt customizer, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;

	if (service == NULL || *tracker != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
        char *filter = NULL;
        asprintf(&filter, "(%s=%s)", OSGI_FRAMEWORK_OBJECTCLASS, service);
        serviceTracker_createWithFilter(context, filter, customizer, tracker);
        free(filter);
	}

	framework_logIfError(context->framework->logger, status, NULL, "Cannot create service tracker");

	return status;
}

celix_status_t serviceTracker_createWithFilter(bundle_context_pt context, const char * filter, service_tracker_customizer_pt customizer, service_tracker_pt *out) {
	service_tracker_t* tracker = calloc(1, sizeof(*tracker));
	*out = tracker;
	tracker->state = CELIX_SERVICE_TRACKER_CLOSED;
    tracker->context = context;
    tracker->filter = celix_utils_strdup(filter);
    tracker->customizer = *customizer;
    free(customizer);

    celixThreadMutex_create(&tracker->closeSync.mutex, NULL);
    celixThreadCondition_init(&tracker->closeSync.cond, NULL);

    celixThreadMutex_create(&tracker->mutex, NULL);
    celixThreadCondition_init(&tracker->condTracked, NULL);
    celixThreadCondition_init(&tracker->condUntracking, NULL);
    tracker->trackedServices = celix_arrayList_create();
    tracker->untrackingServices = celix_arrayList_create();

    tracker->currentHighestServiceId = -1;

    tracker->listener.handle = tracker;
    tracker->listener.serviceChanged = (void *) serviceTracker_serviceChanged;

	return CELIX_SUCCESS;
}

celix_status_t serviceTracker_destroy(service_tracker_pt tracker) {
    free(tracker->serviceName);
	free(tracker->filter);
    celixThreadMutex_destroy(&tracker->closeSync.mutex);
    celixThreadCondition_destroy(&tracker->closeSync.cond);
    celixThreadMutex_destroy(&tracker->mutex);
    celixThreadCondition_destroy(&tracker->condTracked);
    celixThreadCondition_destroy(&tracker->condUntracking);
    celix_arrayList_destroy(tracker->trackedServices);
    celix_arrayList_destroy(tracker->untrackingServices);
    free(tracker);
	return CELIX_SUCCESS;
}

celix_status_t serviceTracker_open(service_tracker_pt tracker) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&tracker->mutex);
    bool needOpening = false;
    switch (tracker->state) {
        case CELIX_SERVICE_TRACKER_OPENING:
            celix_bundleContext_log(tracker->context, CELIX_LOG_LEVEL_WARNING, "Cannot open opening tracker");
            status = CELIX_ILLEGAL_STATE;
            break;
        case CELIX_SERVICE_TRACKER_OPEN:
            //already open, silently ignore.
            break;
        case CELIX_SERVICE_TRACKER_CLOSED:
            tracker->state = CELIX_SERVICE_TRACKER_OPENING;
            needOpening = true;
            break;
        case CELIX_SERVICE_TRACKER_CLOSING:
            celix_bundleContext_log(tracker->context, CELIX_LOG_LEVEL_WARNING, "Cannot open closing tracker");
            status = CELIX_ILLEGAL_STATE;
            break;
    }
    celixThreadMutex_unlock(&tracker->mutex);

    if (needOpening) {
        bundleContext_addServiceListener(tracker->context, &tracker->listener, tracker->filter);
        celixThreadMutex_lock(&tracker->mutex);
        tracker->state = CELIX_SERVICE_TRACKER_OPEN;
        celixThreadMutex_unlock(&tracker->mutex);
    }

    return status;
}

celix_status_t serviceTracker_close(service_tracker_t* tracker) {
	//put all tracked entries in tmp array list, so that the untrack (etc) calls are not blocked.
    //set state to close to prevent service listener events

    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&tracker->mutex);
    bool needClosing = false;
    switch (tracker->state) {
        case CELIX_SERVICE_TRACKER_OPENING:
            celix_bundleContext_log(tracker->context, CELIX_LOG_LEVEL_WARNING, "Cannot close opening tracker");
            status = CELIX_ILLEGAL_STATE;
            break;
        case CELIX_SERVICE_TRACKER_OPEN:
            tracker->state = CELIX_SERVICE_TRACKER_CLOSING;
            needClosing = true;
            break;
        case CELIX_SERVICE_TRACKER_CLOSING:
            celix_bundleContext_log(tracker->context, CELIX_LOG_LEVEL_WARNING, "Cannot close closing tracker");
            status = CELIX_ILLEGAL_STATE;
            break;
        case CELIX_SERVICE_TRACKER_CLOSED:
            //silently ignore
            break;
    }
    celixThreadMutex_unlock(&tracker->mutex);

    if (needClosing) {
        //indicate that the service tracking is closing and wait for the still pending service registration events.
        celixThreadMutex_lock(&tracker->closeSync.mutex);
        tracker->closeSync.closing = true;
        while (tracker->closeSync.activeCalls > 0) {
            celixThreadCondition_wait(&tracker->closeSync.cond, &tracker->closeSync.mutex);
        }
        celixThreadMutex_unlock(&tracker->closeSync.mutex);

        int nrOfTrackedEntries;
        do {
            celixThreadMutex_lock(&tracker->mutex);
            celix_tracked_entry_t *tracked = NULL;
            nrOfTrackedEntries = celix_arrayList_size(tracker->trackedServices);
            if (nrOfTrackedEntries > 0) {
                tracked = celix_arrayList_get(tracker->trackedServices, 0);
                celix_arrayList_removeAt(tracker->trackedServices, 0);
                celix_arrayList_add(tracker->untrackingServices, tracked);
            }
            celixThreadMutex_unlock(&tracker->mutex);

            if (tracked != NULL) {
                int currentSize = nrOfTrackedEntries - 1;
                serviceTracker_untrackTracked(tracker, tracked, currentSize, currentSize == 0);
                celixThreadMutex_lock(&tracker->mutex);
                celix_arrayList_remove(tracker->untrackingServices, tracked);
                celixThreadCondition_broadcast(&tracker->condUntracking);
                celixThreadMutex_unlock(&tracker->mutex);
            }


            celixThreadMutex_lock(&tracker->mutex);
            nrOfTrackedEntries = celix_arrayList_size(tracker->trackedServices);
            celixThreadMutex_unlock(&tracker->mutex);
        } while (nrOfTrackedEntries > 0);


        fw_removeServiceListener(tracker->context->framework, tracker->context->bundle, &tracker->listener);

        celixThreadMutex_lock(&tracker->mutex);
        tracker->state = CELIX_SERVICE_TRACKER_CLOSED;
        celixThreadMutex_unlock(&tracker->mutex);
    }

	return status;
}

service_reference_pt serviceTracker_getServiceReference(service_tracker_t* tracker) {
    //TODO deprecated warning -> not locked

    service_reference_pt result = NULL;

    celixThreadMutex_lock(&tracker->mutex);
    if(celix_arrayList_size(tracker->trackedServices) > 0) {
        celix_tracked_entry_t *tracked = celix_arrayList_get(tracker->trackedServices, 0);
        result = tracked->reference;
    }
    celixThreadMutex_unlock(&tracker->mutex);

	return result;
}

array_list_pt serviceTracker_getServiceReferences(service_tracker_t* tracker) {
    //TODO deprecated warning -> not locked
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadMutex_lock(&tracker->mutex);
    for (int i = 0; i < celix_arrayList_size(tracker->trackedServices); i++) {
        celix_tracked_entry_t *tracked = celix_arrayList_get(tracker->trackedServices, i);
        arrayList_add(references, tracked->reference);
    }
    celixThreadMutex_unlock(&tracker->mutex);

	return references;
}

void *serviceTracker_getService(service_tracker_t* tracker) {
    //TODO deprecated warning -> not locked
    void *service = NULL;

    celixThreadMutex_lock(&tracker->mutex);
    if(celix_arrayList_size(tracker->trackedServices) > 0) {
        celix_tracked_entry_t* tracked = celix_arrayList_get(tracker->trackedServices, 0);
        service = tracked->service;
    }
    celixThreadMutex_unlock(&tracker->mutex);

    return service;
}

array_list_pt serviceTracker_getServices(service_tracker_t* tracker) {
    //TODO deprecated warning -> not locked, also make locked variant
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadMutex_lock(&tracker->mutex);
    for (int i = 0; i < celix_arrayList_size(tracker->trackedServices); i++) {
        celix_tracked_entry_t *tracked = celix_arrayList_get(tracker->trackedServices, i);
        arrayList_add(references, tracked->service);
    }
    celixThreadMutex_unlock(&tracker->mutex);

    return references;
}

void *serviceTracker_getServiceByReference(service_tracker_pt tracker, service_reference_pt reference) {
    //TODO deprecated warning -> not locked
    void *service = NULL;

    celixThreadMutex_lock(&tracker->mutex);
    for (int i = 0; i < celix_arrayList_size(tracker->trackedServices); i++) {
        bool equals = false;
        celix_tracked_entry_t *tracked = celix_arrayList_get(tracker->trackedServices, i);
        serviceReference_equals(reference, tracked->reference, &equals);
        if (equals) {
            service = tracked->service;
            break;
        }
    }
    celixThreadMutex_unlock(&tracker->mutex);

	return service;
}

static void serviceTracker_serviceChanged(void *handle, celix_service_event_t *event) {
    service_tracker_t *tracker = handle;

    celixThreadMutex_lock(&tracker->closeSync.mutex);
    bool closing = tracker->closeSync.closing;
    if (!closing) {
        tracker->closeSync.activeCalls += 1;
    }
    celixThreadMutex_unlock(&tracker->closeSync.mutex);

    switch (event->type) {
        case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
        case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED:
            if(!closing) {
                serviceTracker_track(tracker, event->reference, event);
            }
            break;
        case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
            //after this call the registration can be gone, to prevent that happens before the tracker finishing its cleanup job with the corresponding service,
            //untrack the reference even when the tracker is closing.
            serviceTracker_untrack(tracker, event->reference);
            break;
        default:
            //nop
            break;
    }

    if (!closing) {
        celixThreadMutex_lock(&tracker->closeSync.mutex);
        tracker->closeSync.activeCalls -= 1;
        celixThreadCondition_broadcast(&tracker->closeSync.cond);
        celixThreadMutex_unlock(&tracker->closeSync.mutex);
    }
}

size_t serviceTracker_nrOfTrackedServices(service_tracker_t *tracker) {
    celixThreadMutex_lock(&tracker->mutex);
    size_t result = (size_t) arrayList_size(tracker->trackedServices);
    celixThreadMutex_unlock(&tracker->mutex);
    return result;
}

static celix_status_t serviceTracker_track(service_tracker_t* tracker, service_reference_pt reference, celix_service_event_t *event) {
	celix_status_t status = CELIX_SUCCESS;

    celix_tracked_entry_t *found = NULL;

    bundleContext_retainServiceReference(tracker->context, reference);

    celixThreadMutex_lock(&tracker->mutex);
    for (int i = 0; i < celix_arrayList_size(tracker->trackedServices); i++) {
        bool equals = false;
        celix_tracked_entry_t *visit = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
        serviceReference_equals(reference, visit->reference, &equals);
        if (equals) {
            //NOTE it is possible to get two REGISTERED events, second one can be ignored.
            found = visit;
            break;
        }
    }
    celixThreadMutex_unlock(&tracker->mutex);

    if (found == NULL) {
        //NEW entry
        void *service = NULL;
        status = serviceTracker_invokeAddingService(tracker, reference, &service);
        if (status == CELIX_SUCCESS && service != NULL) {
            assert(reference != NULL);

            service_registration_t *reg = NULL;
            properties_t *props = NULL;
            bundle_t *bnd = NULL;

            serviceReference_getBundle(reference, &bnd);
            serviceReference_getServiceRegistration(reference, &reg);
            if (reg != NULL) {
                serviceRegistration_getProperties(reg, &props);
            }

            celix_tracked_entry_t *tracked = tracked_create(reference, service, props, bnd); //use count 1

            celixThreadMutex_lock(&tracker->mutex);
            arrayList_add(tracker->trackedServices, tracked);
            celixThreadCondition_broadcast(&tracker->condTracked);
            celixThreadMutex_unlock(&tracker->mutex);

            if (tracker->set != NULL || tracker->setWithProperties != NULL || tracker->setWithOwner != NULL) {
                celix_serviceTracker_useHighestRankingService(tracker, NULL, 0, tracker, NULL, NULL,
                                                              serviceTracker_checkAndInvokeSetService);
            }
            serviceTracker_invokeAddService(tracker, tracked);
        } else {
            bundleContext_ungetServiceReference(tracker->context, reference);
        }
    } else {
        bundleContext_ungetServiceReference(tracker->context, reference);
    }

    framework_logIfError(tracker->context->framework->logger, status, NULL, "Cannot track reference");

    return status;
}

static void serviceTracker_checkAndInvokeSetService(void *handle, void *highestSvc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    service_tracker_t *tracker = handle;
    bool update = false;
    long svcId = -1;
    if (highestSvc == NULL) {
        //no services available anymore -> unset == call with NULL
        update = true;
    } else {
        svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);
    }
    if (svcId >= 0) {
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

static celix_status_t serviceTracker_invokeAddService(service_tracker_t *tracker, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;

    void *customizerHandle = NULL;
    added_callback_pt function = NULL;

    serviceTrackerCustomizer_getHandle(&tracker->customizer, &customizerHandle);
    serviceTrackerCustomizer_getAddedFunction(&tracker->customizer, &function);
    if (function != NULL) {
        function(customizerHandle, tracked->reference, tracked->service);
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

static celix_status_t serviceTracker_invokeAddingService(service_tracker_t *tracker, service_reference_pt ref, void **svcOut) {
	celix_status_t status = CELIX_SUCCESS;

    void *handle = NULL;
    adding_callback_pt function = NULL;

    status = serviceTrackerCustomizer_getHandle(&tracker->customizer, &handle);

    if (status == CELIX_SUCCESS) {
        status = serviceTrackerCustomizer_getAddingFunction(&tracker->customizer, &function);
    }

    if (status == CELIX_SUCCESS && function != NULL) {
        status = function(handle, ref, svcOut);
    } else if (status == CELIX_SUCCESS) {
        status = bundleContext_getService(tracker->context, ref, svcOut);
    }

    framework_logIfError(tracker->context->framework->logger, status, NULL, "Cannot handle addingService");

    return status;
}

static celix_status_t serviceTracker_untrack(service_tracker_t* tracker, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    celix_tracked_entry_t *remove = NULL;

    celixThreadMutex_lock(&tracker->mutex);
    for (int i = 0; i < celix_arrayList_size(tracker->trackedServices); i++) {
        bool equals;
        celix_tracked_entry_t *tracked = (celix_tracked_entry_t*) arrayList_get(tracker->trackedServices, i);
        serviceReference_equals(reference, tracked->reference, &equals);
        if (equals) {
            remove = tracked;
            //remove from trackedServices to prevent getting this service, but don't destroy yet, can be in use
            celix_arrayList_removeAt(tracker->trackedServices, i);
            celix_arrayList_add(tracker->untrackingServices, remove);
            break;
        }
    }
    int size = celix_arrayList_size(tracker->trackedServices); //updated size
    celixThreadMutex_unlock(&tracker->mutex);

    //note also syncing on untracking entries, to ensure no untrack is parallel in progress
    if (remove != NULL) {
        serviceTracker_untrackTracked(tracker, remove, size, true);
        celixThreadMutex_lock(&tracker->mutex);
        celix_arrayList_remove(tracker->untrackingServices, remove);
        celixThreadMutex_unlock(&tracker->mutex);
    } else {
        //ensure no untrack is still happening (to ensure it safe to unregister service)
        celixThreadMutex_lock(&tracker->mutex);
        while (celix_arrayList_size(tracker->untrackingServices) > 0) {
            celixThreadCondition_wait(&tracker->condUntracking, &tracker->mutex);
        }
        celixThreadMutex_unlock(&tracker->mutex);
    }

    framework_logIfError(tracker->context->framework->logger, status, NULL, "Cannot untrack reference");

    return status;
}

static void serviceTracker_untrackTracked(service_tracker_t *tracker, celix_tracked_entry_t *tracked, int trackedSize, bool set) {
    assert(tracker != NULL);
    serviceTracker_invokeRemovingService(tracker, tracked);

    if(set) {
        if (trackedSize == 0) {
            serviceTracker_checkAndInvokeSetService(tracker, NULL, NULL, NULL);
        } else {
            celix_serviceTracker_useHighestRankingService(tracker, NULL, 0, tracker, NULL, NULL,
                                                          serviceTracker_checkAndInvokeSetService);
        }
    }

    bundleContext_ungetServiceReference(tracker->context, tracked->reference);
    tracked_release(tracked);

    //Wait till the useCount is 0, because the untrack should only return if the service is not used anymore.
    tracked_waitAndDestroy(tracked);
}

static celix_status_t serviceTracker_invokeRemovingService(service_tracker_t *tracker, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;
    bool ungetSuccess = true;

    void *customizerHandle = NULL;
    removed_callback_pt function = NULL;

    serviceTrackerCustomizer_getHandle(&tracker->customizer, &customizerHandle);
    serviceTrackerCustomizer_getRemovedFunction(&tracker->customizer, &function);

    if (function != NULL) {
        status = function(customizerHandle, tracked->reference, tracked->service);
    }

    void *handle = tracker->callbackHandle;
    if (tracker->remove != NULL) {
        tracker->remove(handle, tracked->service);
    }
    if (tracker->removeWithProperties != NULL) {
        tracker->removeWithProperties(handle, tracked->service, tracked->properties);
    }
    if (tracker->removeWithOwner != NULL) {
        tracker->removeWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
    }

    if (status == CELIX_SUCCESS) {
        status = bundleContext_ungetService(tracker->context, tracked->reference, &ungetSuccess);
    }

    if (!ungetSuccess) {
        celix_framework_log(tracker->context->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error ungetting service");
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
    const char* serviceName = NULL;
    char *filter = NULL;
    assert(ctx != NULL);
    assert(opts != NULL);
    serviceName = opts->filter.serviceName == NULL ? "*" : opts->filter.serviceName;
    filter = celix_serviceRegistry_createFilterFor(ctx->framework->registry, opts->filter.serviceName, opts->filter.versionRange, opts->filter.filter);
    if(filter == NULL) {
        celix_framework_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__,
                            "Error cannot create filter.");
        return NULL;
    }
    tracker = calloc(1, sizeof(*tracker));
    if(tracker == NULL) {
        free(filter);
        celix_framework_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__,
                            "No memory for tracker.");
       return NULL;
    }

    tracker->context = ctx;
    tracker->serviceName = celix_utils_strdup(serviceName);
    tracker->filter = filter;
    tracker->state = CELIX_SERVICE_TRACKER_CLOSED;

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

    celixThreadMutex_create(&tracker->closeSync.mutex, NULL);
    celixThreadCondition_init(&tracker->closeSync.cond, NULL);

    celixThreadMutex_create(&tracker->mutex, NULL);
    celixThreadCondition_init(&tracker->condTracked, NULL);
    celixThreadCondition_init(&tracker->condUntracking, NULL);
    tracker->trackedServices = celix_arrayList_create();
    tracker->untrackingServices = celix_arrayList_create();
    tracker->currentHighestServiceId = -1;

    tracker->listener.handle = tracker;
    tracker->listener.serviceChanged = (void *) serviceTracker_serviceChanged;

    serviceTracker_open(tracker);
    return tracker;
}

void celix_serviceTracker_destroy(celix_service_tracker_t *tracker) {
    if (tracker != NULL) {
        serviceTracker_close(tracker);
        serviceTracker_destroy(tracker);
    }
}

bool celix_serviceTracker_useHighestRankingService(service_tracker_t *tracker,
                                                   const char *serviceName /*sanity*/,
                                                   double waitTimeoutInSeconds /*0 -> do not wait */,
                                                   void *callbackHandle,
                                                   void (*use)(void *handle, void *svc),
                                                   void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
                                                   void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)) {
    bool called = false;
    celix_tracked_entry_t *tracked = NULL;
    celix_tracked_entry_t *highest = NULL;
    unsigned int i;
    struct timespec begin = celix_gettime(CLOCK_MONOTONIC);
    double remaining = waitTimeoutInSeconds > INT_MAX ? INT_MAX : waitTimeoutInSeconds;
    remaining = remaining < 0 ? 0 : remaining;
    double elapsed = 0;
    long seconds = remaining;
    long nanoseconds = (remaining - seconds) * CELIX_NS_IN_SEC;

    //first lock tracker and get highest tracked entry
    celixThreadMutex_lock(&tracker->mutex);
    while (highest == NULL) {
        unsigned int size = arrayList_size(tracker->trackedServices);

        for (i = 0; i < size; i++) {
            tracked = (celix_tracked_entry_t *) arrayList_get(tracker->trackedServices, i);
            if (serviceName == NULL || (serviceName != NULL && tracked->serviceName != NULL &&
                                        celix_utils_stringEquals(tracked->serviceName, serviceName))) {
                if (highest == NULL) {
                    highest = tracked;
                } else {
                    int compare = celix_utils_compareServiceIdsAndRanking(
                            tracked->serviceId, tracked->serviceRanking,
                            highest->serviceId, highest->serviceRanking
                    );
                    if (compare < 0) {
                        highest = tracked;
                    }
                }
            }
        }
        if(highest == NULL && (seconds > 0 || nanoseconds > 0)) {
            celixThreadCondition_timedwaitRelative(&tracker->condTracked, &tracker->mutex, seconds, nanoseconds);
            elapsed  = celix_elapsedtime(CLOCK_MONOTONIC, begin);
            remaining = remaining > elapsed ? (remaining - elapsed) : 0;
            seconds = remaining;
            nanoseconds = (remaining - seconds) * CELIX_NS_IN_SEC;
        } else {
            // highest found or timeout
            break;
        }
    }
    if (highest != NULL) {
        //highest found lock tracked entry and increase use count
        tracked_retain(highest);
    }
    //unlock tracker so that the tracked entry can be removed from the trackedServices list if unregistered.
    celixThreadMutex_unlock(&tracker->mutex);

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
        tracked_release(highest);
    }

    return called;
}

size_t celix_serviceTracker_useServices(
        service_tracker_t *tracker,
        const char* serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc),
        void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
        void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)) {
    size_t count = 0;
    //first lock tracker, get tracked entries and increase use count
    celixThreadMutex_lock(&tracker->mutex);
    int size = celix_arrayList_size(tracker->trackedServices);
    count = (size_t)size;
    celix_tracked_entry_t *entries[size];
    for (int i = 0; i < size; i++) {
        celix_tracked_entry_t *tracked = (celix_tracked_entry_t *) arrayList_get(tracker->trackedServices, i);
        tracked_retain(tracked);
        entries[i] = tracked;
    }
    //unlock tracker so that the tracked entry can be removed from the trackedServices list if unregistered.
    celixThreadMutex_unlock(&tracker->mutex);

    //then use entries and decrease use count
    for (int i = 0; i < size; i++) {
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

        tracked_release(entry);
    }
    return count;
}