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
#include <stdio.h>
#include <string.h>
#include <utils.h>

#include "constants.h"
#include "bundle_context_private.h"
#include "framework_private.h"
#include "bundle.h"
#include "celix_bundle.h"
#include "celix_log.h"
#include "service_tracker.h"
#include "celix_dependency_manager.h"
#include "dm_dependency_manager_impl.h"
#include "celix_array_list.h"

static celix_status_t bundleContext_bundleChanged(void *handle, bundle_event_t *event);
static void bundleContext_cleanupBundleTrackers(bundle_context_t *ct);
static void bundleContext_cleanupServiceTrackers(bundle_context_t *ctx);
static void bundleContext_cleanupServiceTrackerTrackers(bundle_context_t *ctx);

celix_status_t bundleContext_create(framework_pt framework, framework_logger_pt logger, bundle_pt bundle, bundle_context_pt *bundle_context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;

	if (*bundle_context != NULL && framework == NULL && bundle == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
        context = malloc(sizeof(*context));
        if (!context) {
            status = CELIX_ENOMEM;
        } else {
            context->framework = framework;
            context->bundle = bundle;
            context->mng = NULL;

            celixThreadMutex_create(&context->mutex, NULL);

            arrayList_create(&context->svcRegistrations);
            context->bundleTrackers = hashMap_create(NULL,NULL,NULL,NULL);
            context->serviceTrackers = hashMap_create(NULL,NULL,NULL,NULL);
            context->serviceTrackerTrackers =  hashMap_create(NULL,NULL,NULL,NULL);
            context->nextTrackerId = 1L;

            *bundle_context = context;

        }
	}

	framework_logIfError(logger, status, NULL, "Failed to create context");

	return status;
}

celix_status_t bundleContext_destroy(bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
	    celixThreadMutex_lock(&context->mutex);


	    bundleContext_cleanupBundleTrackers(context);
	    bundleContext_cleanupServiceTrackers(context);
        bundleContext_cleanupServiceTrackerTrackers(context);

        //TODO cleanup service registrations

	    //NOTE still present service registrations will be cleared during bundle stop in the
	    //service registry (serviceRegistry_clearServiceRegistrations).
        celixThreadMutex_unlock(&context->mutex);
	    celixThreadMutex_destroy(&context->mutex); 
	    arrayList_destroy(context->svcRegistrations);

	    if (context->mng != NULL) {
	        celix_dependencyManager_removeAllComponents(context->mng);
            celix_private_dependencyManager_destroy(context->mng);
            context->mng = NULL;
	    }

	    free(context);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to destroy context");

	return status;
}

celix_status_t bundleContext_getBundle(bundle_context_pt context, bundle_pt *out) {
	celix_status_t status = CELIX_SUCCESS;
    celix_bundle_t *bnd = celix_bundleContext_getBundle(context);
    if (context == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }
    if (out != NULL) {
        *out = bnd;
    }

    framework_logIfError(logger, status, NULL, "Failed to get bundle");

	return status;
}

celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*framework = context->framework;
	}

	framework_logIfError(logger, status, NULL, "Failed to get framework");

	return status;
}

celix_status_t bundleContext_installBundle(bundle_context_pt context, const char * location, bundle_pt *bundle) {
	return bundleContext_installBundle2(context, location, NULL, bundle);
}

