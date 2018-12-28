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
#include "bundle_context_private.h"
#include "celix_array_list.h"

static celix_status_t serviceTracker_track(celix_service_tracker_instance_t *tracker, service_reference_pt reference, celix_service_event_t *event);
static celix_status_t serviceTracker_untrack(celix_service_tracker_instance_t *tracker, service_reference_pt reference, celix_service_event_t *event);
static void serviceTracker_untrackTracked(celix_service_tracker_instance_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeAddingService(celix_service_tracker_instance_t *tracker, service_reference_pt ref, void **svcOut);
static celix_status_t serviceTracker_invokeAddService(celix_service_tracker_instance_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeModifiedService(celix_service_tracker_instance_t *tracker, celix_tracked_entry_t *tracked);
static celix_status_t serviceTracker_invokeRemovingService(celix_service_tracker_instance_t *tracker, celix_tracked_entry_t *tracked);
static void serviceTracker_checkAndInvokeSetService(void *handle, void *highestSvc, const properties_t *props, const bundle_t *bnd);
static bool serviceTracker_useHighestRankingServiceInternal(celix_service_tracker_instance_t *instance,
                                                            const char *serviceName /*sanity*/,
                                                            void *callbackHandle,
                                                            void (*use)(void *handle, void *svc),
                                                            void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
                                                            void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner));

static void serviceTracker_addInstanceFromShutdownList(celix_service_tracker_instance_t *instance);
static void serviceTracker_remInstanceFromShutdownList(celix_service_tracker_instance_t *instance);

static celix_thread_once_t g_once = CELIX_THREAD_ONCE_INIT;
static celix_thread_mutex_t g_mutex;
static celix_thread_cond_t g_cond;
static celix_array_list_t *g_shutdownInstances = NULL; //value = celix_service_tracker_instance -> used for syncing with shutdown threads

static void serviceTracker_once(void) {
    celixThreadMutex_create(&g_mutex, NULL);
    celixThreadCondition_init(&g_cond, NULL);
}

static inline celix_tracked_entry_t* tracked_create(service_reference_pt ref, void *svc, celix_properties_t *props, celix_bundle_t *bnd) {
    celix_tracked_entry_t *tracked = calloc(1, sizeof(*tracked));
    tracked->reference = ref;
    tracked->service = svc;
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
    if (tracked->useCount == 0) {
        celixThreadCondition_broadcast(&tracked->useCond);
    }
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
        (*tracker)->customizer = customizer;
	}

	framework_logIfError(logger, status, NULL, "Cannot create service tracker [filter=%s]", filter);

	return status;
}

celix_status_t serviceTracker_destroy(service_tracker_pt tracker) {
    if (tracker->customizer != NULL) {
	    serviceTrackerCustomizer_destroy(tracker->customizer);
	}

	free(tracker->filter);
	free(tracker);

	return CELIX_SUCCESS;
}

