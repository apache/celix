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
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <assert.h>

#include "celix_utils.h"
#include "celix_constants.h"
#include "bundle_context_private.h"
#include "framework_private.h"
#include "bundle.h"
#include "celix_bundle.h"
#include "celix_log.h"
#include "service_tracker.h"
#include "celix_dependency_manager.h"
#include "dm_dependency_manager_impl.h"
#include "celix_array_list.h"
#include "module.h"
#include "service_tracker_private.h"
#include "celix_array_list.h"

static celix_status_t bundleContext_bundleChanged(void *handle, bundle_event_t *event);
static void bundleContext_cleanupBundleTrackers(bundle_context_t *ct);
static void bundleContext_cleanupServiceTrackers(bundle_context_t *ctx);
static void bundleContext_cleanupServiceTrackerTrackers(bundle_context_t *ctx);
static void bundleContext_cleanupServiceRegistration(bundle_context_t* ctx);

celix_status_t bundleContext_create(framework_pt framework, celix_framework_logger_t*  logger, bundle_pt bundle, bundle_context_pt *bundle_context) {
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
            context->metaTrackers =  hashMap_create(NULL,NULL,NULL,NULL);
            context->nextTrackerId = 1L;

            *bundle_context = context;

        }
	}

	framework_logIfError(context->framework->logger, status, NULL, "Failed to create context");

	return status;
}

celix_status_t bundleContext_destroy(bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
	    celixThreadMutex_lock(&context->mutex);


	    assert(hashMap_size(context->bundleTrackers) == 0);
        hashMap_destroy(context->bundleTrackers, false, false);
        assert(hashMap_size(context->serviceTrackers) == 0);
        hashMap_destroy(context->serviceTrackers, false, false);
        assert(hashMap_size(context->metaTrackers) == 0);
        hashMap_destroy(context->metaTrackers, false, false);
        assert(celix_arrayList_size(context->svcRegistrations) == 0);
        celix_arrayList_destroy(context->svcRegistrations);

        //NOTE still present service registrations will be cleared during bundle stop in the
	    //service registry (serviceRegistry_clearServiceRegistrations).
        celixThreadMutex_unlock(&context->mutex);
	    celixThreadMutex_destroy(&context->mutex);

	    if (context->mng != NULL) {
	        celix_dependencyManager_removeAllComponents(context->mng);
            celix_private_dependencyManager_destroy(context->mng);
            context->mng = NULL;
	    }

	    free(context);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(context->framework->logger, status, NULL, "Failed to destroy context");

	return status;
}

void celix_bundleContext_cleanup(celix_bundle_context_t *ctx) {
    //NOTE not perfect, because stopping of registrations/tracker when the activator is destroyed can lead to segfault.
    //but atleast we can try to warn the bundle implementer that some cleanup is missing.
    bundleContext_cleanupBundleTrackers(ctx);
    bundleContext_cleanupServiceTrackers(ctx);
    bundleContext_cleanupServiceTrackerTrackers(ctx);
    bundleContext_cleanupServiceRegistration(ctx);
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

    framework_logIfError(context->framework->logger, status, NULL, "Failed to get bundle");

	return status;
}

celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*framework = context->framework;
	}

	framework_logIfError(context->framework->logger, status, NULL, "Failed to get framework");

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

	framework_logIfError(context->framework->logger, status, NULL, "Failed to install bundle");

	return status;
}

celix_status_t bundleContext_registerService(bundle_context_pt context, const char * serviceName, const void * svcObj,
        properties_pt properties, service_registration_pt *service_registration) {
	service_registration_pt registration = NULL;
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
	    long bndId = celix_bundle_getId(context->bundle);
	    fw_registerService(context->framework, &registration, bndId, serviceName, svcObj, properties);
	    *service_registration = registration;
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(context->framework->logger, status, NULL, "Failed to register service. serviceName '%s'", serviceName);

	return status;
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, const char * serviceName, service_factory_pt factory,
        properties_pt properties, service_registration_pt *service_registration) {
    service_registration_pt registration = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_registration == NULL) {
        long bndId = celix_bundle_getId(context->bundle);
        fw_registerServiceFactory(context->framework, &registration, bndId, serviceName, factory, properties);
        *service_registration = registration;
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to register service factory");

    return status;
}

celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, const char * filter, array_list_pt *service_references) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_references == NULL) {
        fw_getServiceReferences(context->framework, service_references, context->bundle, serviceName, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to get service references");

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

    framework_logIfError(context->framework->logger, status, NULL, "Failed to get service reference");

	return status;
}

FRAMEWORK_EXPORT celix_status_t bundleContext_retainServiceReference(bundle_context_pt context, service_reference_pt ref) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && ref != NULL) {
        serviceRegistry_retainServiceReference(context->framework->registry, context->bundle, ref);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to get service references");

    return status;
}

celix_status_t bundleContext_ungetServiceReference(bundle_context_pt context, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        status = framework_ungetServiceReference(context->framework, context->bundle, reference);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to unget service_reference");

    return status;
}

celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void** service_instance) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL && service_instance != NULL) {
        /*NOTE argument service_instance should be considered a 'const void**'.
        To ensure backwards compatibility a cast is made instead.
        */
	    status = fw_getService(context->framework, context->bundle, reference, (const void**) service_instance);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status != CELIX_SUCCESS) {
        fw_log(context->framework->logger, CELIX_LOG_LEVEL_ERROR, "Failed to get service");
    }

    return status;
}

celix_status_t bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        status = framework_ungetService(context->framework, context->bundle, reference, result);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to unget service");

    return status;
}

celix_status_t bundleContext_getBundles(bundle_context_pt context, array_list_pt *bundles) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL || *bundles != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*bundles = framework_getBundles(context->framework);
	}

	framework_logIfError(context->framework->logger, status, NULL, "Failed to get bundles");

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

    framework_logIfError(context->framework->logger, status, NULL, "Failed to get bundle [id=%ld]", id);

	return status;
}

celix_status_t bundleContext_addServiceListener(bundle_context_pt context, celix_service_listener_t *listener, const char* filter) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addServiceListener(context->framework, context->bundle, listener, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to add service listener");

    return status;
}

celix_status_t bundleContext_removeServiceListener(bundle_context_pt context, celix_service_listener_t *listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeServiceListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to remove service listener");

    return status;
}

celix_status_t bundleContext_addBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to add bundle listener");

    return status;
}

celix_status_t bundleContext_removeBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to remove bundle listener");

    return status;
}

celix_status_t bundleContext_addFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addFrameworkListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to add framework listener");

    return status;
}

celix_status_t bundleContext_removeFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeFrameworkListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(context->framework->logger, status, NULL, "Failed to remove framework listener");

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

long celix_bundleContext_registerServiceAsync(celix_bundle_context_t *ctx, void *svc, const char *serviceName, celix_properties_t *properties) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.svc = svc;
    opts.serviceName = serviceName;
    opts.properties = properties;
    return celix_bundleContext_registerServiceAsyncWithOptions(ctx, &opts);
}

long celix_bundleContext_registerService(bundle_context_t *ctx, void *svc, const char *serviceName, celix_properties_t *properties) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.svc = svc;
    opts.serviceName = serviceName;
    opts.properties = properties;
    return celix_bundleContext_registerServiceWithOptions(ctx, &opts);
}


long celix_bundleContext_registerServiceFactoryAsync(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.factory = factory;
    opts.serviceName = serviceName;
    opts.properties = props;
    return celix_bundleContext_registerServiceAsyncWithOptions(ctx, &opts);
}

long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props) {
    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    opts.factory = factory;
    opts.serviceName = serviceName;
    opts.properties = props;
    return celix_bundleContext_registerServiceWithOptions(ctx, &opts);
}