celix_status_t bundleContext_installBundle2(bundle_context_pt context, const char *location, const char *inputFile, bundle_pt *bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_pt b = NULL;

	if (context != NULL) {
		if (fw_installBundle(context->framework, &b, location, inputFile) != CELIX_SUCCESS) {
            status = CELIX_FRAMEWORK_EXCEPTION;
		} else {
			*bundle = b;
		}
	} else {
        status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to install bundle");

	return status;
}

celix_status_t bundleContext_registerService(bundle_context_pt context, const char * serviceName, const void * svcObj,
        properties_pt properties, service_registration_pt *service_registration) {
	service_registration_pt registration = NULL;
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
	    fw_registerService(context->framework, &registration, context->bundle, serviceName, svcObj, properties);
	    *service_registration = registration;
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to register service. serviceName '%s'", serviceName);

	return status;
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, const char * serviceName, service_factory_pt factory,
        properties_pt properties, service_registration_pt *service_registration) {
    service_registration_pt registration = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_registration == NULL) {
        fw_registerServiceFactory(context->framework, &registration, context->bundle, serviceName, factory, properties);
        *service_registration = registration;
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to register service factory");

    return status;
}

celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, const char * filter, array_list_pt *service_references) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_references == NULL) {
        fw_getServiceReferences(context->framework, service_references, context->bundle, serviceName, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service references");

	return status;
}

celix_status_t bundleContext_getServiceReference(bundle_context_pt context, const char * serviceName, service_reference_pt *service_reference) {
    service_reference_pt reference = NULL;
    array_list_pt services = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (serviceName != NULL) {
        if (bundleContext_getServiceReferences(context, serviceName, NULL, &services) == CELIX_SUCCESS) {
            reference = (arrayList_size(services) > 0) ? arrayList_get(services, 0) : NULL;
            arrayList_destroy(services);
            *service_reference = reference;
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service reference");

	return status;
}

FRAMEWORK_EXPORT celix_status_t bundleContext_retainServiceReference(bundle_context_pt context, service_reference_pt ref) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && ref != NULL) {
        serviceRegistry_retainServiceReference(context->framework->registry, context->bundle, ref);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service references");

    return status;
}

celix_status_t bundleContext_ungetServiceReference(bundle_context_pt context, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        status = framework_ungetServiceReference(context->framework, context->bundle, reference);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to unget service_reference");

    return status;
}

celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void** service_instance) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL && service_instance != NULL) {
        /*NOTE argument service_instance should be considerd a 'const void**'. 
        To ensure backwards compatiblity a cast is made instead.
        */
	    status = fw_getService(context->framework, context->bundle, reference, (const void**) service_instance);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service");

    return status;
}

celix_status_t bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        status = framework_ungetService(context->framework, context->bundle, reference, result);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to unget service");

    return status;
}

celix_status_t bundleContext_getBundles(bundle_context_pt context, array_list_pt *bundles) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL || *bundles != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*bundles = framework_getBundles(context->framework);
	}

	framework_logIfError(logger, status, NULL, "Failed to get bundles");

	return status;
}

celix_status_t bundleContext_getBundleById(bundle_context_pt context, long id, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;

    if (context == NULL || *bundle != NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *bundle = framework_getBundleById(context->framework, id);
        if (*bundle == NULL) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    framework_logIfError(logger, status, NULL, "Failed to get bundle [id=%ld]", id);

	return status;
}

celix_status_t bundleContext_addServiceListener(bundle_context_pt context, celix_service_listener_t *listener, const char* filter) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addServiceListener(context->framework, context->bundle, listener, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to add service listener");

    return status;
}

celix_status_t bundleContext_removeServiceListener(bundle_context_pt context, celix_service_listener_t *listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeServiceListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to remove service listener");

    return status;
}

celix_status_t bundleContext_addBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to add bundle listener");

    return status;
}

celix_status_t bundleContext_removeBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to remove bundle listener");

    return status;
}

celix_status_t bundleContext_addFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addFrameworkListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to add framework listener");

    return status;
}

celix_status_t bundleContext_removeFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeFrameworkListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to remove framework listener");

    return status;
}

celix_status_t bundleContext_getProperty(bundle_context_pt context, const char *name, const char** value) {
	return bundleContext_getPropertyWithDefault(context, name, NULL, value);
}

celix_status_t bundleContext_getPropertyWithDefault(bundle_context_pt context, const char* name, const char* defaultValue, const char** out) {
    const char *val = celix_bundleContext_getProperty(context, name, defaultValue);
    if (out != NULL) {
        *out = val;
    }
    return CELIX_SUCCESS;
}










/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/


long celix_bundleContext_registerService(bundle_context_t *ctx, void *svc, const char *serviceName, celix_properties_t *properties) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.svc = svc;
    opts.serviceName = serviceName;
    opts.properties = properties;
    return celix_bundleContext_registerServiceWithOptions(ctx, &opts);
}