celix_status_t serviceTracker_open(service_tracker_pt tracker) {
    celix_service_listener_t *listener = NULL;
    celix_service_tracker_instance_t *instance = NULL;
    array_list_pt initial = NULL;
    celix_status_t status = CELIX_SUCCESS;

    celixThreadRwlock_writeLock(&tracker->instanceLock);
    if (tracker->instance == NULL) {
        instance = calloc(1, sizeof(*instance));
        instance->context = tracker->context;

        instance->closing = false;
        instance->activeServiceChangeCalls = 0;
        celixThreadMutex_create(&instance->closingLock, NULL);
        celixThreadCondition_init(&instance->activeServiceChangeCallsCond, NULL);


        celixThreadRwlock_create(&instance->lock, NULL);
        instance->trackedServices = celix_arrayList_create();

        celixThreadMutex_create(&instance->mutex, NULL);
        instance->currentHighestServiceId = -1;

        instance->listener.handle = instance;
        instance->listener.serviceChanged = (void *) serviceTracker_serviceChanged;
        listener = &instance->listener;

        instance->callbackHandle = tracker->callbackHandle;
        instance->filter = strdup(tracker->filter);
        if (tracker->customizer != NULL) {
            memcpy(&instance->customizer, tracker->customizer, sizeof(instance->customizer));
        }
        instance->add = tracker->add;
        instance->addWithProperties = tracker->addWithProperties;
        instance->addWithOwner = tracker->addWithOwner;
        instance->set = tracker->set;
        instance->setWithProperties = tracker->setWithProperties;
        instance->setWithOwner = tracker->setWithOwner;
        instance->remove = tracker->remove;
        instance->removeWithProperties = tracker->removeWithProperties;
        instance->removeWithOwner = tracker->removeWithOwner;

        status = bundleContext_getServiceReferences(tracker->context, NULL, tracker->filter, &initial); //REF COUNT to 1

        tracker->instance = instance;
    } else {
        //already open
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);

    //TODO add fw call which adds a service listener and return the then valid service references.
    if (status == CELIX_SUCCESS && listener != NULL) { //register service listener
        status = bundleContext_addServiceListener(tracker->context, listener, tracker->filter);
    }
    if (status == CELIX_SUCCESS && initial != NULL) {
        service_reference_pt initial_reference;
        unsigned int i;
        for (i = 0; i < arrayList_size(initial); i++) {
            initial_reference = (service_reference_pt) arrayList_get(initial, i);
            serviceTracker_track(instance, initial_reference, NULL); //REF COUNT to 2
            bundleContext_ungetServiceReference(tracker->context, initial_reference); //REF COUNT to 1
        }
        arrayList_destroy(initial);
    }

	if (status != CELIX_SUCCESS && listener != NULL){
		free(listener);
	}

	framework_logIfError(logger, status, NULL, "Cannot open tracker");

	return status;
}

static void* shutdownServiceTrackerInstanceHandler(void *data) {
    celix_service_tracker_instance_t *instance = data;

    fw_removeServiceListener(instance->context->framework, instance->context->bundle, &instance->listener);

    celixThreadMutex_destroy(&instance->closingLock);
    celixThreadCondition_destroy(&instance->activeServiceChangeCallsCond);
    celixThreadMutex_destroy(&instance->mutex);
    celixThreadRwlock_destroy(&instance->lock);
    celix_arrayList_destroy(instance->trackedServices);
    free(instance->filter);

    serviceTracker_remInstanceFromShutdownList(instance);
    free(instance);

    return NULL;
}

celix_status_t serviceTracker_close(service_tracker_pt tracker) {
	//put all tracked entries in tmp array list, so that the untrack (etc) calls are not blocked.
    //set state to close to prevent service listener events

    celixThreadRwlock_writeLock(&tracker->instanceLock);
    celix_service_tracker_instance_t *instance = tracker->instance;
    tracker->instance = NULL;
    celixThreadRwlock_unlock(&tracker->instanceLock);

    if (instance != NULL) {

        //prevent service listener events
        celixThreadMutex_lock(&instance->closingLock);
        instance->closing = true;
        celixThreadMutex_unlock(&instance->closingLock);

        int i;
        celixThreadRwlock_writeLock(&instance->lock);
        size_t size = celix_arrayList_size(instance->trackedServices);
        celix_tracked_entry_t *trackedEntries[size];
        for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
            trackedEntries[i] = (celix_tracked_entry_t *) arrayList_get(instance->trackedServices, i);
        }
        arrayList_clear(instance->trackedServices);
        celixThreadRwlock_unlock(&instance->lock);

        //loop trough tracked entries an untrack
        for (i = 0; i < size; i++) {
            serviceTracker_untrackTracked(instance, trackedEntries[i]);
        }

        //sync til all pending serviceChanged event are handled.. (TODO again a possible deadlock??)
        celixThreadMutex_lock(&instance->closingLock);
        while(instance->activeServiceChangeCalls > 0) {
            celixThreadCondition_wait(&instance->activeServiceChangeCallsCond, &instance->closingLock);
        }
        celixThreadMutex_unlock(&instance->closingLock);



        //NOTE Separate thread is needed to prevent deadlock where closing is triggered from a serviceChange event and the
        // untrack -> removeServiceListener will try to remove a service listener which is being invoked and is the
        // actual thread calling the removeServiceListener.
        //
        // This can be detached -> because service listener events are ignored (closing=true) and so no callbacks
        //are made back to the celix framework / tracker owner.
        serviceTracker_addInstanceFromShutdownList(instance);
        celix_thread_t localThread;
        celixThread_create(&localThread, NULL, shutdownServiceTrackerInstanceHandler, instance);
        celixThread_detach(localThread);
    }

	return CELIX_SUCCESS;
}