static long celix_bundleContext_registerServiceWithOptionsInternal(bundle_context_t *ctx, const celix_service_registration_options_t *opts, bool async) {
    bool valid = opts->serviceName != NULL && strncmp("", opts->serviceName, 1) != 0;
    if (!valid) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Required serviceName argument is NULL or empty");
        return -1;
    }
    valid = opts->svc != NULL || opts->factory != NULL;
    if (!valid) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Required svc or factory argument is NULL");
        return -1;
    }

    //set properties
    celix_properties_t *props = opts->properties;
    if (props == NULL) {
        props = celix_properties_create();
    }
    if (opts->serviceVersion != NULL && strncmp("", opts->serviceVersion, 1) != 0) {
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, opts->serviceVersion);
    }
    const char *lang = opts->serviceLanguage != NULL && strncmp("", opts->serviceLanguage, 1) != 0 ? opts->serviceLanguage : CELIX_FRAMEWORK_SERVICE_C_LANGUAGE;
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang);

    long svcId = -1;
    if (!async && celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        /*
         * Note already on event loop, cannot register the service async, because we cannot wait a future event (the
         * service registration) the event loop.
         *
         * So in this case we handle the service registration the "traditional way" and call the sync fw service
         * registrations versions on the event loop thread
         */

        svcId = celix_framework_registerService(ctx->framework, ctx->bundle, opts->serviceName, opts->svc, opts->factory, props);
    } else {
        svcId = celix_framework_registerServiceAsync(ctx->framework, ctx->bundle, opts->serviceName, opts->svc, opts->factory, props, opts->asyncData, opts->asyncCallback);
        if (!async && svcId >= 0) {
            //note on event loop thread, but in a sync call, so waiting till service registration is concluded
            celix_bundleContext_waitForAsyncRegistration(ctx, svcId);
            if (opts->asyncCallback) {
                opts->asyncCallback(opts->asyncData, svcId);
            }
        }
    }


    if (svcId < 0) {
        properties_destroy(props);
    } else {
        celixThreadMutex_lock(&ctx->mutex);
        celix_arrayList_addLong(ctx->svcRegistrations, svcId);
        celixThreadMutex_unlock(&ctx->mutex);
        if (!async) {
            if (opts->asyncCallback) {
                opts->asyncCallback(opts->asyncData, svcId);
            }
        }
    }
    return svcId;
}

long celix_bundleContext_registerServiceWithOptions(bundle_context_t *ctx, const celix_service_registration_options_t *opts) {
    return celix_bundleContext_registerServiceWithOptionsInternal(ctx, opts, false);
}

long celix_bundleContext_registerServiceAsyncWithOptions(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts) {
    return celix_bundleContext_registerServiceWithOptionsInternal(ctx, opts, true);
}

void celix_bundleContext_waitForAsyncRegistration(celix_bundle_context_t* ctx, long serviceId) {
    if (serviceId >= 0) {
        celix_framework_waitForAsyncRegistration(ctx->framework, serviceId);
    }
}

static void celix_bundleContext_unregisterServiceInternal(celix_bundle_context_t *ctx, long serviceId, bool async) {
    long found = -1L;
    if (ctx != NULL && serviceId >= 0) {
        celixThreadMutex_lock(&ctx->mutex);
        int size = celix_arrayList_size(ctx->svcRegistrations);
        for (int i = 0; i < size; ++i) {
            long entryId = celix_arrayList_getLong(ctx->svcRegistrations, i);
            if (entryId == serviceId) {
                celix_arrayList_removeAt(ctx->svcRegistrations, i);
                found = entryId;
                break;
            }
        }
        celixThreadMutex_unlock(&ctx->mutex);

        if (found >= 0) {
            if (async) {
                celix_framework_unregisterAsync(ctx->framework, ctx->bundle, found);
            } else if (celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
                /*
                 * sync unregistration.
                 * Note already on event loop, cannot unregister the service async, because we cannot wait a future event (the
                 * service unregistration) the event loop.
                 *
                 * So in this case we handle the service unregistration the "traditional way" and call the sync fw service
                 * unregistrations versions
                 */
                celix_framework_unregister(ctx->framework, ctx->bundle, found);
            } else {
                celix_framework_unregisterAsync(ctx->framework, ctx->bundle, found);
                celix_bundleContext_waitForAsyncUnregistration(ctx, serviceId);
            }
        } else {
            framework_logIfError(ctx->framework->logger, CELIX_ILLEGAL_ARGUMENT, NULL,
                                 "No service registered with svc id %li for bundle %s (bundle id: %li)!", serviceId,
                                 celix_bundle_getSymbolicName(ctx->bundle), celix_bundle_getId(ctx->bundle));
        }
    }
}

void celix_bundleContext_unregisterServiceAsync(celix_bundle_context_t *ctx, long serviceId) {
    return celix_bundleContext_unregisterServiceInternal(ctx, serviceId, true);
}

void celix_bundleContext_unregisterService(bundle_context_t *ctx, long serviceId) {
    return celix_bundleContext_unregisterServiceInternal(ctx, serviceId, false);
}