long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.factory = factory;
    opts.serviceName = serviceName;
    opts.properties = props;
    return celix_bundleContext_registerServiceWithOptions(ctx, &opts);
}

long celix_bundleContext_registerServiceWithOptions(bundle_context_t *ctx, const celix_service_registration_options_t *opts) {
    long svcId = -1;
    service_registration_t *reg = NULL;
    celix_properties_t *props = opts->properties;
    if (props == NULL) {
        props = celix_properties_create();
    }
    if (opts->serviceVersion != NULL && strncmp("", opts->serviceVersion, 1) != 0) {
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, opts->serviceVersion);
    }
    const char *lang = opts->serviceLanguage != NULL && strncmp("", opts->serviceLanguage, 1) != 0 ? opts->serviceLanguage : CELIX_FRAMEWORK_SERVICE_C_LANGUAGE;
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang);
    if (opts->serviceName != NULL && strncmp("", opts->serviceName, 1) != 0) {
        if (opts->factory != NULL) {
            reg = celix_framework_registerServiceFactory(ctx->framework, ctx->bundle, opts->serviceName, opts->factory, props);
        } else {
            bundleContext_registerService(ctx, opts->serviceName, opts->svc, props, &reg);
        }
        svcId = serviceRegistration_getServiceId(reg); //save to call with NULL
    } else {
        framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL, "Required serviceName argument is NULL");
    }
    if (svcId < 0) {
        properties_destroy(props);
    } else {
        celixThreadMutex_lock(&ctx->mutex);
        arrayList_add(ctx->svcRegistrations, reg);
        celixThreadMutex_unlock(&ctx->mutex);
    }
    return svcId;
}

void celix_bundleContext_unregisterService(bundle_context_t *ctx, long serviceId) {
    service_registration_t *found = NULL;
    if (ctx != NULL && serviceId >= 0) {
        celixThreadMutex_lock(&ctx->mutex);
        unsigned int size = arrayList_size(ctx->svcRegistrations);
        for (unsigned int i = 0; i < size; ++i) {
            service_registration_t *reg = arrayList_get(ctx->svcRegistrations, i);
            if (reg != NULL) {
                long svcId = serviceRegistration_getServiceId(reg);
                if (svcId == serviceId) {
                    found = reg;
                    arrayList_remove(ctx->svcRegistrations, i);
                    break;
                }
            }
        }
        celixThreadMutex_unlock(&ctx->mutex);

        if (found != NULL) {
            serviceRegistration_unregister(found);
        } else {
            framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL,
                                 "Provided service id is not used to registered using bundleContext_registerCService/registerServiceForLang");
        }
    }
}

celix_dependency_manager_t* celix_bundleContext_getDependencyManager(bundle_context_t *ctx) {
    celix_dependency_manager_t* result = NULL;
    if (ctx != NULL) {
        celixThreadMutex_lock(&ctx->mutex);
        if (ctx->mng == NULL) {
            ctx->mng = celix_private_dependencyManager_create(ctx);
        }
        if (ctx->mng == NULL) {
            framework_logIfError(logger, CELIX_BUNDLE_EXCEPTION, NULL, "Cannot create dependency manager");
        }
        result = ctx->mng;
        celixThreadMutex_unlock(&ctx->mutex);
    }
    return result;
}

static celix_status_t bundleContext_bundleChanged(void *listenerSvc, bundle_event_t *event) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_listener_t *listener = listenerSvc;
    celix_bundle_context_bundle_tracker_entry_t *tracker = NULL;
    if (listener != NULL) {
        tracker = listener->handle;
    }

    bool handleEvent = true;
    if (event->bundleId == 0 /*framework bundle*/)  {
        handleEvent = tracker->opts.includeFrameworkBundle;
    }

    if (tracker != NULL && handleEvent) {
        void *callbackHandle = tracker->opts.callbackHandle;

        if (event->type == OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED && tracker->opts.onStarted != NULL) {
            bundle_t *bnd = framework_getBundleById(tracker->ctx->framework, event->bundleId);
            tracker->opts.onStarted(callbackHandle, bnd);
        } else if (event->type == OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING && tracker->opts.onStopped != NULL) {
            bundle_t *bnd = framework_getBundleById(tracker->ctx->framework, event->bundleId);
            tracker->opts.onStopped(callbackHandle, bnd);
        }

        if (tracker->opts.onBundleEvent != NULL) {
            tracker->opts.onBundleEvent(callbackHandle, event);
        }
    }
    return status;
}