service_reference_pt serviceTracker_getServiceReference(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked
	celix_tracked_entry_t *tracked;
    service_reference_pt result = NULL;
	unsigned int i;

	celixThreadRwlock_readLock(&tracker->instanceLock);
	celix_service_tracker_instance_t *instance = tracker->instance;
	if (instance != NULL) {
        celixThreadRwlock_readLock(&instance->lock);
        for (i = 0; i < arrayList_size(instance->trackedServices); ++i) {
            tracked = (celix_tracked_entry_t *) arrayList_get(instance->trackedServices, i);
            result = tracked->reference;
            break;
        }
        celixThreadRwlock_unlock(&instance->lock);
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);

	return result;
}

array_list_pt serviceTracker_getServiceReferences(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked
    celix_tracked_entry_t *tracked;
	unsigned int i;
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadRwlock_readLock(&tracker->instanceLock);
	celix_service_tracker_instance_t *instance = tracker->instance;
	if (instance != NULL) {
        celixThreadRwlock_readLock(&instance->lock);
        for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
            tracked = (celix_tracked_entry_t*) arrayList_get(instance->trackedServices, i);
            arrayList_add(references, tracked->reference);
        }
        celixThreadRwlock_unlock(&instance->lock);
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);

	return references;
}

void *serviceTracker_getService(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked
    celix_tracked_entry_t* tracked;
    void *service = NULL;
	unsigned int i;

	celixThreadRwlock_readLock(&tracker->instanceLock);
	celix_service_tracker_instance_t *instance = tracker->instance;
	if (instance != NULL) {
        celixThreadRwlock_readLock(&instance->lock);
        for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
            tracked = (celix_tracked_entry_t*) arrayList_get(instance->trackedServices, i);
            service = tracked->service;
            break;
        }
        celixThreadRwlock_unlock(&instance->lock);
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);

    return service;
}

array_list_pt serviceTracker_getServices(service_tracker_pt tracker) {
    //TODO deprecated warning -> not locked, also make locked variant
    celix_tracked_entry_t *tracked;
	unsigned int i;
	array_list_pt references = NULL;
	arrayList_create(&references);

    celixThreadRwlock_readLock(&tracker->instanceLock);
    celix_service_tracker_instance_t *instance = tracker->instance;
    if (instance != NULL) {
        celixThreadRwlock_readLock(&instance->lock);
        for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
            tracked = (celix_tracked_entry_t *) arrayList_get(instance->trackedServices, i);
            arrayList_add(references, tracked->service);
        }
        celixThreadRwlock_unlock(&instance->lock);
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);

    return references;
}

void *serviceTracker_getServiceByReference(service_tracker_pt tracker, service_reference_pt reference) {
    //TODO deprecated warning -> not locked
    celix_tracked_entry_t *tracked;
    void *service = NULL;
	unsigned int i;

    celixThreadRwlock_readLock(&tracker->instanceLock);
    celix_service_tracker_instance_t *instance = tracker->instance;
    if (instance != NULL) {
        celixThreadRwlock_readLock(&instance->lock);
        for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
            bool equals = false;
            tracked = (celix_tracked_entry_t *) arrayList_get(instance->trackedServices, i);
            serviceReference_equals(reference, tracked->reference, &equals);
            if (equals) {
                service = tracked->service;
                break;
            }
        }
        celixThreadRwlock_unlock(&instance->lock);
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);

	return service;
}