void celix_bundleContext_waitForAsyncUnregistration(celix_bundle_context_t* ctx, long serviceId) {
    if (serviceId >= 0) {
        celix_framework_waitForAsyncUnregistration(ctx->framework, serviceId);
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
            framework_logIfError(ctx->framework->logger, CELIX_BUNDLE_EXCEPTION, NULL, "Cannot create dependency manager");
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
    long bndId = celix_bundle_getId(event->bnd);
    if (bndId == 0 /*framework bundle*/)  {
        handleEvent = tracker->opts.includeFrameworkBundle;
    }

    if (tracker != NULL && handleEvent) {
        void *callbackHandle = tracker->opts.callbackHandle;

        if (event->type == OSGI_FRAMEWORK_BUNDLE_EVENT_INSTALLED && tracker->opts.onInstalled != NULL) {
            tracker->opts.onInstalled(callbackHandle, event->bnd);
        } else if (event->type == OSGI_FRAMEWORK_BUNDLE_EVENT_STARTED && tracker->opts.onStarted != NULL) {
            tracker->opts.onStarted(callbackHandle, event->bnd);
        } else if (event->type == OSGI_FRAMEWORK_BUNDLE_EVENT_STOPPING && tracker->opts.onStopped != NULL) {
            tracker->opts.onStopped(callbackHandle, event->bnd);
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
    memcpy(&entry->opts, opts, sizeof(*opts));
    entry->ctx = ctx;
    entry->listener.handle = entry;
    entry->listener.bundleChanged = bundleContext_bundleChanged;
    fw_addBundleListener(ctx->framework, ctx->bundle, &entry->listener);

    celixThreadMutex_lock(&ctx->mutex);
    entry->trackerId = ctx->nextTrackerId++;
    hashMap_put(ctx->bundleTrackers, (void*)entry->trackerId, entry);
    celixThreadMutex_unlock(&ctx->mutex);
    trackerId = entry->trackerId;
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

bool celix_bundleContext_useBundle(
        bundle_context_t *ctx,
        long bundleId,
        void *callbackHandle,
        void (*use)(void *handle, const bundle_t *bundle)) {
    return celix_framework_useBundle(ctx->framework, true, bundleId, callbackHandle, use);
}

static void bundleContext_cleanupBundleTrackers(bundle_context_t *ctx) {
    module_pt module;
    const char *symbolicName;
    bundle_getCurrentModule(ctx->bundle, &module);
    module_getSymbolicName(module, &symbolicName);

    celix_array_list_t* danglingTrkIds = NULL;

    celixThreadMutex_lock(&ctx->mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(ctx->bundleTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        long trkId = (long)hashMapIterator_nextKey(&iter);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Dangling bundle tracker with id %li for bundle %s. Add missing 'celix_bundleContext_stopTracker' calls.", trkId, symbolicName);
        if (danglingTrkIds == NULL) {
            danglingTrkIds = celix_arrayList_create();
        }
        celix_arrayList_addLong(danglingTrkIds, trkId);
    }
    celixThreadMutex_unlock(&ctx->mutex);

    if (danglingTrkIds != NULL) {
        for (int i = 0; i < celix_arrayList_size(danglingTrkIds); ++i) {
            long trkId = celix_arrayList_getLong(danglingTrkIds, i);
            celix_bundleContext_stopTracker(ctx, trkId);
        }
        celix_arrayList_destroy(danglingTrkIds);
    }
}

static void bundleContext_cleanupServiceTrackers(bundle_context_t *ctx) {
    module_pt module;
    const char *symbolicName;
    bundle_getCurrentModule(ctx->bundle, &module);
    module_getSymbolicName(module, &symbolicName);

    celix_array_list_t* danglingTrkIds = NULL;

    celixThreadMutex_lock(&ctx->mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(ctx->serviceTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        long trkId = (long)hashMapIterator_nextKey(&iter);
        celix_service_tracker_t *entry = hashMap_get(ctx->serviceTrackers, (void*)trkId);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Dangling service tracker with trkId %li, for bundle %s and with filter %s. Add missing 'celix_bundleContext_stopTracker' calls.", trkId, symbolicName, entry->filter);
        if (danglingTrkIds == NULL) {
            danglingTrkIds = celix_arrayList_create();
        }
        celix_arrayList_addLong(danglingTrkIds, trkId);
    }
    celixThreadMutex_unlock(&ctx->mutex);

    if (danglingTrkIds != NULL) {
        for (int i = 0; i < celix_arrayList_size(danglingTrkIds); ++i) {
            long trkId = celix_arrayList_getLong(danglingTrkIds, i);
            celix_bundleContext_stopTracker(ctx, trkId);
        }
        celix_arrayList_destroy(danglingTrkIds);
    }
}

static void bundleContext_cleanupServiceTrackerTrackers(bundle_context_t *ctx) {
    module_pt module;
    const char *symbolicName;
    bundle_getCurrentModule(ctx->bundle, &module);
    module_getSymbolicName(module, &symbolicName);

    celix_array_list_t* danglingTrkIds = NULL;

    celixThreadMutex_lock(&ctx->mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(ctx->metaTrackers);
    while (hashMapIterator_hasNext(&iter)) {
        long trkId = (long)hashMapIterator_nextKey(&iter);
        celix_bundle_context_service_tracker_tracker_entry_t *entry = hashMap_get(ctx->metaTrackers, (void*)trkId);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Dangling meta tracker (service tracker tracker) with trkId %li, for bundle %s and for the services %s. Add missing 'celix_bundleContext_stopTracker' calls.", trkId, symbolicName, entry->serviceName);
        if (danglingTrkIds == NULL) {
            danglingTrkIds = celix_arrayList_create();
        }
        celix_arrayList_addLong(danglingTrkIds, trkId);
    }
    celixThreadMutex_unlock(&ctx->mutex);

    if (danglingTrkIds != NULL) {
        for (int i = 0; i < celix_arrayList_size(danglingTrkIds); ++i) {
            long trkId = celix_arrayList_getLong(danglingTrkIds, i);
            celix_bundleContext_stopTracker(ctx, trkId);
        }
        celix_arrayList_destroy(danglingTrkIds);
    }
}

static void bundleContext_cleanupServiceRegistration(bundle_context_t* ctx) {
    module_pt module;
    const char *symbolicName;
    bundle_getCurrentModule(ctx->bundle, &module);
    module_getSymbolicName(module, &symbolicName);

    celix_array_list_t* danglingSvcIds = NULL;

    celixThreadMutex_lock(&ctx->mutex);
    for (int i = 0; i < celix_arrayList_size(ctx->svcRegistrations); ++i) {
        service_registration_pt reg = celix_arrayList_get(ctx->svcRegistrations, i);
        long svcId = serviceRegistration_getServiceId(reg);
        const char* svcName = NULL;
        serviceRegistration_getServiceName(reg, &svcName);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Dangling service registration with svcId %li, for bundle %s and with service name %s. Add missing 'celix_bundleContext_unregisterService' calls.", svcId, symbolicName, svcName);
        if (danglingSvcIds == NULL) {
            danglingSvcIds = celix_arrayList_create();
        }
        celix_arrayList_addLong(danglingSvcIds, svcId);
    }
    celixThreadMutex_unlock(&ctx->mutex);

    if (danglingSvcIds != NULL) {
        for (int i = 0; i < celix_arrayList_size(danglingSvcIds); ++i) {
            long svcId = celix_arrayList_getLong(danglingSvcIds, i);
            celix_bundleContext_unregisterService(ctx, svcId);
        }
        celix_arrayList_destroy(danglingSvcIds);
    }
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
        } else if (hashMap_containsKey(ctx->metaTrackers, (void*)trackerId)) {
            found = true;
            svcTrackerTracker = hashMap_remove(ctx->metaTrackers, (void*)trackerId);
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
            framework_logIfError(ctx->framework->logger, CELIX_ILLEGAL_ARGUMENT, NULL, "No tracker with id %li found'", trackerId);
        }
    }
}

long celix_bundleContext_installBundle(bundle_context_t *ctx, const char *bundleLoc, bool autoStart) {
    return celix_framework_installBundle(ctx->framework, bundleLoc, autoStart);
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

bool celix_bundleContext_isBundleInstalled(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_isBundleInstalled(ctx->framework, bndId);
}

bool celix_bundleContext_isBundleActive(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_isBundleActive(ctx->framework, bndId);
}

bool celix_bundleContext_startBundle(celix_bundle_context_t *ctx, long bundleId) {
    return celix_framework_startBundle(ctx->framework, bundleId);
}


bool celix_bundleContext_stopBundle(celix_bundle_context_t *ctx, long bundleId) {
    return celix_framework_stopBundle(ctx->framework, bundleId);
}

bool celix_bundleContext_uninstallBundle(bundle_context_t *ctx, long bundleId) {
    return celix_framework_uninstallBundle(ctx->framework, bundleId);
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


size_t celix_bundleContext_useServices(
        bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)) {
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.use = use;
    return celix_bundleContext_useServicesWithOptions(ctx, &opts);
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
            called = celix_serviceTracker_useHighestRankingService(trk, opts->filter.serviceName, opts->waitTimeoutInSeconds, opts->callbackHandle, opts->use, opts->useWithProperties, opts->useWithOwner);
            celix_serviceTracker_destroy(trk);
        }
    }
    return called;
}



size_t celix_bundleContext_useServicesWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts) {
    size_t count = 0;
    celix_service_tracking_options_t trkOpts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    if (opts != NULL) {
        trkOpts.filter.serviceName = opts->filter.serviceName;
        trkOpts.filter.filter = opts->filter.filter;
        trkOpts.filter.versionRange = opts->filter.versionRange;
        trkOpts.filter.serviceLanguage = opts->filter.serviceLanguage;
        trkOpts.filter.ignoreServiceLanguage = opts->filter.ignoreServiceLanguage;

        service_tracker_t *trk = celix_serviceTracker_createWithOptions(ctx, &trkOpts);
        if (trk != NULL) {
            count = celix_serviceTracker_useServices(trk, opts->filter.serviceName, opts->callbackHandle, opts->use, opts->useWithProperties, opts->useWithOwner);
            celix_serviceTracker_destroy(trk);
        }
    }
    return count;
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
    if (ctx == NULL) {
        return -1L;
    } else if (opts == NULL) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot track services with a NULL service tracking options argument");
        return -1L;
    }

    if (opts->filter.serviceName == NULL) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Starting a tracker for any services");
    }

    long trackerId = -1;
    celix_service_tracker_t *tracker = celix_serviceTracker_createWithOptions(ctx, opts);
    if (tracker != NULL) {
        celixThreadMutex_lock(&ctx->mutex);
        trackerId = ctx->nextTrackerId++;
        hashMap_put(ctx->serviceTrackers, (void *) trackerId, tracker);
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

            bool match = entry->serviceName == NULL || (filterSvcName != NULL && strncmp(filterSvcName, entry->serviceName, 1024*1024) == 0);

            if (add && entry->add != NULL && match) {
                entry->add(entry->callbackHandle, &trkInfo);
            } else if (!add && entry->remove != NULL && match) {
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
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Note starting a meta tracker for all services", __FUNCTION__);
    }

    celix_bundle_context_service_tracker_tracker_entry_t *entry = calloc(1, sizeof(*entry));

    entry->callbackHandle = callbackHandle;
    entry->add = trackerAdd;
    entry->remove = trackerRemove;
    entry->serviceName = celix_utils_strdup(serviceName);

    entry->hook.handle = entry;
    entry->hook.added = bundleContext_callServicedTrackerTrackerAdd;
    entry->hook.removed = bundleContext_callServicedTrackerTrackerRemove;
    bundleContext_registerService(ctx, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, &entry->hook, NULL, &entry->hookReg);

    if (entry->hookReg != NULL) {
        celixThreadMutex_lock(&ctx->mutex);
        entry->trackerId = ctx->nextTrackerId++;
        hashMap_put(ctx->metaTrackers, (void*)entry->trackerId, entry);
        trackerId = entry->trackerId;
        celixThreadMutex_unlock(&ctx->mutex);
    } else {
        framework_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error registering service listener hook for service tracker tracker\n");
        free(entry);
    }
    return trackerId;
}

celix_bundle_t* celix_bundleContext_getBundle(const celix_bundle_context_t *ctx) {
    celix_bundle_t *bnd = NULL;
    if (ctx != NULL) {
        bnd = ctx->bundle;
    }
    return bnd;
}

celix_framework_t* celix_bundleContext_getFramework(const celix_bundle_context_t *ctx) {
    celix_framework_t *fw = NULL;
    if (ctx != NULL) {
        fw = ctx->framework;
    }
    return fw;
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

static void celix_bundleContext_getBundleSymbolicNameCallback(void *data, const celix_bundle_t *bnd) {
    char **out = data;
    *out = celix_utils_strdup(celix_bundle_getSymbolicName(bnd));
}

char* celix_bundleContext_getBundleSymbolicName(celix_bundle_context_t *ctx, long bndId) {
    char *name = NULL;
    celix_framework_useBundle(ctx->framework, false, bndId, &name, celix_bundleContext_getBundleSymbolicNameCallback);
    return name;
}