long celix_bundleContext_trackBundlesWithOptions(
        bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts) {
    long trackerId = -1;
    celix_bundle_context_bundle_tracker_entry_t *entry = calloc(1, sizeof(*entry));
    if (entry != NULL) {
        memcpy(&entry->opts, opts, sizeof(*opts));
        entry->ctx = ctx;
        entry->listener.handle = entry;
        entry->listener.bundleChanged = bundleContext_bundleChanged;
        fw_addBundleListener(ctx->framework, ctx->bundle, &entry->listener);

        celixThreadMutex_lock(&ctx->mutex);
        entry->trackerId = ctx->nextTrackerId++;
        celixThreadMutex_unlock(&ctx->mutex);
        trackerId = entry->trackerId;

        //loop through all already installed bundles.
        // FIXME there is a race condition between installing the listener and looping through the started bundles.
        // NOTE move this to the framework, so that the framework can ensure locking to ensure not bundles is missed.
        if (entry->opts.onStarted != NULL) {
            celix_framework_useBundles(ctx->framework, entry->opts.includeFrameworkBundle, entry->opts.callbackHandle, entry->opts.onStarted);
        }

        celixThreadMutex_lock(&ctx->mutex);
        hashMap_put(ctx->bundleTrackers, (void*)entry->trackerId, entry);
        celixThreadMutex_unlock(&ctx->mutex);

    }
    return trackerId;
}

long celix_bundleContext_trackBundles(
        bundle_context_t* ctx,
        void* callbackHandle,
        void (*onStarted)(void* handle, const bundle_t *bundle),
        void (*onStopped)(void *handle, const bundle_t *bundle)) {
    celix_bundle_tracking_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.callbackHandle = callbackHandle;
    opts.onStarted = onStarted;
    opts.onStopped = onStopped;
    return celix_bundleContext_trackBundlesWithOptions(ctx, &opts);
}


void celix_bundleContext_useBundles(
        bundle_context_t *ctx,
        void *callbackHandle,
        void (*use)(void *handle, const bundle_t *bundle)) {
    celix_framework_useBundles(ctx->framework, false, callbackHandle, use);
}

void celix_bundleContext_useBundle(
        bundle_context_t *ctx,
        long bundleId,
        void *callbackHandle,
        void (*use)(void *handle, const bundle_t *bundle)) {
    celix_framework_useBundle(ctx->framework, true, bundleId, callbackHandle, use);
}

static void bundleContext_cleanupBundleTrackers(bundle_context_t *ctx) {
    hash_map_iterator_t iter = hashMapIterator_construct(ctx->bundleTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_bundle_context_bundle_tracker_entry_t *tracker = hashMapIterator_nextValue(&iter);
        fw_removeBundleListener(ctx->framework, ctx->bundle, &tracker->listener);
        free(tracker);
    }
    hashMap_destroy(ctx->bundleTrackers, false, false);
}

static void bundleContext_cleanupServiceTrackers(bundle_context_t *ctx) {
    hash_map_iterator_t iter = hashMapIterator_construct(ctx->serviceTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_service_tracker_t *tracker = hashMapIterator_nextValue(&iter);
        celix_serviceTracker_destroy(tracker);
    }
    hashMap_destroy(ctx->serviceTrackers, false, false);
}

static void bundleContext_cleanupServiceTrackerTrackers(bundle_context_t *ctx) {
    hash_map_iterator_t iter = hashMapIterator_construct(ctx->serviceTrackerTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_bundle_context_service_tracker_tracker_entry_t *entry = hashMapIterator_nextValue(&iter);
        serviceRegistration_unregister(entry->hookReg);
        free(entry);
    }
    hashMap_destroy(ctx->serviceTrackerTrackers, false, false);
}