void serviceTracker_serviceChanged(celix_service_listener_t *listener, celix_service_event_t *event) {
	celix_service_tracker_instance_t *instance = listener->handle;

    celixThreadMutex_lock(&instance->closingLock);
    bool closing = instance->closing;
    if (!closing) {
        instance->activeServiceChangeCalls += 1;
    }
    celixThreadMutex_unlock(&instance->closingLock);

    if (!closing) {
        switch (event->type) {
            case OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED:
                serviceTracker_track(instance, event->reference, event);
                break;
            case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED:
                serviceTracker_track(instance, event->reference, event);
                break;
            case OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING:
                serviceTracker_untrack(instance, event->reference, event);
                break;
            case OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED_ENDMATCH:
                //TODO
                break;
        }
        celixThreadMutex_lock(&instance->closingLock);
        assert(instance->activeServiceChangeCalls > 0);
        instance->activeServiceChangeCalls -= 1;
        if (instance->activeServiceChangeCalls == 0) {
            celixThreadCondition_broadcast(&instance->activeServiceChangeCallsCond);
        }
        celixThreadMutex_unlock(&instance->closingLock);
    }
}

static celix_status_t serviceTracker_track(celix_service_tracker_instance_t *instance, service_reference_pt reference, celix_service_event_t *event) {
	celix_status_t status = CELIX_SUCCESS;

    celix_tracked_entry_t *found = NULL;
    unsigned int i;
    
    bundleContext_retainServiceReference(instance->context, reference);

    celixThreadRwlock_readLock(&instance->lock);
    for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
        bool equals = false;
        celix_tracked_entry_t *visit = (celix_tracked_entry_t*) arrayList_get(instance->trackedServices, i);
        serviceReference_equals(reference, visit->reference, &equals);
        if (equals) {
            found = visit;
            tracked_retain(found);
            break;
        }
    }
    celixThreadRwlock_unlock(&instance->lock);

    if (found != NULL) {
        status = serviceTracker_invokeModifiedService(instance, found);
        tracked_retain(found);
    } else if (status == CELIX_SUCCESS && found == NULL) {
        //NEW entry
        void *service = NULL;
        status = serviceTracker_invokeAddingService(instance, reference, &service);
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

            celix_tracked_entry_t *tracked = tracked_create(reference, service, props, bnd);

            celixThreadRwlock_writeLock(&instance->lock);
            arrayList_add(instance->trackedServices, tracked);
            celixThreadRwlock_unlock(&instance->lock);

            serviceTracker_invokeAddService(instance, tracked);
            serviceTracker_useHighestRankingServiceInternal(instance, tracked->serviceName, instance, NULL, NULL, serviceTracker_checkAndInvokeSetService);
        }
    }

    framework_logIfError(logger, status, NULL, "Cannot track reference");

    return status;
}

static void serviceTracker_checkAndInvokeSetService(void *handle, void *highestSvc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    celix_service_tracker_instance_t *instance = handle;
    bool update = false;
    long svcId = -1;
    if (highestSvc == NULL) {
        //no services available anymore -> unset == call with NULL
        update = true;
    } else {
        svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1);
    }
    if (svcId > 0) {
        celixThreadMutex_lock(&instance->mutex);
        if (instance->currentHighestServiceId != svcId) {
            instance->currentHighestServiceId = svcId;
            update = true;
            //update
        }
        celixThreadMutex_unlock(&instance->mutex);
    }
    if (update) {
        void *h = instance->callbackHandle;
        if (instance->set != NULL) {
            instance->set(h, highestSvc);
        }
        if (instance->setWithProperties != NULL) {
            instance->setWithProperties(h, highestSvc, props);
        }
        if (instance->setWithOwner != NULL) {
            instance->setWithOwner(h, highestSvc, props, bnd);
        }
    }
}

