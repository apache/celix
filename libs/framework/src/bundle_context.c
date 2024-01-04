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
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>

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
#include "service_reference_private.h"
#include "celix_array_list.h"
#include "celix_convert_utils.h"

static celix_status_t bundleContext_bundleChanged(void* listenerSvc, bundle_event_t* event);
static void bundleContext_cleanupBundleTrackers(bundle_context_t *ct);
static void bundleContext_cleanupServiceTrackers(bundle_context_t *ctx);
static void bundleContext_cleanupServiceTrackerTrackers(bundle_context_t *ctx);
static void bundleContext_cleanupServiceRegistration(bundle_context_t* ctx);
static long celix_bundleContext_trackServicesWithOptionsInternal(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts, bool async);

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

            context->svcRegistrations = celix_arrayList_create();
            context->bundleTrackers = hashMap_create(NULL,NULL,NULL,NULL);
            context->serviceTrackers = hashMap_create(NULL,NULL,NULL,NULL);
            context->metaTrackers =  hashMap_create(NULL,NULL,NULL,NULL);
            context->stoppingTrackerEventIds = hashMap_create(NULL,NULL,NULL,NULL);
            context->nextTrackerId = 1L;

            *bundle_context = context;

        }
	}
	framework_logIfError(logger, status, NULL, "Failed to create context");

	return status;
}