void celix_bundleContext_stopTracker(bundle_context_t *ctx, long trackerId) {
    if (ctx != NULL && trackerId >0) {
        bool found = false;
        celix_bundle_context_bundle_tracker_entry_t *bundleTracker = NULL;
        service_tracker_t *serviceTracker = NULL;
        celix_bundle_context_service_tracker_tracker_entry_t *svcTrackerTracker = NULL;

        celixThreadMutex_lock(&ctx->mutex);
        if (hashMap_containsKey(ctx->bundleTrackers, (void*)trackerId)) {
            found = true;
            bundleTracker = hashMap_remove(ctx->bundleTrackers, (void*)trackerId);
        } else if (hashMap_containsKey(ctx->serviceTrackers, (void*)trackerId)) {
            found = true;
            serviceTracker = hashMap_remove(ctx->serviceTrackers, (void*)trackerId);
        } else if (hashMap_containsKey(ctx->serviceTrackerTrackers, (void*)trackerId)) {
            found = true;
            svcTrackerTracker = hashMap_remove(ctx->serviceTrackerTrackers, (void*)trackerId);
        }
        celixThreadMutex_unlock(&ctx->mutex);

        if (bundleTracker != NULL) {
            fw_removeBundleListener(ctx->framework, ctx->bundle, &bundleTracker->listener);
            free(bundleTracker);
        }
        if (serviceTracker != NULL) {
            celix_serviceTracker_destroy(serviceTracker);
        }
        if (svcTrackerTracker != NULL) {
            serviceRegistration_unregister(svcTrackerTracker->hookReg);
            free(svcTrackerTracker->serviceName);
            free(svcTrackerTracker);
        }

        if (!found) {
            framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL, "No tracker with id %li found'", trackerId);
        }
    }
}

long celix_bundleContext_installBundle(bundle_context_t *ctx, const char *bundleLoc, bool autoStart) {
    long bundleId = -1;
    bundle_t *bnd = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (ctx != NULL && fw_installBundle(ctx->framework, &bnd, bundleLoc, NULL) == CELIX_SUCCESS) {
        status = bundle_getBundleId(bnd, &bundleId);
        if (status == CELIX_SUCCESS && autoStart) {
            status = bundle_start(bnd);
            if (status != CELIX_SUCCESS) {
                bundleId = -1;
                bundle_uninstall(bnd);
            }
        }
    }

    framework_logIfError(logger, status, NULL, "Failed to install bundle");

    return bundleId;
}

static void bundleContext_listBundlesCallback(void *handle, const bundle_t *c_bnd) {
    celix_array_list_t* ids = handle;
    long id = celix_bundle_getId(c_bnd);
    if (id > 0) { //note skipping framework bundle id (0)
        celix_arrayList_addLong(ids, id);
    }
}

celix_array_list_t* celix_bundleContext_listBundles(celix_bundle_context_t *ctx) {
    celix_array_list_t *result = celix_arrayList_create();
    celix_bundleContext_useBundles(ctx, result, bundleContext_listBundlesCallback);
    return result;
}

static void bundleContext_startBundleCallback(void *handle, const bundle_t *c_bnd) {
    bool *started = handle;
    *started = false;
    bundle_t *bnd = (bundle_t*)c_bnd;
    celix_bundle_state_e state = celix_bundle_getState(bnd);
    if (state == OSGI_FRAMEWORK_BUNDLE_INSTALLED || state == OSGI_FRAMEWORK_BUNDLE_RESOLVED) {
        celix_status_t rc = bundle_start(bnd);
        *started = rc == CELIX_SUCCESS;
    }
}

bool celix_bundleContext_startBundle(celix_bundle_context_t *ctx, long bundleId) {
    bool started = false;

    celix_framework_useBundle(ctx->framework, false, bundleId, &started, bundleContext_startBundleCallback);
    return started;
}