static celix_status_t serviceTracker_invokeModifiedService(celix_service_tracker_instance_t *instance, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;

    void *customizerHandle = NULL;
    modified_callback_pt function = NULL;
    serviceTrackerCustomizer_getHandle(&instance->customizer, &customizerHandle);
    serviceTrackerCustomizer_getModifiedFunction(&instance->customizer, &function);
    if (function != NULL) {
        function(customizerHandle, tracked->reference, tracked->service);
    }
    
    void *handle = instance->callbackHandle;
    if (instance->modified != NULL) {
        instance->modified(handle, tracked->service);
    }
    if (instance->modifiedWithProperties != NULL) {
        instance->modifiedWithProperties(handle, tracked->service, tracked->properties);
    }
    if (instance->modifiedWithOwner != NULL) {
        instance->modifiedWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
    }
    return status;
}


static celix_status_t serviceTracker_invokeAddService(celix_service_tracker_instance_t *instance, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;

    void *customizerHandle = NULL;
    added_callback_pt function = NULL;

    serviceTrackerCustomizer_getHandle(&instance->customizer, &customizerHandle);
    serviceTrackerCustomizer_getAddedFunction(&instance->customizer, &function);
    if (function != NULL) {
        function(customizerHandle, tracked->reference, tracked->service);
    }

    void *handle = instance->callbackHandle;
    if (instance->add != NULL) {
        instance->add(handle, tracked->service);
    }
    if (instance->addWithProperties != NULL) {
        instance->addWithProperties(handle, tracked->service, tracked->properties);
    }
    if (instance->addWithOwner != NULL) {
        instance->addWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
    }
    return status;
}

static celix_status_t serviceTracker_invokeAddingService(celix_service_tracker_instance_t *instance, service_reference_pt ref, void **svcOut) {
	celix_status_t status = CELIX_SUCCESS;

    void *handle = NULL;
    adding_callback_pt function = NULL;

    status = serviceTrackerCustomizer_getHandle(&instance->customizer, &handle);

    if (status == CELIX_SUCCESS) {
        status = serviceTrackerCustomizer_getAddingFunction(&instance->customizer, &function);
    }

    if (status == CELIX_SUCCESS && function != NULL) {
        status = function(handle, ref, svcOut);
    } else if (status == CELIX_SUCCESS) {
        status = bundleContext_getService(instance->context, ref, svcOut);
    }

    framework_logIfError(logger, status, NULL, "Cannot handle addingService");

    return status;
}

static celix_status_t serviceTracker_untrack(celix_service_tracker_instance_t* instance, service_reference_pt reference, celix_service_event_t *event) {
    celix_status_t status = CELIX_SUCCESS;
    celix_tracked_entry_t *remove = NULL;
    unsigned int i;
    unsigned int size;
    const char *serviceName = NULL;

    celixThreadRwlock_writeLock(&instance->lock);
    size = arrayList_size(instance->trackedServices);
    for (i = 0; i < size; i++) {
        bool equals;
        celix_tracked_entry_t *tracked = (celix_tracked_entry_t*) arrayList_get(instance->trackedServices, i);
        serviceName = tracked->serviceName;
        serviceReference_equals(reference, tracked->reference, &equals);
        if (equals) {
            remove = tracked;
            //remove from trackedServices to prevent getting this service, but don't destroy yet, can be in use
            arrayList_remove(instance->trackedServices, i);
            break;
        }
    }
    size = arrayList_size(instance->trackedServices); //updated size
    celixThreadRwlock_unlock(&instance->lock);

    if (size == 0) {
        serviceTracker_checkAndInvokeSetService(instance, NULL, NULL, NULL);
    } else {
        serviceTracker_useHighestRankingServiceInternal(instance, serviceName, instance, NULL, NULL, serviceTracker_checkAndInvokeSetService);
    }

    serviceTracker_untrackTracked(instance, remove);

    framework_logIfError(logger, status, NULL, "Cannot untrack reference");

    return status;
}