celix_status_t bundleContext_destroy(bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

    if(context == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    assert(hashMap_size(context->bundleTrackers) == 0);
    hashMap_destroy(context->bundleTrackers, false, false);
    assert(hashMap_size(context->serviceTrackers) == 0);
    hashMap_destroy(context->serviceTrackers, false, false);
    assert(hashMap_size(context->metaTrackers) == 0);
    hashMap_destroy(context->metaTrackers, false, false);
    assert(celix_arrayList_size(context->svcRegistrations) == 0);
    celix_arrayList_destroy(context->svcRegistrations);
    hashMap_destroy(context->stoppingTrackerEventIds, false, false);

    celixThreadMutex_destroy(&context->mutex);

    if (context->mng != NULL) {
        celix_dependencyManager_removeAllComponents(context->mng);
        celix_private_dependencyManager_destroy(context->mng);
        context->mng = NULL;
    }

    free(context);
	return status;
}

void celix_bundleContext_cleanup(celix_bundle_context_t* ctx) {
        fw_log(ctx->framework->logger,
               CELIX_LOG_LEVEL_TRACE,
               "Cleaning up bundle context `%s` (id=%li)",
               celix_bundle_getSymbolicName(ctx->bundle),
               celix_bundle_getId(ctx->bundle));

        celix_framework_cleanupScheduledEvents(ctx->framework, celix_bundle_getId(ctx->bundle));
        // NOTE not perfect, because stopping of registrations/tracker when the activator is destroyed can lead to
        // segfault. but at least we can try to warn the bundle implementer that some cleanup is missing.
        bundleContext_cleanupBundleTrackers(ctx);
        bundleContext_cleanupServiceTrackers(ctx);
        bundleContext_cleanupServiceTrackerTrackers(ctx);
        bundleContext_cleanupServiceRegistration(ctx);
}

celix_status_t bundleContext_getBundle(bundle_context_pt context, bundle_pt *out) {
    if (context == NULL || out == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *out = celix_bundleContext_getBundle(context);
    return CELIX_SUCCESS;
}

celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework) {
    if (context == NULL || framework == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *framework = context->framework;
    return CELIX_SUCCESS;
}

celix_status_t bundleContext_installBundle(bundle_context_pt context, const char * location, bundle_pt *bundle) {
    return bundleContext_installBundle2(context, location, NULL, bundle);
}

celix_status_t bundleContext_installBundle2(bundle_context_pt context,
                                            const char* location,
                                            const char* inputFile __attribute__((unused)),
                                            bundle_pt* bundle) {
    celix_status_t status = CELIX_SUCCESS;
    long id = -1L;
    if (context == NULL || location == NULL || bundle == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    status = celix_framework_installBundleInternal(context->framework, location, &id);
    if (status == CELIX_SUCCESS) {
        *bundle = framework_getBundleById(context->framework, id);
    }
    return status;
}

celix_status_t bundleContext_registerService(bundle_context_pt context, const char * serviceName, const void * svcObj,
         celix_properties_t* properties, service_registration_pt *service_registration) {
    if (context == NULL || service_registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    long bndId = celix_bundle_getId(context->bundle);
    return fw_registerService(context->framework, service_registration, bndId, serviceName, svcObj, properties);
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, const char * serviceName, service_factory_pt factory,
         celix_properties_t* properties, service_registration_pt *service_registration) {
    if (context == NULL || service_registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    long bndId = celix_bundle_getId(context->bundle);
    return fw_registerServiceFactory(context->framework, service_registration, bndId, serviceName, factory, properties);
}

celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, const char * filter, celix_array_list_t** service_references) {
    if (context == NULL || service_references == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return fw_getServiceReferences(context->framework, service_references, context->bundle, serviceName, filter);
}

celix_status_t bundleContext_getServiceReference(bundle_context_pt context, const char * serviceName, service_reference_pt *service_reference) {
    service_reference_pt reference = NULL;
    celix_array_list_t* services = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (serviceName != NULL) {
        if (bundleContext_getServiceReferences(context, serviceName, NULL, &services) == CELIX_SUCCESS) {
            unsigned int size = celix_arrayList_size(services);
            for(unsigned int i = 0; i < size; i++) {
                if(i == 0) {
                    reference = celix_arrayList_get(services, 0);
                } else {
                    bundleContext_ungetServiceReference(context, celix_arrayList_get(services, i));
                }
            }
            celix_arrayList_destroy(services);
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

static bool bundleContext_IsServiceReferenceValid(bundle_context_pt context, service_reference_pt ref) {
    bundle_pt refBundle = NULL;
    serviceReference_getOwner(ref, &refBundle);
    return context->bundle == refBundle;
}

celix_status_t bundleContext_retainServiceReference(bundle_context_pt context, service_reference_pt ref) {
    if (context == NULL || ref == NULL || !bundleContext_IsServiceReferenceValid(context, ref)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return serviceReference_retain(ref);
}

celix_status_t bundleContext_ungetServiceReference(bundle_context_pt context, service_reference_pt reference) {
    if (context == NULL || reference == NULL || !bundleContext_IsServiceReferenceValid(context, reference)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return serviceReference_release(reference, NULL);
}

celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void** service_instance) {
    if (context == NULL || reference == NULL || service_instance == NULL || !bundleContext_IsServiceReferenceValid(context, reference)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    /*NOTE argument service_instance should be considered a 'const void**'.
    To ensure backwards compatibility a cast is made instead.
    */
    return serviceReference_getService(reference, (const void**) service_instance);
}

celix_status_t bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result) {
    if (context == NULL || reference == NULL || !bundleContext_IsServiceReferenceValid(context, reference)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return serviceReference_ungetService(reference, result);
}

celix_status_t bundleContext_getBundles(bundle_context_pt context, celix_array_list_t** bundles) {
    if (context == NULL || bundles == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *bundles = framework_getBundles(context->framework);
    return CELIX_SUCCESS;
}

celix_status_t bundleContext_getBundleById(bundle_context_pt context, long id, bundle_pt *bundle) {
    if (context == NULL || bundle == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_status_t status = CELIX_SUCCESS;
    *bundle = framework_getBundleById(context->framework, id);
    if (*bundle == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
    }
    framework_logIfError(context->framework->logger, status, NULL, "Failed to get bundle [id=%ld]", id);
    return status;
}

celix_status_t bundleContext_addServiceListener(bundle_context_pt context, celix_service_listener_t *listener, const char* filter) {
    if (context == NULL || listener == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    fw_addServiceListener(context->framework, context->bundle, listener, filter);
    return CELIX_SUCCESS;
}

celix_status_t bundleContext_removeServiceListener(bundle_context_pt context, celix_service_listener_t *listener) {
    if (context == NULL || listener == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    fw_removeServiceListener(context->framework, context->bundle, listener);
    return CELIX_SUCCESS;
}

celix_status_t bundleContext_addBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    if (context == NULL || listener == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return fw_addBundleListener(context->framework, context->bundle, listener);
}

celix_status_t bundleContext_removeBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    if (context == NULL || listener == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return fw_removeBundleListener(context->framework, context->bundle, listener);
}

celix_status_t bundleContext_addFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    if (context == NULL || listener == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return fw_addFrameworkListener(context->framework, context->bundle, listener);
}

celix_status_t bundleContext_removeFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    if (context == NULL || listener == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return fw_removeFrameworkListener(context->framework, context->bundle, listener);
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
    return celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
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
    return celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
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
    celix_autoptr(celix_properties_t) props = opts->properties;
    if (props == NULL) {
        props = celix_properties_create();
    }

    if (opts->serviceVersion != NULL && strncmp("", opts->serviceVersion, 1) != 0) {
        celix_autoptr(celix_version_t) version = celix_version_createVersionFromString(opts->serviceVersion);
        if (!version) {
            celix_framework_logTssErrors(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR);
            fw_log(
                ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot parse service version %s", opts->serviceVersion);
            return -1;
        }
        celix_status_t rc =
            celix_properties_assignVersion(props, CELIX_FRAMEWORK_SERVICE_VERSION, celix_steal_ptr(version));
        if (rc != CELIX_SUCCESS) {
            celix_framework_logTssErrors(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR);
            fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot set service version %s", opts->serviceVersion);
            return -1;
        }
    }

    long svcId;
    if (!async && celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        /*
         * Note already on event loop, cannot register the service async, because we cannot wait a future event (the
         * service registration) the event loop.
         *
         * So in this case we handle the service registration the "traditional way" and call the sync fw service
         * registrations versions on the event loop thread
         */

        svcId = celix_framework_registerService(ctx->framework, ctx->bundle, opts->serviceName, opts->svc, opts->factory, celix_steal_ptr(props));
    } else {
        void (*asyncCallback)(void *data, long serviceId) = async ? opts->asyncCallback : NULL; //NOTE for not async call do not use the callback.
        svcId = celix_framework_registerServiceAsync(ctx->framework, ctx->bundle, opts->serviceName, opts->svc, opts->factory, celix_steal_ptr(props), opts->asyncData, asyncCallback, NULL, NULL);
        if (!async && svcId >= 0) {
            //note on event loop thread, but in a sync call, so waiting till service registration is concluded
            celix_bundleContext_waitForAsyncRegistration(ctx, svcId);
        }
    }

    if (svcId >= 0) {
        celixThreadMutex_lock(&ctx->mutex);
        celix_arrayList_addLong(ctx->svcRegistrations, svcId);
        celixThreadMutex_unlock(&ctx->mutex);
    }
    return svcId;
}

long celix_bundleContext_registerServiceWithOptions(bundle_context_t *ctx, const celix_service_registration_options_t *opts) {
    return celix_bundleContext_registerServiceWithOptionsInternal(ctx, opts, false);
}

long celix_bundleContext_registerServiceWithOptionsAsync(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts) {
    return celix_bundleContext_registerServiceWithOptionsInternal(ctx, opts, true);
}

void celix_bundleContext_waitForAsyncRegistration(celix_bundle_context_t* ctx, long serviceId) {
    if (serviceId >= 0) {
        celix_framework_waitForAsyncRegistration(ctx->framework, serviceId);
    }
}

bool celix_bundleContext_isServiceRegistered(celix_bundle_context_t* ctx, long serviceId) {
    return celix_serviceRegistry_isServiceRegistered(ctx->framework->registry, serviceId);
}

static void celix_bundleContext_unregisterServiceInternal(celix_bundle_context_t *ctx, long serviceId, bool async, void *data, void (*done)(void*)) {
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
                celix_framework_unregisterAsync(ctx->framework, ctx->bundle, found, data, done);
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
                if (done != NULL) {
                    done(data);
                }
            } else {
                celix_framework_unregisterAsync(ctx->framework, ctx->bundle, found, data, done);
                celix_bundleContext_waitForAsyncUnregistration(ctx, serviceId);
            }
        } else {
            framework_logIfError(ctx->framework->logger, CELIX_ILLEGAL_ARGUMENT, NULL,
                                 "No service registered with svc id %li for bundle %s (bundle id: %li)!", serviceId,
                                 celix_bundle_getSymbolicName(ctx->bundle), celix_bundle_getId(ctx->bundle));
        }
    }
}

void celix_bundleContext_unregisterServiceAsync(celix_bundle_context_t *ctx, long serviceId, void *data, void (*done)(void*)) {
    return celix_bundleContext_unregisterServiceInternal(ctx, serviceId, true, data, done);
}

void celix_bundleContext_unregisterService(bundle_context_t *ctx, long serviceId) {
    return celix_bundleContext_unregisterServiceInternal(ctx, serviceId, false, NULL, NULL);
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

static celix_status_t bundleContext_bundleChanged(void* listenerSvc, bundle_event_t* event) {
    bundle_listener_t* listener = listenerSvc;
    celix_bundle_context_bundle_tracker_entry_t* tracker = listener->handle;

    bool handleEvent = true;
    long bndId = celix_bundle_getId(event->bnd);
    if (bndId == 0 /*framework bundle*/) {
        handleEvent = tracker->opts.includeFrameworkBundle;
    }

    if (handleEvent) {
        void* callbackHandle = tracker->opts.callbackHandle;

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
    return CELIX_SUCCESS;
}

void celix_bundleContext_trackBundlesWithOptionsCallback(void *data) {
    celix_bundle_context_bundle_tracker_entry_t* entry = data;
    assert(celix_framework_isCurrentThreadTheEventLoop(entry->ctx->framework));
    celixThreadMutex_lock(&entry->ctx->mutex);
    bool cancelled = entry->cancelled;
    entry->created = true;
    celixThreadMutex_unlock(&entry->ctx->mutex);
    if (cancelled) {
        fw_log(entry->ctx->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Creation of bundle tracker cancelled. trk id = %li", entry->trackerId);
        free(entry);
    } else {
        fw_addBundleListener(entry->ctx->framework, entry->ctx->bundle, &entry->listener);
        if (entry->opts.trackerCreatedCallback) {
            entry->opts.trackerCreatedCallback(entry->opts.trackerCreatedCallbackData);
        }
    }

}

static long celix_bundleContext_trackBundlesWithOptionsInternal(
        bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts,
        bool async) {
    celix_bundle_context_bundle_tracker_entry_t *entry = calloc(1, sizeof(*entry));
    memcpy(&entry->opts, opts, sizeof(*opts));
    entry->ctx = ctx;
    entry->createEventId = celix_framework_nextEventId(ctx->framework);
    entry->listener.handle = entry;
    entry->listener.bundleChanged = bundleContext_bundleChanged;

    celixThreadMutex_lock(&ctx->mutex);
    entry->trackerId = ctx->nextTrackerId++;
    hashMap_put(ctx->bundleTrackers, (void*)(entry->trackerId), entry);
    long trackerId = entry->trackerId;
    celixThreadMutex_unlock(&ctx->mutex);

    if (!async) { //note only using the async callback if this is a async call.
        entry->opts.trackerCreatedCallback = NULL;
    }
    long id = celix_framework_fireGenericEvent(
            ctx->framework,
            entry->createEventId,
            celix_bundle_getId(ctx->bundle),
            "add bundle listener",
            entry,
            celix_bundleContext_trackBundlesWithOptionsCallback,
            NULL,
            NULL);

    if (!async) {
        celix_framework_waitForGenericEvent(ctx->framework, id);
    }

    return trackerId;
}

long celix_bundleContext_trackBundlesWithOptions(
        bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts) {
    return celix_bundleContext_trackBundlesWithOptionsInternal(ctx, opts, false);
}

long celix_bundleContext_trackBundlesWithOptionsAsync(
        celix_bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts) {
    return celix_bundleContext_trackBundlesWithOptionsInternal(ctx, opts, true);
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

long celix_bundleContext_trackBundlesAsync(
        celix_bundle_context_t* ctx,
        void* callbackHandle,
        void (*onStarted)(void* handle, const celix_bundle_t *bundle),
        void (*onStopped)(void *handle, const celix_bundle_t *bundle)) {
    celix_bundle_tracking_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.callbackHandle = callbackHandle;
    opts.onStarted = onStarted;
    opts.onStopped = onStopped;
    return celix_bundleContext_trackBundlesWithOptionsAsync(ctx, &opts);
}

size_t celix_bundleContext_useBundles(
        bundle_context_t *ctx,
        void *callbackHandle,
        void (*use)(void *handle, const bundle_t *bundle)) {
    return celix_framework_useBundles(ctx->framework, false, callbackHandle, use);
}

bool celix_bundleContext_useBundle(
        bundle_context_t *ctx,
        long bundleId,
        void *callbackHandle,
        void (*use)(void *handle, const bundle_t *bundle)) {
    return celix_framework_useBundle(ctx->framework, false, bundleId, callbackHandle, use);
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
        celix_bundle_context_service_tracker_entry_t* entry = hashMap_get(ctx->serviceTrackers, (void*)trkId);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Dangling service tracker with trkId %li, for bundle %s and with filter %s. Add missing 'celix_bundleContext_stopTracker' calls.", trkId, symbolicName, entry->tracker->filter);
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
        long svcId = celix_arrayList_getLong(ctx->svcRegistrations, i);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Dangling service registration with svcId %li, for bundle %s. Add missing 'celix_bundleContext_unregisterService' calls.", svcId, symbolicName);
        if (danglingSvcIds == NULL) {
            danglingSvcIds = celix_arrayList_create();
        }
        celix_arrayList_addLong(danglingSvcIds, svcId);
    }
    celixThreadMutex_unlock(&ctx->mutex);

    if (danglingSvcIds != NULL) {
        for (int i = 0; i < celix_arrayList_size(danglingSvcIds); ++i) {
            long svcId = celix_arrayList_getLong(danglingSvcIds, i);
            celix_framework_unregister(ctx->framework, ctx->bundle, svcId);
        }
        celix_arrayList_destroy(danglingSvcIds);
    }
}

static void celix_bundleContext_removeBundleTracker(void *data) {
    celix_bundle_context_bundle_tracker_entry_t *tracker = data;
    fw_removeBundleListener(tracker->ctx->framework, tracker->ctx->bundle, &tracker->listener);
    celixThreadMutex_lock(&tracker->ctx->mutex);
    hashMap_remove(tracker->ctx->stoppingTrackerEventIds, (void*)tracker->trackerId);
    celixThreadMutex_unlock(&tracker->ctx->mutex);
    free(tracker);
}

static void celix_bundleContext_removeServiceTracker(void *data) {
    celix_bundle_context_service_tracker_entry_t *tracker = data;
    celix_serviceTracker_destroy(tracker->tracker);
    celixThreadMutex_lock(&tracker->ctx->mutex);
    hashMap_remove(tracker->ctx->stoppingTrackerEventIds, (void*)tracker->trackerId);
    celixThreadMutex_unlock(&tracker->ctx->mutex);
    if (tracker->isFreeFilterNeeded) {
        free((char*)tracker->opts.filter.serviceName);
        free((char*)tracker->opts.filter.versionRange);
        free((char*)tracker->opts.filter.filter);
    }
    free(tracker);
}

static void celix_bundleContext_removeServiceTrackerTracker(void *data) {
    celix_bundle_context_service_tracker_tracker_entry_t *tracker = data;
    celix_framework_unregister(tracker->ctx->framework, tracker->ctx->bundle, tracker->serviceId);
    celixThreadMutex_lock(&tracker->ctx->mutex);
    hashMap_remove(tracker->ctx->stoppingTrackerEventIds, (void*)tracker->trackerId);
    celixThreadMutex_unlock(&tracker->ctx->mutex);
    free(tracker->serviceName);
    free(tracker);
}

static void celix_bundleContext_stopTrackerInternal(bundle_context_t *ctx, long trackerId, bool async, void *doneData, void (*doneCallback)(void* doneData)) {
    if (ctx == NULL || trackerId <= 0) {
        return;
    }

    bool found = false;
    bool cancelled = false;
    celix_bundle_context_bundle_tracker_entry_t *bundleTracker = NULL;
    celix_bundle_context_service_tracker_entry_t *serviceTracker = NULL;
    celix_bundle_context_service_tracker_tracker_entry_t *svcTrackerTracker = NULL;

    celixThreadMutex_lock(&ctx->mutex);

    if (hashMap_containsKey(ctx->bundleTrackers, (void *) trackerId)) {
        found = true;
        bundleTracker = hashMap_remove(ctx->bundleTrackers, (void *) trackerId);
        if (!bundleTracker->created && !async) {
            //note tracker not yet created, so cancel instead of removing
            bundleTracker->cancelled = true;
            cancelled = true;
        }
    } else if (hashMap_containsKey(ctx->serviceTrackers, (void *) trackerId)) {
        found = true;
        serviceTracker = hashMap_remove(ctx->serviceTrackers, (void *) trackerId);
        if (serviceTracker->tracker == NULL && !async) {
            //note tracker not yet created, so cancel instead of removing
            serviceTracker->cancelled = true;
            cancelled = true;
        }
    } else if (hashMap_containsKey(ctx->metaTrackers, (void *) trackerId)) {
        found = true;
        svcTrackerTracker = hashMap_remove(ctx->metaTrackers, (void *) trackerId);
        //note because a meta tracker is a service listener hook under waiter, no additional cancel is needed (svc reg will be cancelled)
    }

    if (found && cancelled) {
        //nop
        celixThreadMutex_unlock(&ctx->mutex);
    } else if (found && !async && celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        //already on the event loop, stop tracker "traditionally" to keep old behavior
        celixThreadMutex_unlock(&ctx->mutex); //note calling remove/stops/unregister out side of locks

        if (bundleTracker != NULL) {
            fw_removeBundleListener(ctx->framework, ctx->bundle, &bundleTracker->listener);
            free(bundleTracker);
        } else if (serviceTracker != NULL) {
            celix_serviceTracker_destroy(serviceTracker->tracker);
            if (serviceTracker->isFreeFilterNeeded) {
                free((char*)serviceTracker->opts.filter.serviceName);
                free((char*)serviceTracker->opts.filter.versionRange);
                free((char*)serviceTracker->opts.filter.filter);
            }
            free(serviceTracker);
        } else if (svcTrackerTracker != NULL) {
            celix_framework_unregister(ctx->framework, ctx->bundle, svcTrackerTracker->serviceId);
            free(svcTrackerTracker->serviceName);
            free(svcTrackerTracker);
        }

        if (doneCallback != NULL) {
            doneCallback(doneData);
        }
    } else if (found && async) {
        //NOTE: for async stopping of tracking we need to ensure we cant wait for the tracker destroy id event.
        long eventId = celix_framework_nextEventId(ctx->framework);
        hashMap_put(ctx->stoppingTrackerEventIds, (void*)trackerId, (void*)eventId);

        if (bundleTracker != NULL) {
            celix_framework_fireGenericEvent(ctx->framework, eventId, celix_bundle_getId(ctx->bundle), "stop tracker", bundleTracker, celix_bundleContext_removeBundleTracker, doneData, doneCallback);
        } else if (serviceTracker != NULL) {
            celix_framework_fireGenericEvent(ctx->framework, eventId, celix_bundle_getId(ctx->bundle), "stop tracker", serviceTracker, celix_bundleContext_removeServiceTracker, doneData, doneCallback);
        } else if (svcTrackerTracker != NULL) {
            celix_framework_fireGenericEvent(ctx->framework, eventId, celix_bundle_getId(ctx->bundle), "stop meta tracker", svcTrackerTracker, celix_bundleContext_removeServiceTrackerTracker, doneData, doneCallback);
        } else {
            fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Unexpected else branch");
        }
        celixThreadMutex_unlock(&ctx->mutex);
    } else if (found) { /*sync, so waiting for events*/
        long eventId = -1L;
        if (bundleTracker != NULL) {
            eventId = celix_framework_fireGenericEvent(ctx->framework, -1L, celix_bundle_getId(ctx->bundle), "stop tracker", bundleTracker, celix_bundleContext_removeBundleTracker, doneData, doneCallback);
        } else if (serviceTracker != NULL) {
            eventId = celix_framework_fireGenericEvent(ctx->framework, -1L, celix_bundle_getId(ctx->bundle), "stop tracker", serviceTracker, celix_bundleContext_removeServiceTracker, doneData, doneCallback);
        } else if (svcTrackerTracker != NULL) {
            eventId = celix_framework_fireGenericEvent(ctx->framework, -1L, celix_bundle_getId(ctx->bundle), "stop meta tracker", svcTrackerTracker, celix_bundleContext_removeServiceTrackerTracker, doneData, doneCallback);
        } else {
            fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Unexpected else branch");
        }
        celixThreadMutex_unlock(&ctx->mutex);
        celix_framework_waitForGenericEvent(ctx->framework, eventId);
    } else {
        celixThreadMutex_unlock(&ctx->mutex);
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "No tracker with id %li found'", trackerId);
    }
}

void celix_bundleContext_stopTracker(bundle_context_t *ctx, long trackerId) {
    return celix_bundleContext_stopTrackerInternal(ctx, trackerId, false, NULL, NULL);
}

void celix_bundleContext_stopTrackerAsync(celix_bundle_context_t *ctx, long trackerId, void *doneData, void (*doneCallback)(void* doneData)) {
    return celix_bundleContext_stopTrackerInternal(ctx, trackerId, true, doneData, doneCallback);
}

static void celix_bundleContext_waitForTrackerInternal(celix_bundle_context_t* ctx, long trackerId, bool waitForStart) {
    if (ctx == NULL || trackerId < 0) {
        //silent ignore
        return;
    }


    bool found = false;
    long eventId = -1;
    long svcId = -1;

    if (waitForStart) {
        celixThreadMutex_lock(&ctx->mutex);
        if (hashMap_containsKey(ctx->bundleTrackers, (void *) trackerId)) {
            found = true;
            celix_bundle_context_bundle_tracker_entry_t* bundleTracker = hashMap_get(ctx->bundleTrackers, (void *) trackerId);
            eventId = bundleTracker->createEventId;
        } else if (hashMap_containsKey(ctx->serviceTrackers, (void *) trackerId)) {
            found = true;
            celix_bundle_context_service_tracker_entry_t* serviceTracker = hashMap_get(ctx->serviceTrackers, (void *) trackerId);
            eventId = serviceTracker->createEventId;
        } else if (hashMap_containsKey(ctx->metaTrackers, (void *) trackerId)) {
            found = true;
            celix_bundle_context_service_tracker_tracker_entry_t* svcTrackerTracker = hashMap_get(ctx->metaTrackers, (void *) trackerId);
            svcId = svcTrackerTracker->serviceId;
        }
        celixThreadMutex_unlock(&ctx->mutex);
    } else {
        celixThreadMutex_lock(&ctx->mutex);
        if (hashMap_containsKey(ctx->stoppingTrackerEventIds, (void*)trackerId)) {
            found = true;
            eventId = (long)hashMap_get(ctx->stoppingTrackerEventIds, (void*)trackerId);
        }
        celixThreadMutex_unlock(&ctx->mutex);
    }

    if (found) {
        if (svcId >= 0) {
            celix_framework_waitForAsyncRegistration(ctx->framework, svcId);
        } else {
            celix_framework_waitForGenericEvent(ctx->framework, eventId);
        }
    }
}


void celix_bundleContext_waitForAsyncTracker(celix_bundle_context_t* ctx, long trackerId) {
    return celix_bundleContext_waitForTrackerInternal(ctx, trackerId, true);
}

void celix_bundleContext_waitForAsyncStopTracker(celix_bundle_context_t* ctx, long trackerId) {
    return celix_bundleContext_waitForTrackerInternal(ctx, trackerId, false);
}

long celix_bundleContext_installBundle(bundle_context_t *ctx, const char *bundleLoc, bool autoStart) {
    return celix_framework_installBundle(ctx->framework, bundleLoc, autoStart);
}

celix_array_list_t* celix_bundleContext_listBundles(celix_bundle_context_t *ctx) {
    return celix_framework_listBundles(ctx->framework);
}

celix_array_list_t* celix_bundleContext_listInstalledBundles(celix_bundle_context_t *ctx) {
    return celix_framework_listInstalledBundles(ctx->framework);
}

bool celix_bundleContext_isBundleInstalled(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_isBundleInstalled(ctx->framework, bndId);
}

bool celix_bundleContext_isBundleActive(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_isBundleActive(ctx->framework, bndId);
}

bool celix_bundleContext_startBundle(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_startBundle(ctx->framework, bndId);
}

bool celix_bundleContext_updateBundle(celix_bundle_context_t *ctx, long bndId, const char* updatedBundleUrl) {
    return celix_framework_updateBundle(ctx->framework, bndId, updatedBundleUrl);
}

bool celix_bundleContext_stopBundle(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_stopBundle(ctx->framework, bndId);
}

bool celix_bundleContext_uninstallBundle(bundle_context_t *ctx, long bndId) {
    return celix_framework_uninstallBundle(ctx->framework, bndId);
}

bool celix_bundleContext_unloadBundle(celix_bundle_context_t *ctx, long bndId) {
    return celix_framework_unloadBundle(ctx->framework, bndId);
}

bool celix_bundleContext_useServiceWithId(
        bundle_context_t *ctx,
        long serviceId,
        const char *serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)) {
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;

    char filter[64];
    snprintf(filter, 64, "(%s=%li)", CELIX_FRAMEWORK_SERVICE_ID, serviceId);

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

typedef struct celix_bundle_context_use_service_data {
    celix_bundle_context_t* ctx;
    const celix_service_use_options_t* opts;

    bool called; //for use service
    size_t count; //for use services
    celix_service_tracker_t * svcTracker;
} celix_bundle_context_use_service_data_t;

static void celix_bundleContext_useServiceWithOptions_1_CreateServiceTracker(void *data) {
    celix_bundle_context_use_service_data_t* d = data;
    assert(celix_framework_isCurrentThreadTheEventLoop(d->ctx->framework));

    celix_service_tracking_options_t trkOpts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    trkOpts.filter = d->opts->filter;

    d->called = false;
    d->count = 0;
    d->svcTracker = celix_serviceTracker_createWithOptions(d->ctx, &trkOpts);
}

static void celix_bundleContext_useServiceWithOptions_2_UseServiceTracker(void *data) {
    celix_bundle_context_use_service_data_t* d = data;
    assert(celix_framework_isCurrentThreadTheEventLoop(d->ctx->framework));

    d->called = celix_serviceTracker_useHighestRankingService(d->svcTracker, d->opts->filter.serviceName, 0, d->opts->callbackHandle, d->opts->use, d->opts->useWithProperties, d->opts->useWithOwner);
}

bool celix_bundleContext_useServiceWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts) {
    if (opts == NULL || opts->filter.serviceName == NULL) {
        return false;
    }

    celix_bundle_context_use_service_data_t data = {0};
    data.ctx = ctx;
    data.opts = opts;
    bool called = false;

    if (celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        // Ignore timeout: blocking the event loop prevents any progress to be made
        celix_bundleContext_useServiceWithOptions_1_CreateServiceTracker(&data);
        celix_bundleContext_useServiceWithOptions_2_UseServiceTracker(&data);
        celix_serviceTracker_destroy(data.svcTracker);
        return data.called;
    }

    long eventId = celix_framework_fireGenericEvent(ctx->framework, -1, celix_bundle_getId(ctx->bundle), "create service tracker for celix_bundleContext_useServiceWithOptions", &data, celix_bundleContext_useServiceWithOptions_1_CreateServiceTracker, NULL, NULL);
    celix_framework_waitForGenericEvent(ctx->framework, eventId);


    if(opts->flags & CELIX_SERVICE_USE_DIRECT) {
        if(opts->flags & CELIX_SERVICE_USE_SOD) {
            // check CelixBundleContextServicesTestSuite.UseServiceOnDemandDirectlyWithAsyncRegisterTest to see what is "service on demand".
            celix_framework_waitUntilNoPendingRegistration(ctx->framework);
        }
        called = celix_serviceTracker_useHighestRankingService(data.svcTracker, NULL, opts->waitTimeoutInSeconds, opts->callbackHandle, opts->use, opts->useWithProperties, opts->useWithOwner);
    } else {
        struct timespec startTime = celix_gettime(CLOCK_MONOTONIC);
        bool useServiceIsDone = false;
        do {
            eventId = celix_framework_fireGenericEvent(ctx->framework, -1, celix_bundle_getId(ctx->bundle), "use service tracker for celix_bundleContext_useServiceWithOptions", &data, celix_bundleContext_useServiceWithOptions_2_UseServiceTracker, NULL, NULL);
            celix_framework_waitForGenericEvent(ctx->framework, eventId);

            bool timeoutNotUsed = opts->waitTimeoutInSeconds == 0;
            bool timeoutExpired = celix_elapsedtime(CLOCK_MONOTONIC, startTime) > opts->waitTimeoutInSeconds;

            called = data.called;

            useServiceIsDone = timeoutNotUsed || timeoutExpired || called;
            if (!useServiceIsDone) {
                usleep(10);
            }
        } while (!useServiceIsDone);
    }

    eventId = celix_framework_fireGenericEvent(ctx->framework, -1, celix_bundle_getId(ctx->bundle), "close service tracker for celix_bundleContext_useServiceWithOptions", data.svcTracker, (void *)celix_serviceTracker_destroy, NULL, NULL);
    celix_framework_waitForGenericEvent(ctx->framework, eventId);

    return called;
}

static void celix_bundleContext_useServicesWithOptions_2_UseServiceTracker(void *data) {
    celix_bundle_context_use_service_data_t* d = data;
    d->count = celix_serviceTracker_useServices(d->svcTracker, d->opts->filter.serviceName, d->opts->callbackHandle, d->opts->use, d->opts->useWithProperties, d->opts->useWithOwner);
}

size_t celix_bundleContext_useServicesWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts) {
    if (opts == NULL || opts->filter.serviceName == NULL) {
        return 0;
    }

    celix_bundle_context_use_service_data_t data = {0};
    data.ctx = ctx;
    data.opts = opts;

    if (celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        celix_bundleContext_useServiceWithOptions_1_CreateServiceTracker(&data);
        celix_bundleContext_useServicesWithOptions_2_UseServiceTracker(&data);
        celix_serviceTracker_destroy(data.svcTracker);
        return data.count;
    }

    long eventId = celix_framework_fireGenericEvent(ctx->framework, -1, celix_bundle_getId(ctx->bundle), "create service tracker for celix_bundleContext_useServicesWithOptions", &data, celix_bundleContext_useServiceWithOptions_1_CreateServiceTracker, NULL, NULL);
    celix_framework_waitForGenericEvent(ctx->framework, eventId);

    if (opts->flags & CELIX_SERVICE_USE_DIRECT) {
        if(opts->flags & CELIX_SERVICE_USE_SOD) {
            // check CelixBundleContextServicesTestSuite.UseServicesOnDemandDirectlyWithAsyncRegisterTest to see what is "service on demand".
            celix_framework_waitUntilNoPendingRegistration(ctx->framework);
        }
        celix_bundleContext_useServicesWithOptions_2_UseServiceTracker(&data);
    } else {
        eventId = celix_framework_fireGenericEvent(ctx->framework, -1, celix_bundle_getId(ctx->bundle), "use service tracker for celix_bundleContext_useServicesWithOptions", &data, celix_bundleContext_useServicesWithOptions_2_UseServiceTracker, NULL, NULL);
        celix_framework_waitForGenericEvent(ctx->framework, eventId);
    }

    eventId = celix_framework_fireGenericEvent(ctx->framework, -1, celix_bundle_getId(ctx->bundle), "use service tracker for celix_bundleContext_useServicesWithOptions", data.svcTracker, (void *)celix_serviceTracker_destroy, NULL, NULL);
    celix_framework_waitForGenericEvent(ctx->framework, eventId);

    return data.count;
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
    return celix_bundleContext_trackServicesWithOptionsInternal(ctx, &opts, false);
}

long celix_bundleContext_trackServiceAsync(
        bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*set)(void* handle, void* svc)) {
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.set = set;
    return celix_bundleContext_trackServicesWithOptionsInternal(ctx, &opts, true);
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
    return celix_bundleContext_trackServicesWithOptionsInternal(ctx, &opts, false);
}

long celix_bundleContext_trackServicesAsync(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*add)(void* handle, void* svc),
        void (*remove)(void* handle, void* svc)) {
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = serviceName;
    opts.callbackHandle = callbackHandle;
    opts.add = add;
    opts.remove = remove;
    return celix_bundleContext_trackServicesWithOptionsInternal(ctx, &opts, true);
}


static void celix_bundleContext_createTrackerOnEventLoop(void *data) {
    celix_bundle_context_service_tracker_entry_t* entry = data;
    assert(celix_framework_isCurrentThreadTheEventLoop(entry->ctx->framework));
    celixThreadMutex_lock(&entry->ctx->mutex);
    bool cancelled = entry->cancelled;
    if (cancelled) {
        fw_log(entry->ctx->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Creating of service tracker was cancelled. trk id = %li, svc name tracked = %s", entry->trackerId, entry->opts.filter.serviceName);
        celixThreadMutex_unlock(&entry->ctx->mutex);
        return;
    }
    celix_service_tracker_t *tracker = celix_serviceTracker_createClosedWithOptions(entry->ctx, &entry->opts);
    if (tracker == NULL) {
        fw_log(entry->ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot create tracker for bnd %s (%li)", celix_bundle_getSymbolicName(entry->ctx->bundle), celix_bundle_getId(entry->ctx->bundle));
    } else {
        entry->tracker = tracker;
    }
    celixThreadMutex_unlock(&entry->ctx->mutex);
    if (tracker) {
        serviceTracker_open(tracker);
    }
}

static void celix_bundleContext_doneCreatingTrackerOnEventLoop(void *data) {
    celix_bundle_context_service_tracker_entry_t* entry = data;
    celixThreadMutex_lock(&entry->ctx->mutex);
    bool cancelled = entry->cancelled;
    celixThreadMutex_unlock(&entry->ctx->mutex);
    if (cancelled) {
        //tracker creation cancelled -> entry already removed from map, but memory needs to be freed.
        if (entry->isFreeFilterNeeded) {
            free((char*)entry->opts.filter.serviceName);
            free((char*)entry->opts.filter.versionRange);
            free((char*)entry->opts.filter.filter);
        }
        free(entry);
    } else if (entry->trackerCreatedCallback != NULL) {
        entry->trackerCreatedCallback(entry->trackerCreatedCallbackData);
    }
}



static long celix_bundleContext_trackServicesWithOptionsInternal(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts, bool async) {
    if (ctx == NULL) {
        return -1L;
    } else if (opts == NULL) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot track services with a NULL service tracking options argument");
        return -1L;
    }

    if (opts->filter.serviceName == NULL) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Starting a tracker for any services");
    }

    if (!async && celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        //already in event loop thread. To keep the old behavior just create the tracker traditionally (chained in the current thread).
        celix_service_tracker_t *tracker = celix_serviceTracker_createWithOptions(ctx, opts);
        long trackerId = -1L;
        if (tracker != NULL) {
            celix_bundle_context_service_tracker_entry_t* entry = calloc(1, sizeof(*entry));
            entry->ctx = ctx;
            entry->tracker = tracker;
            entry->opts = *opts;
            entry->isFreeFilterNeeded = false;
            entry->createEventId = -1;
            celixThreadMutex_lock(&ctx->mutex);
            entry->trackerId = ctx->nextTrackerId++;
            trackerId = entry->trackerId;
            hashMap_put(ctx->serviceTrackers, (void *) trackerId, entry);
            celixThreadMutex_unlock(&ctx->mutex);
        }
        return trackerId;
    } else {
        celix_bundle_context_service_tracker_entry_t* entry = calloc(1, sizeof(*entry));
        entry->ctx = ctx;
        entry->createEventId = celix_framework_nextEventId(ctx->framework);
        entry->tracker = NULL; //will be set async
        entry->opts = *opts;

        if (async) { //note only setting the async callback if this is a async call
            entry->trackerCreatedCallbackData = opts->trackerCreatedCallbackData;
            entry->trackerCreatedCallback = opts->trackerCreatedCallback;

            //for async copying the const char* inputs
            entry->isFreeFilterNeeded = true;
            entry->opts.filter.serviceName = celix_utils_strdup(opts->filter.serviceName);
            entry->opts.filter.versionRange = celix_utils_strdup(opts->filter.versionRange);
            entry->opts.filter.filter = celix_utils_strdup(opts->filter.filter);
        }
        celixThreadMutex_lock(&ctx->mutex);
        entry->trackerId = ctx->nextTrackerId++;
        long trackerId = entry->trackerId;
        hashMap_put(ctx->serviceTrackers, (void *)entry->trackerId, entry);
        celixThreadMutex_unlock(&ctx->mutex);

        long id = celix_framework_fireGenericEvent(ctx->framework, entry->createEventId, celix_bundle_getId(ctx->bundle), "create service tracker event", entry, celix_bundleContext_createTrackerOnEventLoop, entry, celix_bundleContext_doneCreatingTrackerOnEventLoop);

        if (!async) {
            celix_framework_waitForGenericEvent(ctx->framework, id);
        }

        return trackerId;
    }
}

long celix_bundleContext_trackServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts) {
    return celix_bundleContext_trackServicesWithOptionsInternal(ctx, opts, false);
}

long celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts) {
    return celix_bundleContext_trackServicesWithOptionsInternal(ctx, opts, true);
}

long celix_bundleContext_findService(celix_bundle_context_t *ctx, const char *serviceName) {
    celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
    opts.serviceName = serviceName;
    return celix_bundleContext_findServiceWithOptions(ctx, &opts);
}

long celix_bundleContext_findServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts) {
    long result = -1L;
    char* filter = celix_serviceRegistry_createFilterFor(ctx->framework->registry, opts->serviceName, opts->versionRange, opts->filter);
    if (filter != NULL) {
        celix_array_list_t *svcIds = celix_serviceRegisrty_findServices(ctx->framework->registry, filter);
        if (svcIds != NULL && celix_arrayList_size(svcIds) > 0) {
            result = celix_arrayList_getLong(svcIds, 0);
        }
        if (svcIds != NULL) {
            celix_arrayList_destroy(svcIds);
        }
        free(filter);
    }
    return result;
}


celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char *serviceName) {
    celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
    opts.serviceName = serviceName;
    return celix_bundleContext_findServicesWithOptions(ctx, &opts);
}

celix_array_list_t* celix_bundleContext_findServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts) {
    celix_array_list_t* result = NULL;
    char* filter = celix_serviceRegistry_createFilterFor(ctx->framework->registry, opts->serviceName, opts->versionRange, opts->filter);
    if (filter != NULL) {
        result = celix_serviceRegisrty_findServices(ctx->framework->registry, filter);
        free(filter);
    }
    return result;
}

static celix_status_t bundleContext_callServicedTrackerTrackerCallback(void *handle, celix_array_list_t *listeners, bool add) {
    celix_bundle_context_service_tracker_tracker_entry_t *entry = handle;
    if (entry != NULL) {
        size_t size = celix_arrayList_size(listeners);
        for (unsigned int i = 0; i < size; ++i) {
            listener_hook_info_pt info = celix_arrayList_get(listeners, i);
            celix_bundle_t *bnd = NULL;
            bundleContext_getBundle(info->context, &bnd);

            celix_autoptr(celix_filter_t) filter = celix_filter_create(info->filter);
            celix_service_tracker_info_t trkInfo;
            memset(&trkInfo, 0, sizeof(trkInfo));
            trkInfo.bundleId = celix_bundle_getId(bnd);
            trkInfo.filter = filter;
            trkInfo.serviceName = celix_filter_findAttribute(filter, CELIX_FRAMEWORK_SERVICE_NAME);
            const char *filterSvcName = celix_filter_findAttribute(filter, CELIX_FRAMEWORK_SERVICE_NAME);

            bool match = entry->serviceName == NULL || (filterSvcName != NULL && strncmp(filterSvcName, entry->serviceName, 1024*1024) == 0);

            if (add && entry->add != NULL && match) {
                entry->add(entry->callbackHandle, &trkInfo);
            } else if (!add && entry->remove != NULL && match) {
                entry->remove(entry->callbackHandle, &trkInfo);
            }
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

long celix_bundleContext_trackServiceTrackersInternal(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info),
        bool async,
        void *doneData,
        void (*doneCallback)(void*)) {
    if (serviceName == NULL) {
        fw_log(ctx->framework->logger, CELIX_LOG_LEVEL_DEBUG, "Note starting a meta tracker for all services");
    }

    celix_bundle_context_service_tracker_tracker_entry_t *entry = calloc(1, sizeof(*entry));
    celixThreadMutex_lock(&ctx->mutex);
    entry->trackerId = ctx->nextTrackerId++;
    celixThreadMutex_unlock(&ctx->mutex);
    entry->ctx = ctx;
    entry->callbackHandle = callbackHandle;
    entry->add = trackerAdd;
    entry->remove = trackerRemove;
    entry->serviceName = celix_utils_strdup(serviceName);
    entry->hook.handle = entry;
    entry->hook.added = bundleContext_callServicedTrackerTrackerAdd;
    entry->hook.removed = bundleContext_callServicedTrackerTrackerRemove;

    if (!async && celix_framework_isCurrentThreadTheEventLoop(ctx->framework)) {
        //already on event loop, registering the "traditional way" i.e. chaining on the current thread
        service_registration_t* reg = NULL;
        bundleContext_registerService(ctx, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, &entry->hook, NULL, &reg);
        entry->serviceId = serviceRegistration_getServiceId(reg);
    } else if (async) {
        entry->serviceId = celix_framework_registerServiceAsync(ctx->framework, ctx->bundle, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, &entry->hook, NULL, NULL, NULL, NULL, doneData, doneCallback);
    } else {
        entry->serviceId = celix_framework_registerServiceAsync(ctx->framework, ctx->bundle, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, &entry->hook, NULL, NULL, NULL, NULL, doneData, doneCallback);
        celix_framework_waitForAsyncRegistration(ctx->framework, entry->serviceId);
    }

    if (entry->serviceId >= 0) {
        celixThreadMutex_lock(&ctx->mutex);
        hashMap_put(ctx->metaTrackers, (void*)entry->trackerId, entry);
        long trkId = entry->trackerId;
        celixThreadMutex_unlock(&ctx->mutex);
        return trkId;
    } else {
        celix_framework_log(ctx->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__, "Error registering service listener hook for service tracker tracker\n");
        free(entry);
        return -1L;
    }
}

long celix_bundleContext_trackServiceTrackers(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info)) {
    return celix_bundleContext_trackServiceTrackersInternal(ctx, serviceName, callbackHandle, trackerAdd, trackerRemove, false, NULL, NULL);
}

long celix_bundleContext_trackServiceTrackersAsync(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info),
        void *doneCallbackData,
        void (*doneCallback)(void* doneCallbackData)) {
    return celix_bundleContext_trackServiceTrackersInternal(ctx, serviceName, callbackHandle, trackerAdd, trackerRemove, true, doneCallbackData, doneCallback);
}

void celix_bundleContext_waitForEvents(celix_bundle_context_t* ctx) {
    celix_framework_waitUntilNoEventsForBnd(ctx->framework, celix_bundle_getId(ctx->bundle));
}

long celix_bundleContext_scheduleEvent(celix_bundle_context_t* ctx,
                                       const celix_scheduled_event_options_t* options) {
    return celix_framework_scheduleEvent(ctx->framework,
                                          celix_bundle_getId(ctx->bundle),
                                          options->name,
                                          options->initialDelayInSeconds,
                                          options->intervalInSeconds,
                                          options->callbackData,
                                          options->callback,
                                          options->removeCallbackData,
                                          options->removeCallback);
}

celix_status_t celix_bundleContext_wakeupScheduledEvent(celix_bundle_context_t* ctx, long scheduledEventId) {
    return celix_framework_wakeupScheduledEvent(ctx->framework, scheduledEventId);
}

celix_status_t celix_bundleContext_waitForScheduledEvent(celix_bundle_context_t* ctx,
                                                         long scheduledEventId,
                                                         double waitTimeInSeconds) {
    return celix_framework_waitForScheduledEvent(ctx->framework, scheduledEventId, waitTimeInSeconds);
}

bool celix_bundleContext_removeScheduledEvent(celix_bundle_context_t* ctx, long scheduledEventId) {
    return celix_framework_removeScheduledEvent(ctx->framework, false, true, scheduledEventId);
}

bool celix_bundleContext_removeScheduledEventAsync(celix_bundle_context_t* ctx, long scheduledEventId) {
    return celix_framework_removeScheduledEvent(ctx->framework, true, true, scheduledEventId);
}

bool celix_bundleContext_tryRemoveScheduledEventAsync(celix_bundle_context_t* ctx, long scheduledEventId) {
    return celix_framework_removeScheduledEvent(ctx->framework, true, false, scheduledEventId);
}

celix_bundle_t* celix_bundleContext_getBundle(const celix_bundle_context_t *ctx) {
    celix_bundle_t *bnd = NULL;
    if (ctx != NULL) {
        bnd = ctx->bundle;
    }
    return bnd;
}

long celix_bundleContext_getBundleId(const celix_bundle_context_t *ctx) {
    celix_bundle_t *bnd = celix_bundleContext_getBundle(ctx);
    return bnd == NULL ? -1L : celix_bundle_getId(bnd);
}

celix_framework_t* celix_bundleContext_getFramework(const celix_bundle_context_t *ctx) {
    celix_framework_t *fw = NULL;
    if (ctx != NULL) {
        fw = ctx->framework;
    }
    return fw;
}

const char* celix_bundleContext_getProperty(celix_bundle_context_t *ctx, const char *key, const char* defaultValue) {
    const char* val = defaultValue;
    if (ctx != NULL) {
        val = celix_framework_getConfigProperty(ctx->framework, key, defaultValue, NULL);
    }
    return val;
}

long celix_bundleContext_getPropertyAsLong(celix_bundle_context_t *ctx, const char *key, long defaultValue) {
    long val = defaultValue;
    if (ctx != NULL) {
        val = celix_framework_getConfigPropertyAsLong(ctx->framework, key, defaultValue, NULL);
    }
    return val;
}

double celix_bundleContext_getPropertyAsDouble(celix_bundle_context_t *ctx, const char *key, double defaultValue) {
    double val = defaultValue;
    if (ctx != NULL) {
        val = celix_framework_getConfigPropertyAsDouble(ctx->framework, key, defaultValue, NULL);
    }
    return val;
}

bool celix_bundleContext_getPropertyAsBool(celix_bundle_context_t *ctx, const char *key, bool defaultValue) {
    bool val = defaultValue;
    if (ctx != NULL) {
        val = celix_framework_getConfigPropertyAsBool(ctx->framework, key, defaultValue, NULL);
    }
    return val;
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

void celix_bundleContext_log(const celix_bundle_context_t *ctx, celix_log_level_e level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    celix_bundleContext_vlog(ctx, level, format, args);
    va_end(args);
}

void celix_bundleContext_vlog(const celix_bundle_context_t *ctx, celix_log_level_e level, const char *format, va_list formatArgs) {
    celix_framework_vlog(ctx->framework->logger, level, NULL, NULL, 0, format, formatArgs);
}

void celix_bundleContext_logTssErrors(const celix_bundle_context_t *ctx, celix_log_level_e level) {
    celix_framework_logTssErrors(ctx->framework->logger, level);
}