static void bundleContext_stopBundleCallback(void *handle, const bundle_t *c_bnd) {
    bool *stopped = handle;
    *stopped = false;
    bundle_t *bnd = (bundle_t*)c_bnd;
    if (celix_bundle_getState(bnd) == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
        celix_status_t rc = bundle_stop(bnd);
        *stopped = rc == CELIX_SUCCESS;
    }
}

bool celix_bundleContext_stopBundle(celix_bundle_context_t *ctx, long bundleId) {
    bool stopped = false;
    celix_framework_useBundle(ctx->framework, true, bundleId, &stopped, bundleContext_stopBundleCallback);
    return stopped;
}

static void bundleContext_uninstallBundleCallback(void *handle, const bundle_t *c_bnd) {
    bool *uninstalled = handle;
    bundle_t *bnd = (bundle_t*)c_bnd; //TODO use mute-able variant ??
    if (celix_bundle_getState(bnd) == OSGI_FRAMEWORK_BUNDLE_ACTIVE) {
        bundle_stop(bnd);
    }
    bundle_uninstall(bnd);
    *uninstalled = true;
}

bool celix_bundleContext_uninstallBundle(bundle_context_t *ctx, long bundleId) {
    bool uninstalled = false;
    celix_framework_useBundle(ctx->framework, true, bundleId, &uninstalled, bundleContext_uninstallBundleCallback);
    return uninstalled;
}

bool celix_bundleContext_useServiceWithId(
        bundle_context_t *ctx,
        long serviceId,
        const char *serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)) {
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;

    char filter[64];
    snprintf(filter, 64, "(%s=%li)", OSGI_FRAMEWORK_SERVICE_ID, serviceId);

    opts.filter.serviceName = serviceName;
    opts.filter.filter = filter;
    opts.callbackHandle = callbackHandle;
    opts.use = use;
    return celix_bundleContext_useServiceWithOptions(ctx, &opts);
}

bool celix_bundleContext_useService(
        bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)) {
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.use = use;
    return celix_bundleContext_useServiceWithOptions(ctx, &opts);
}


void celix_bundleContext_useServices(
        bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)) {
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.use = use;
    celix_bundleContext_useServicesWithOptions(ctx, &opts);
}


bool celix_bundleContext_useServiceWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts) {
    bool called = false;
    celix_service_tracking_options_t trkOpts;
    memset(&trkOpts, 0, sizeof(trkOpts));

    if (opts != NULL) {
        trkOpts.filter.serviceName = opts->filter.serviceName;
        trkOpts.filter = opts->filter;
        trkOpts.filter.versionRange = opts->filter.versionRange;
        trkOpts.filter.serviceLanguage = opts->filter.serviceLanguage;

        service_tracker_t *trk = celix_serviceTracker_createWithOptions(ctx, &trkOpts);
        if (trk != NULL) {
            if (opts->use != NULL) {

            }
            if (opts->useWithProperties != NULL) {

            }
            if (opts->useWithOwner != NULL) {

            }
            called = celix_serviceTracker_useHighestRankingService(trk, opts->filter.serviceName, opts->callbackHandle, opts->use, opts->useWithProperties, opts->useWithOwner);
            celix_serviceTracker_destroy(trk);
        }
    }
    return called;
}



void celix_bundleContext_useServicesWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts) {
    celix_service_tracking_options_t trkOpts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;

    if (opts != NULL) {
        trkOpts.filter.serviceName = opts->filter.serviceName;
        trkOpts.filter.filter = opts->filter.filter;
        trkOpts.filter.versionRange = opts->filter.versionRange;
        trkOpts.filter.serviceLanguage = opts->filter.serviceLanguage;

        service_tracker_t *trk = celix_serviceTracker_createWithOptions(ctx, &trkOpts);
        if (trk != NULL) {
            celix_serviceTracker_useServices(trk, opts->filter.serviceName, opts->callbackHandle, opts->use, opts->useWithProperties, opts->useWithOwner);
            celix_serviceTracker_destroy(trk);
        }
    }
}


long celix_bundleContext_trackService(
        bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*set)(void* handle, void* svc)) {
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.set = set;
    return celix_bundleContext_trackServicesWithOptions(ctx, &opts);
}