static void serviceTracker_untrackTracked(celix_service_tracker_instance_t *instance, celix_tracked_entry_t *tracked) {
    if (tracked != NULL) {
        serviceTracker_invokeRemovingService(instance, tracked);
        bundleContext_ungetServiceReference(instance->context, tracked->reference);
        tracked_release(tracked);

        //Wait till the useCount is 0, because the untrack should only return if the service is not used anymore.
        tracked_waitAndDestroy(tracked);
    }
}

static celix_status_t serviceTracker_invokeRemovingService(celix_service_tracker_instance_t *instance, celix_tracked_entry_t *tracked) {
    celix_status_t status = CELIX_SUCCESS;
    bool ungetSuccess = true;

    void *customizerHandle = NULL;
    removed_callback_pt function = NULL;

    serviceTrackerCustomizer_getHandle(&instance->customizer, &customizerHandle);
    serviceTrackerCustomizer_getRemovedFunction(&instance->customizer, &function);

    if (function != NULL) {
        status = function(customizerHandle, tracked->reference, tracked->service);
    }

    void *handle = instance->callbackHandle;
    if (instance->remove != NULL) {
        instance->remove(handle, tracked->service);
    }
    if (instance->addWithProperties != NULL) {
        instance->removeWithProperties(handle, tracked->service, tracked->properties);
    }
    if (instance->removeWithOwner != NULL) {
        instance->removeWithOwner(handle, tracked->service, tracked->properties, tracked->serviceOwner);
    }

    if (status == CELIX_SUCCESS) {
        status = bundleContext_ungetService(instance->context, tracked->reference, &ungetSuccess);
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

            celixThreadRwlock_create(&tracker->instanceLock, NULL);

            //setting lang
            const char *lang = opts->filter.serviceLanguage;
            if (lang == NULL || strncmp("", lang, 1) == 0) {
                lang = CELIX_FRAMEWORK_SERVICE_C_LANGUAGE;
            }

            //setting filter
            if (opts->filter.ignoreServiceLanguage) {
                if (opts->filter.filter != NULL && opts->filter.versionRange != NULL) {
                    //TODO version range
                    asprintf(&tracker->filter, "&((%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName, opts->filter.filter);
                } else if (opts->filter.versionRange != NULL) {
                    //TODO version range
                    asprintf(&tracker->filter, "&((%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName);
                } else if (opts->filter.filter != NULL) {
                    asprintf(&tracker->filter, "(&(%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName, opts->filter.filter);
                } else {
                    asprintf(&tracker->filter, "(&(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, opts->filter.serviceName);
                }
            } else {
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

static bool serviceTracker_useHighestRankingServiceInternal(celix_service_tracker_instance_t *instance,
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
    celixThreadRwlock_readLock(&instance->lock);
    for (i = 0; i < arrayList_size(instance->trackedServices); i++) {
        tracked = (celix_tracked_entry_t *) arrayList_get(instance->trackedServices, i);
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
        tracked_retain(highest);
    }
    //unlock tracker so that the tracked entry can be removed from the trackedServices list if unregistered.
    celixThreadRwlock_unlock(&instance->lock);

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


bool celix_serviceTracker_useHighestRankingService(
        celix_service_tracker_t *tracker,
        const char *serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc),
        void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
        void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)) {
    celixThreadRwlock_readLock(&tracker->instanceLock);
    celix_service_tracker_instance_t *instance = tracker->instance;
    bool called = false;
    if (instance != NULL) {
        called = serviceTracker_useHighestRankingServiceInternal(instance, serviceName, callbackHandle, use,
                                                                 useWithProperties, useWithOwner);
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);
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

    celixThreadRwlock_readLock(&tracker->instanceLock);
    celix_service_tracker_instance_t *instance = tracker->instance;
    if (instance != NULL) {
        //first lock tracker, get tracked entries and increase use count
        celixThreadRwlock_readLock(&instance->lock);
        size_t size = celix_arrayList_size(instance->trackedServices);
        celix_tracked_entry_t *entries[size];
        for (i = 0; i < size; i++) {
            celix_tracked_entry_t *tracked = (celix_tracked_entry_t *) arrayList_get(instance->trackedServices, i);
            tracked_retain(tracked);
            entries[i] = tracked;
        }
        //unlock tracker so that the tracked entry can be removed from the trackedServices list if unregistered.
        celixThreadRwlock_unlock(&instance->lock);

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

            tracked_release(entry);
        }
    }
    celixThreadRwlock_unlock(&tracker->instanceLock);
}

void celix_serviceTracker_syncForFramework(void *fw) {
    celixThread_once(&g_once, serviceTracker_once);
    celixThreadMutex_lock(&g_mutex);
    size_t count = 0;
    do {
        count = 0;
        if (g_shutdownInstances != NULL) {
            for (int i = 0; i < celix_arrayList_size(g_shutdownInstances); ++i) {
                celix_service_tracker_instance_t *instance = celix_arrayList_get(g_shutdownInstances, i);
                if (instance->context->framework == fw) {
                    count += 1;
                }
            }
        }
        if (count > 0) {
            pthread_cond_wait(&g_cond, &g_mutex);
        }
    } while (count > 0);

    if (g_shutdownInstances != NULL && celix_arrayList_size(g_shutdownInstances) == 0) {
        celix_arrayList_destroy(g_shutdownInstances);
        g_shutdownInstances = NULL;
    }
    celixThreadMutex_unlock(&g_mutex);
}

void celix_serviceTracker_syncForContext(void *ctx) {
    celixThread_once(&g_once, serviceTracker_once);
    celixThreadMutex_lock(&g_mutex);
    size_t count;
    do {
        count = 0;
        if (g_shutdownInstances != NULL) {
            for (int i = 0; i < celix_arrayList_size(g_shutdownInstances); ++i) {
                celix_service_tracker_instance_t *instance = celix_arrayList_get(g_shutdownInstances, i);
                if (instance->context == ctx) {
                    count += 1;
                }
            }
        }
        if (count > 0) {
            pthread_cond_wait(&g_cond, &g_mutex);
        }
    } while (count > 0);

    if (g_shutdownInstances != NULL && celix_arrayList_size(g_shutdownInstances) == 0) {
        celix_arrayList_destroy(g_shutdownInstances);
        g_shutdownInstances = NULL;
    }
    celixThreadMutex_unlock(&g_mutex);
}

static void serviceTracker_addInstanceFromShutdownList(celix_service_tracker_instance_t *instance) {
    celixThread_once(&g_once, serviceTracker_once);
    celixThreadMutex_lock(&g_mutex);
    if (g_shutdownInstances == NULL) {
        g_shutdownInstances = celix_arrayList_create();
    }
    celix_arrayList_add(g_shutdownInstances, instance);
    celixThreadMutex_unlock(&g_mutex);
}

static void serviceTracker_remInstanceFromShutdownList(celix_service_tracker_instance_t *instance) {
    celixThread_once(&g_once, serviceTracker_once);
    celixThreadMutex_lock(&g_mutex);
    if (g_shutdownInstances != NULL) {
        size_t size = celix_arrayList_size(g_shutdownInstances);
        for (size_t i = 0; i < size; ++i) {
            celix_array_list_entry_t entry;
            memset(&entry, 0, sizeof(entry));
            entry.voidPtrVal = instance;
            celix_arrayList_removeEntry(g_shutdownInstances, entry);
        }
        if (celix_arrayList_size(g_shutdownInstances) == 0) {
            celix_arrayList_destroy(g_shutdownInstances);
            g_shutdownInstances = NULL;
        }
        celixThreadCondition_broadcast(&g_cond);
    }
    celixThreadMutex_unlock(&g_mutex);
}