long celix_bundleContext_trackServices(
        bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*add)(void* handle, void* svc),
        void (*remove)(void* handle, void* svc)) {
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.add = add;
    opts.remove = remove;
    return celix_bundleContext_trackServicesWithOptions(ctx, &opts);
}


long celix_bundleContext_trackServicesWithOptions(bundle_context_t *ctx, const celix_service_tracking_options_t *opts) {
    long trackerId = -1;
    celix_service_tracker_t *tracker = celix_serviceTracker_createWithOptions(ctx, opts);
    if (tracker != NULL) {
        celixThreadMutex_lock(&ctx->mutex);
        trackerId = ctx->nextTrackerId++;
        hashMap_put(ctx->serviceTrackers, (void*)trackerId, tracker);
        celixThreadMutex_unlock(&ctx->mutex);
    }
    return trackerId;
}

long celix_bundleContext_findService(celix_bundle_context_t *ctx, const char *serviceName) {
    celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
    opts.serviceName = serviceName;
    return celix_bundleContext_findServiceWithOptions(ctx, &opts);
}

static void bundleContext_retrieveSvcId(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
    long *svcId = handle;
    *svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
}

long celix_bundleContext_findServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts) {
    long svcId = -1L;
    celix_service_use_options_t useOpts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    memcpy(&useOpts.filter, opts, sizeof(useOpts.filter));
    useOpts.callbackHandle = &svcId;
    useOpts.useWithProperties = bundleContext_retrieveSvcId;
    celix_bundleContext_useServiceWithOptions(ctx, &useOpts);
    return svcId;
}


celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char *serviceName) {
    celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
    opts.serviceName = serviceName;
    return celix_bundleContext_findServicesWithOptions(ctx, &opts);
}

static void bundleContext_retrieveSvcIds(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
    celix_array_list_t *list = handle;
    if (list != NULL) {
        long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
        celix_arrayList_addLong(list, svcId);
    }
}

celix_array_list_t* celix_bundleContext_findServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts) {
    celix_array_list_t* list = celix_arrayList_create();
    celix_service_use_options_t useOpts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    memcpy(&useOpts.filter, opts, sizeof(useOpts.filter));
    useOpts.callbackHandle = list;
    useOpts.useWithProperties = bundleContext_retrieveSvcIds;
    celix_bundleContext_useServicesWithOptions(ctx, &useOpts);
    return list;
}

static celix_status_t bundleContext_callServicedTrackerTrackerCallback(void *handle, celix_array_list_t *listeners, bool add) {
    celix_bundle_context_service_tracker_tracker_entry_t *entry = handle;
    if (entry != NULL) {
        size_t size = celix_arrayList_size(listeners);
        for (unsigned int i = 0; i < size; ++i) {
            listener_hook_info_pt info = arrayList_get(listeners, i);
            celix_bundle_t *bnd = NULL;
            bundleContext_getBundle(info->context, &bnd);

            celix_service_tracker_info_t trkInfo;
            memset(&trkInfo, 0, sizeof(trkInfo));
            trkInfo.bundleId = celix_bundle_getId(bnd);
            trkInfo.filter = celix_filter_create(info->filter);
            trkInfo.serviceName = celix_filter_findAttribute(trkInfo.filter, OSGI_FRAMEWORK_OBJECTCLASS);
            trkInfo.serviceLanguage = celix_filter_findAttribute(trkInfo.filter, CELIX_FRAMEWORK_SERVICE_LANGUAGE);
            const char *filterSvcName = celix_filter_findAttribute(trkInfo.filter, OSGI_FRAMEWORK_OBJECTCLASS);

            if (add && entry->add != NULL && filterSvcName != NULL && strncmp(filterSvcName, entry->serviceName, 1024*1024) == 0) {
                entry->add(entry->callbackHandle, &trkInfo);
            } else if (entry->remove != NULL && filterSvcName != NULL && strncmp(filterSvcName, entry->serviceName, 1024*1024) == 0) {
                entry->remove(entry->callbackHandle, &trkInfo);
            }
            celix_filter_destroy(trkInfo.filter);
        }
    }
    return CELIX_SUCCESS;
}

static celix_status_t bundleContext_callServicedTrackerTrackerAdd(void *handle, celix_array_list_t *listeners) {
    return bundleContext_callServicedTrackerTrackerCallback(handle, listeners, true);
}

static celix_status_t bundleContext_callServicedTrackerTrackerRemove(void *handle, celix_array_list_t *listeners) {
    return bundleContext_callServicedTrackerTrackerCallback(handle, listeners, false);
}

long celix_bundleContext_trackServiceTrackers(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info)) {

    long trackerId = -1L;

    if (serviceName == NULL) {
        fw_log(ctx->framework->logger, OSGI_FRAMEWORK_LOG_ERROR, "Required serviceName not provided for function ", __FUNCTION__);
        return trackerId;
    }

    celix_bundle_context_service_tracker_tracker_entry_t *entry = calloc(1, sizeof(*entry));

    entry->callbackHandle = callbackHandle;
    entry->add = trackerAdd;
    entry->remove = trackerRemove;
    entry->serviceName = strndup(serviceName, 1024*1024);

    entry->hook.handle = entry;
    entry->hook.added = bundleContext_callServicedTrackerTrackerAdd;
    entry->hook.removed = bundleContext_callServicedTrackerTrackerRemove;
    bundleContext_registerService(ctx, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, &entry->hook, NULL, &entry->hookReg);

    if (entry->hookReg != NULL) {
        celixThreadMutex_lock(&ctx->mutex);
        entry->trackerId = ctx->nextTrackerId++;
        hashMap_put(ctx->serviceTrackerTrackers, (void*)entry->trackerId, entry);
        trackerId = entry->trackerId;
        celixThreadMutex_unlock(&ctx->mutex);
    } else {
        framework_log(ctx->framework->logger, OSGI_FRAMEWORK_LOG_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error registering service listener hook for service tracker tracker\n");
        free(entry);
    }
    return trackerId;
}

celix_bundle_t* celix_bundleContext_getBundle(celix_bundle_context_t *ctx) {
    celix_bundle_t *bnd = NULL;
    if (ctx != NULL) {
        bnd = ctx->bundle;
    }
    return bnd;
}


const char* celix_bundleContext_getProperty(celix_bundle_context_t *ctx, const char *key, const char *defaultVal) {
    const char *val = NULL;
    if (ctx != NULL) {
        fw_getProperty(ctx->framework, key, defaultVal, &val);
    }
    return val;
}

long celix_bundleContext_getPropertyAsLong(celix_bundle_context_t *ctx, const char *key, long defaultValue) {
    long result = defaultValue;
    const char *val = celix_bundleContext_getProperty(ctx, key, NULL);
    if (val != NULL) {
        char *enptr = NULL;
        errno = 0;
        long r = strtol(val, &enptr, 10);
        if (enptr != val && errno == 0) {
            result = r;
        }
    }
    return result;
}

double celix_bundleContext_getPropertyAsDouble(celix_bundle_context_t *ctx, const char *key, double defaultValue) {
    double result = defaultValue;
    const char *val = celix_bundleContext_getProperty(ctx, key, NULL);
    if (val != NULL) {
        char *enptr = NULL;
        errno = 0;
        double r = strtod(val, &enptr);
        if (enptr != val && errno == 0) {
            result = r;
        }
    }
    return result;
}


bool celix_bundleContext_getPropertyAsBool(celix_bundle_context_t *ctx, const char *key, bool defaultValue) {
    bool result = defaultValue;
    const char *val = celix_bundleContext_getProperty(ctx, key, NULL);
    if (val != NULL) {
        char buf[32];
        snprintf(buf, 32, "%s", val);
        char *trimmed = utils_stringTrim(buf);
        if (strncasecmp("true", trimmed, strlen("true")) == 0) {
            result = true;
        } else if (strncasecmp("false", trimmed, strlen("false")) == 0) {
            result = false;
        }
    }
    return result;
}
