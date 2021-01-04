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

#include "celix_types.h"
#include "celix_service_factory.h"
#include "celix_properties.h"
#include "celix_array_list.h"
#include "celix_filter.h"

#ifndef CELIX_BUNDLE_CONTEXT_H_
#define CELIX_BUNDLE_CONTEXT_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init macro so that the opts are correctly initialized for C++ compilers
 */
#ifdef __cplusplus
#define OPTS_INIT {}
#else
#define OPTS_INIT
#endif


/**
 * Register a service to the Celix framework.
 *
 * The service will be registered async on the Celix event loop thread. This means that service registration is (probably)
 * not yet concluded when this function returns, but is added to the event loop.
 * Use celix_bundleContext_waitForAsyncRegistration to synchronise with the
 * actual service registration in the framework's service registry.
 *
 * @param ctx The bundle context
 * @param svc the service object. Normally a pointer to a service struct (i.e. a struct with function pointers)
 * @param serviceName the service name, cannot be NULL
 * @param properties The meta properties associated with the service. The service registration will take ownership of the properties (i.e. no destroy needed)
 * @return The serviceId (>=0) or -1 if the registration was unsuccessful.
 */
long celix_bundleContext_registerServiceAsync(celix_bundle_context_t *ctx, void *svc, const char *serviceName, celix_properties_t *properties);

/**
 * Register a service to the Celix framework.
 * Note: Please use the celix_bundleContext_registerServiceAsync instead.
 *
 * @param ctx The bundle context
 * @param svc the service object. Normally a pointer to a service struct (i.e. a struct with function pointers)
 * @param serviceName the service name, cannot be NULL
 * @param properties The meta properties associated with the service. The service registration will take ownership of the properties (i.e. no destroy needed)
 * @return The serviceId (>=0) or -1 if the registration was unsuccessful.
 */
long celix_bundleContext_registerService(celix_bundle_context_t *ctx, void *svc, const char *serviceName, celix_properties_t *properties); //__attribute__((deprecated("Use celix_bundleContext_registerServiceAsync instead!")));

/**
 * Register a service factory in the framework (for the C language).
 * The service factory will be called for every bundle requesting/de-requesting a service. This gives the provider the
 * option to create bundle specific service instances.
 *
 * When a service is requested for a bundle the getService of the factory service will be called. This function must
 * return a valid pointer to a service conform the registered service name or NULL.
 * When a service in no longer needed for a bundle (e.g. ending the useService(s) calls or when a service tracker is stopped)
 * the ungetService function of the service factory will be called.
 *
 * The service will be registered async on the Celix event loop thread. This means that service registration is (probably)
 * not yet concluded when this function returns, but is added to the event loop.
 * Use celix_bundleContext_waitForAsyncRegistration to synchronise with the
 * actual service registration in the framework's service registry.
 *
 * @param ctx The bundle context
 * @param factory The pointer to the factory service.
 * @param serviceName The required service name of the services this factory will produce.
 * @param properties The optional service factory properties. For a service consumer this will be seen as the service properties.
 * @return The serviceId (>= 0) or < 0 if the registration was unsuccessful.
 */
long celix_bundleContext_registerServiceFactoryAsync(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props);

/**
 * Register a service factory in the framework (for the C language).
 * The service factory will be called for every bundle requesting/de-requesting a service. This gives the provider the
 * option to create bundle specific service instances.
 * Note: Please use the celix_bundleContext_registerServiceFactoryAsync instead.
 *
 * When a service is requested for a bundle the getService of the factory service will be called. This function must
 * return a valid pointer to a service conform the registered service name or NULL.
 * When a service in no longer needed for a bundle (e.g. ending the useService(s) calls or when a service tracker is stopped)
 * the ungetService function of the service factory will be called.
 *
 * @param ctx The bundle context
 * @param factory The pointer to the factory service.
 * @param serviceName The required service name of the services this factory will produce.
 * @param properties The optional service factory properties. For a service consumer this will be seen as the service properties.
 * @return The serviceId (>= 0) or < 0 if the registration was unsuccessful.
 */
long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props); //__attribute__((deprecated("Use celix_bundleContext_registerServiceFactoryAsync instead!")));

/**
 * Service Registration Options when registering services to the Celix framework.
 */
typedef struct celix_service_registration_options {
    /**
     * The service pointer. The actual pointer to the service. For C this is normally a pointer to a struct
     * with function pointers, but theoretically this can be a pointer to anything (e.g. a pointer to a single function,
     * or a pointer to a C++ interface implementation, or just a pointer to a data structure).
     *
     * The bundle is responsible to keep the service pointer valid as long as it is registered in the Celix framework.
     */
    void *svc OPTS_INIT;

    /**
     * The service factory pointer.
     * Note if the factory service is set, the svc field will not be used.
     *
     * The service factory will be called for every bundle requesting/de-requesting a service. This gives the provider the
     * option to create bundle specific service instances.
     *
     * When a service is requested for a bundle the getService of the factory service will be called. This function must
     * return a valid pointer to a service conform the registered service name or NULL.
     * When a service in no longer needed for a bundle (e.g. ending the useService(s) calls when a service tacker is stopped)
     * the ungetService function of the service factory will be called.
     *
     * The bundle is responsible to keep the service factory pointer valid as long as it is registered in the Celix framework.
     */
    celix_service_factory_t *factory OPTS_INIT;

    /**
     * The required service name. This is used to identify the service. A fully qualified name with a namespace is
     * advisable to prevent name collision. (e.g. EXAMPLE_PRESSURE_SENSOR).
     */
    const char *serviceName OPTS_INIT;

    /**
     * The optional service properties. These contain meta information about the service in the
     * form of string key/values. (e.g. the location of a pressure sensor: location=left-tire).
     *
     * When a service is registered the Celix framework will take ownership of the provided properties.
     * If a registration fails, the properties will be destroyed (freed) by the Celix framework.
     */
    celix_properties_t *properties OPTS_INIT;

    /**
     * The optional service language. If this is NULL, CELIX_FRAMEWORK_SERVICE_LANGUAGE_C is used.
     */
    const char *serviceLanguage OPTS_INIT;

    /**
     * The optional service version (in the form of <MAJOR>.<MINOR>.<MICRO>.<QUALIFIER>).
     * If present consumer of the service can specific which service version range of
     * a specific service they are interested in. Note that it is the responsibility of the users to ensure that
     * service in those version range are compatible (binary of source). It is advisable to use semantic versioning
     * for this.
     */
    const char *serviceVersion OPTS_INIT;

    /**
     * Async data pointer for the async register callback.
     */
     void *asyncData OPTS_INIT;

    /**
    * Async callback. Will be called after the a service is registered in the service registry using a async call.
    * Will be called on the Celix event loop.
    */
    void (*asyncCallback)(void *data, long serviceId) OPTS_INIT;
} celix_service_registration_options_t;

/**
 * C Macro to create a empty celix_service_registration_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS { .svc = NULL, \
    .factory = NULL, \
    .serviceName = NULL, \
    .properties = NULL, \
    .serviceLanguage = NULL, \
    .serviceVersion = NULL, \
    .asyncData = NULL, \
    .asyncCallback = NULL }
#endif

/**
 * Register a service to the Celix framework using the provided service registration options.
 *
 * The service will be registered async on the Celix event loop thread. This means that service registration is (probably)
 * not yet concluded when this function returns, but is added to the event loop..
 * Use celix_bundleContext_waitForAsyncRegistration to synchronise with the
 * actual service registration in the framework's service registry.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the registration options. The options are only in the during registration call.
 * @return The serviceId (>= 0) or -1 if the registration was unsuccessful and -2 if the registration was cancelled (@see celix_bundleContext_reserveSvcId).
 */
long celix_bundleContext_registerServiceWithOptionsAsync(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts);

/**
 * Register a service to the Celix framework using the provided service registration options.
 * Note: Please use the celix_bundleContext_registerServiceAsyncWithOptions instead.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the registration options. The options are only in the during registration call.
 * @return The serviceId (>= 0) or -1 if the registration was unsuccessful and -2 if the registration was cancelled (@see celix_bundleContext_reserveSvcId).
 */
long celix_bundleContext_registerServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts); //__attribute__((deprecated("Use celix_bundleContext_registerServiceAsyncWithOptions instead!")));

/**
 * Waits til the async service registration for the provided serviceId is done.
 * Silently ignore service ids < 0.
 * Will directly return if there is no pending service registration for the provided service id.
 */
void celix_bundleContext_waitForAsyncRegistration(celix_bundle_context_t* ctx, long serviceId);

/**
 * Checks whether a service for the provided service id is registered in the service registry.
 * Note return false if the service for the provided service id is still pending in the event loop.
 * Silently ignore service ids < 0 (returns false).
 *
 * Returns true if the service is registered in the service registry.
 */
bool celix_bundleContext_isServiceRegistered(celix_bundle_context_t* ctx, long serviceId);


/**
 * Unregister the service or service factory with service id.
 * The service will only be unregistered if the bundle of the bundle context is the owner of the service.
 *
 * Will log an error if service id is unknown. Will silently ignore services ids < 0.
 *
 * @param ctx The bundle context
 * @param serviceId The service id
 */
void celix_bundleContext_unregisterService(celix_bundle_context_t *ctx, long serviceId); //__attribute__((deprecated("Use celix_bundleContext_unregisterService instead!")));


/**
 * Unregister the service or service factory with service id.
 * The service will only be unregistered if the bundle of the bundle context is the owner of the service.
 *
 * The service will be umregistered async on the Celix event loop thread. This means that service unregistration is (probably)
 * not yet concluded when this function returns. Use celix_bundleContext_waitForAsyncUnregistration to synchronise with the
 * actual service unregistration in the framework's service registry.
 *
 * @param ctx The bundle context
 * @param serviceId The service id
 * @param doneData The data used on the doneCallback (if present)
 * @param doneCallback If not NULL, this callback will be called when the unregisration is done. (will be called on the event loop thread)
 */
void celix_bundleContext_unregisterServiceAsync(celix_bundle_context_t *ctx, long serviceId, void* doneData, void (*doneCallback)(void* doneData));


/**
 * Waits til the async service unregistration for the provided serviceId is done.
 * Silently ignore service < 0.
 */
void celix_bundleContext_waitForAsyncUnregistration(celix_bundle_context_t* ctx, long serviceId);




/**
 * Finds the highest ranking service and returns the service id.
 *
 * @param ctx The bundle context
 * @param serviceName The required service name
 * @return If found a valid service id (>= 0) if not found -1.
 */
long celix_bundleContext_findService(celix_bundle_context_t *ctx, const char *serviceName);

/**
 * Finds the services with the provided service name and returns a list of the found service ids.
 *
 * @param ctx The bundle context
 * @param serviceName The required service name
 * @return A array list with as value a long int.
 */
celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char *serviceName);

/**
 * Service filter options which can be used to query for certain services.
 */
typedef struct celix_service_filter_options {
    /**
     * The service name.
     * If NULL is used any services which matches the filter string will be tracked.
     */
    const char* serviceName OPTS_INIT;

    /**
     * The optional version range. If service are registered with a service version this attribute can be used to
     * only select service with a version in the version range.
     * It uses the maven version range format, e.g. [1.0.0,2.0.0) or [1.1.1], etc.
     */
    const char* versionRange OPTS_INIT;

    /**
     * LDAP filter to use for fine tuning the filtering, e.g. (|(location=middle)(location=front))
     * The filter will be applied to all the user provided and framework provided service properties.
     */
    const char* filter OPTS_INIT;

    /**
     * The optional service language to filter for. If this is NULL or "" the C language will be used.
     */
    const char* serviceLanguage OPTS_INIT;


    /**
     * Whether to ignore (not filter for) the service.lang property.
     * If this is set the serviceLanguage field is ignored and the (service.lang=<>) part is not added tot he filter
     */
    bool ignoreServiceLanguage OPTS_INIT;
} celix_service_filter_options_t;

/**
 * C Macro to create a empty celix_service_filter_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_SERVICE_FILTER_OPTIONS {.serviceName = NULL, .versionRange = NULL, .filter = NULL, .serviceLanguage = NULL, .ignoreServiceLanguage = false}
#endif


/**
 * Finds the highest ranking service and returns the service id.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the filter options.
 * @return If found a valid service id (>= 0) if not found -1.
 */
long celix_bundleContext_findServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts);

/**
 * Finds the services conform the provider filter options and returns a list of the found service ids.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the filter options.
 * @return A array list with as value a long int.
 */
celix_array_list_t* celix_bundleContext_findServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts);

/**
 * track the highest ranking service with the provided serviceName.
 * The highest ranking services will used for the callback.
 * If a new and higher ranking services the callback with be called again with the new service.
 * If a service is removed a the callback with be called with next highest ranking service or NULL as service.
 *
 * The service tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track.
 *                    If NULL is all service are tracked.
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param set is a required callback, which will be called when a new highest ranking service is set.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServiceAsync(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*set)(void* handle, void* svc)
);

/**
 * track the highest ranking service with the provided serviceName.
 * The highest ranking services will used for the callback.
 * If a new and higher ranking services the callback with be called again with the new service.
 * If a service is removed a the callback with be called with next highest ranking service or NULL as service.
 * Note: Please use the celix_bundleContext_trackServiceAsync instead.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track.
 *                    If NULL is all service are tracked.
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param set is a required callback, which will be called when a new highest ranking service is set.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackService(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*set)(void* handle, void* svc)
); //__attribute__((deprecated("Use celix_bundleContext_trackServiceSync instead!")));

/**
 * track services with the provided serviceName.
 *
 * The service tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 *                    If NULL is all service are tracked.
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param add is a required callback, which will be called when a service is added and initially for the existing service.
 * @param remove is a required callback, which will be called when a service is removed
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServicesAsync(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*add)(void* handle, void* svc),
        void (*remove)(void* handle, void* svc)
);

/**
 * track services with the provided serviceName.
 * Note: Please use the celix_bundleContext_trackServicesAsync instead.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 *                    If NULL is all service are tracked.
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param add is a required callback, which will be called when a service is added and initially for the existing service.
 * @param remove is a required callback, which will be called when a service is removed
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServices(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*add)(void* handle, void* svc),
        void (*remove)(void* handle, void* svc)
); //__attribute__((deprecated("Use celix_bundleContext_trackServicesAsync instead!")));;

/**
 * Service Tracker Options used to fine tune which services to track and the callback to be used for the tracked services.
 */
typedef struct celix_service_tracking_options {
    /**
     * The service filter options, used to setup the filter for the service to track.
     */
    celix_service_filter_options_t filter OPTS_INIT;

    /**
     * The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void* callbackHandle OPTS_INIT;

    /**
     * The optional set callback will be called when a new highest ranking service is available conform the provided
     * service filter options.
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of the highest ranking service.
     */
    void (*set)(void *handle, void *svc) OPTS_INIT;

    /**
     * The optional setWithProperties callback is handled as the set callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*setWithProperties)(void *handle, void *svc, const celix_properties_t *props) OPTS_INIT; //highest ranking

    /**
     * The optional setWithOwner callback is handled as the set callback, but with the addition that the service properties
     * and the bundle owning the service will also be provided to the callback.
     */
    void (*setWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) OPTS_INIT; //highest ranking

    /**
     * The optional add callback will be called for every current and future service found conform the provided service filter
     * options as long as the tracker is active.
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of a service matching the provided service filter options.
     */
    void (*add)(void *handle, void *svc) OPTS_INIT;

    /**
     * The optional addWithProperties callback is handled as the add callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*addWithProperties)(void *handle, void *svc, const celix_properties_t *props) OPTS_INIT;

    /**
     * The optional addWithOwner callback is handled as the add callback, but with the addition that the service properties
     * and the bundle owning the service will also be provided to the callback.
     */
    void (*addWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) OPTS_INIT;

    /**
     * The optional remove callback will be called for every service conform the provided service filter options that is
     * unregistered. When the remove call is finished the removed services should be considered invalid. This means
     * that the callback provider should ensure that the removed service is not in use or going to be used after the
     * remove callback is finished.
     *
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of a service matching the provided service filter options.
     */
    void (*remove)(void *handle, void *svc) OPTS_INIT;

    /**
     * The optional removeWithProperties callback is handled as the remove callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*removeWithProperties)(void *handle, void *svc, const celix_properties_t *props) OPTS_INIT;

    /**
    * The optional removeWithOwner callback is handled as the remove callback, but with the addition that the service properties
    * and the bundle owning the service will also be provided to the callback.
    */
    void (*removeWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) OPTS_INIT;


    /**
     * Data for the trackerCreatedCallback.
     */
    void *trackerCreatedCallbackData OPTS_INIT;

    /**
     * The callback called when the tracker has ben created (and is active) when using a async call.
     */
    void (*trackerCreatedCallback)(void *trackerCreatedCallbackData) OPTS_INIT;
} celix_service_tracking_options_t;

/**
 * C Macro to create a empty celix_service_tracking_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_SERVICE_TRACKING_OPTIONS { .filter.serviceName = NULL, \
    .filter.versionRange = NULL, \
    .filter.filter = NULL, \
    .filter.serviceLanguage = NULL, \
    .filter.ignoreServiceLanguage = false, \
    .callbackHandle = NULL, \
    .set = NULL, \
    .add = NULL, \
    .remove = NULL, \
    .setWithProperties = NULL, \
    .addWithProperties = NULL, \
    .removeWithProperties = NULL, \
    .setWithOwner = NULL, \
    .addWithOwner = NULL, \
    .removeWithOwner = NULL, \
    .trackerCreatedCallbackData = NULL, \
    .trackerCreatedCallback = NULL }
#endif

/**
 * Tracks services using the provided tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 *
 * The service tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx The bundle context.
 * @param opts The pointer to the tracker options.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts);

/**
 * Tracks services using the provided tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * Note: Please use the celix_bundleContext_registerServiceFactoryAsync instead.
 *
 *
 * @param ctx The bundle context.
 * @param opts The pointer to the tracker options.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts); //__attribute__((deprecated("Use celix_bundleContext_trackServicesWithOptionsAsync instead!")));

/**
 * Stop the tracker with the provided track id.
 * Could be a service tracker, bundle tracker or service tracker tracker.
 * Only works for the trackers owned by the bundle of the bundle context.
 *
 * The service tracker will be destroyed async on the Celix event loop thread. This means that the function can return
 * before the tracker is destroyed.
 *
 * if the doneCallback is not NULL, this will be called when the destruction of the service tracker is done.
 * (will be called on the event loop thread).
 *
 * Will log a error if the provided tracker id is unknown. Will silently ignore trackerId < 0.
 */
void celix_bundleContext_stopTrackerAsync(
        celix_bundle_context_t *ctx,
        long trackerId,
        void *doneCallbackData,
        void (*doneCallback)(void* doneCallbackData));

/**
 * Wait for (async) creation of tracker
 */
void celix_bundleContext_waitForAsyncTracker(celix_bundle_context_t* ctx, long trackerId);

/**
 * Wait for (async) stopping of tracking.
 */
void celix_bundleContext_waitForAsyncStopTracker(celix_bundle_context_t* ctx, long trackerId);

/**
 * Stop the tracker with the provided track id.
 * Could be a service tracker, bundle tracker or service tracker tracker.
 * Only works for the trackers owned by the bundle of the bundle context.
 * Note: Please use the celix_bundleContext_registerServiceFactoryAsync instead.
 *
 * Will log a error if the provided tracker id is unknown. Will silently ignore trackerId < 0.
 */
void celix_bundleContext_stopTracker(celix_bundle_context_t *ctx, long trackerId); //__attribute__((deprecated("Use celix_bundleContext_stopTrackerAsync instead!")));



/**
 * Use the service with the provided service id using the provided callback. The Celix framework will ensure that
 * the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param ctx The bundle context
 * @param serviceId the service id.
 * @param serviceName the service name of the service. Should match with the registered service name of the provided service id (sanity check)
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param use The callback, which will be called when service is retrieved.
 * @param bool returns true if a service was found.
 */
bool celix_bundleContext_useServiceWithId(
        celix_bundle_context_t *ctx,
        long serviceId,
        const char *serviceName /*sanity check*/,
        void *callbackHandle,
        void (*use)(void *handle, void* svc)
);

/**
 * Use the highest ranking service with the provided service name using the provided callback. The Celix framework will
 * ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context
 * @param   serviceName the required service name.
 * @param   callbackHandle The data pointer, which will be used in the callbacks
 * @param   use The callback, which will be called when service is retrieved.
 * @return  True if a service was found.
 */
bool celix_bundleContext_useService(
        celix_bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)
);

/**
 * Use the services with the provided service name using the provided callback. The Celix framework will
 * ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context
 * @param   serviceName the required service name.
 * @param   callbackHandle The data pointer, which will be used in the callbacks
 * @param   use The callback, which will be called for every service found.
 * @return  The number of services found and called
 */
size_t celix_bundleContext_useServices(
        celix_bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)
);

/**
 * Service Use Options used to fine tune which services to use and which callbacks to use.
 */
typedef struct celix_service_use_options {
    /**
     * The service filter options, used to setup the filter for the service to track.
     */
    celix_service_filter_options_t filter OPTS_INIT;

    /**
     * An optional timeout (in seconds), if > 0 the use service call will block untill the timeout is expired or
     * when at least one service is found.
     * Default (0)
     */
     double waitTimeoutInSeconds OPTS_INIT;

    /**
     * The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void *callbackHandle OPTS_INIT;

    /**
     * The optional use callback will be called when for every services found conform the service filter options
     * - in case of findServices - or only for the highest ranking service found - in case of findService -.
     *
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of the highest ranking service.
     */
    void (*use)(void *handle, void *svc) OPTS_INIT;

    /**
     * The optional useWithProperties callback is handled as the use callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props) OPTS_INIT;

    /**
     * The optional useWithOwner callback is handled as the yse callback, but with the addition that the service properties
     * and the bundle owning the service will also be provided to the callback.
     */
    void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) OPTS_INIT;
} celix_service_use_options_t;

/**
 * C Macro to create a empty celix_service_use_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_SERVICE_USE_OPTIONS {.filter.serviceName = NULL, \
    .filter.versionRange = NULL, \
    .filter.filter = NULL, \
    .filter.serviceLanguage = NULL, \
    .waitTimeoutInSeconds = 0.0F, \
    .callbackHandle = NULL, \
    .use = NULL, \
    .useWithProperties = NULL, \
    .useWithOwner = NULL}
#endif

/**
 * Use the services with the provided service filter options using the provided callback. The Celix framework will
 * ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context.
 * @param   opts The required options. Note that the serviceName is required.
 * @return  True if a service was found.
 */
bool celix_bundleContext_useServiceWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts);


/**
 * Use the services with the provided service filter options using the provided callback. The Celix framework will
 * ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context.
 * @param   opts The required options. Note that the serviceName is required.
 * @return  The number of services found and called
 */
size_t celix_bundleContext_useServicesWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts);




/**
 * List the installed and started bundle ids.
 * The bundle ids does not include the framework bundle (bundle id 0).
 *
 * @param ctx The bundle context
 * @return A array with bundle ids (long). The caller is responsible for destroying the array.
 */
celix_array_list_t* celix_bundleContext_listBundles(celix_bundle_context_t *ctx);

/**
 * Check whether a bundle is installed.
 * @param ctx       The bundle context.
 * @param bndId     The bundle id to check
 * @return          true if the bundle is installed.
 */
bool celix_bundleContext_isBundleInstalled(celix_bundle_context_t *ctx, long bndId);

/**
 * Check whether the bundle is active.
 * @param ctx       The bundle context.
 * @param bndId     The bundle id to check
 * @return          true if the bundle is installed and active.
 */
bool celix_bundleContext_isBundleActive(celix_bundle_context_t *ctx, long bndId);


/**
 * Install and optional start a bundle.
 * Will silently ignore bundle ids < 0.
 *
 * @param ctx The bundle context
 * @param bundleLoc The bundle location to the bundle zip file.
 * @param autoStart If the bundle should also be started.
 * @return the bundleId (>= 0) or < 0 if the bundle could not be installed and possibly started.
 */
long celix_bundleContext_installBundle(celix_bundle_context_t *ctx, const char *bundleLoc, bool autoStart);

/**
 * Uninstall the bundle with the provided bundle id. If needed the bundle will be stopped first.
 * Will silently ignore bundle ids < 0.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to uninstall.
 * @return true if the bundle is correctly uninstalled. False if not.
 */
bool celix_bundleContext_uninstallBundle(celix_bundle_context_t *ctx, long bndId);

/**
 * Stop the bundle with the provided bundle id.
 * Will silently ignore bundle ids < 0.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to stop.
 * @return true if the bundle is found & correctly stop. False if not.
 */
bool celix_bundleContext_stopBundle(celix_bundle_context_t *ctx, long bndId);

/**
 * Start the bundle with the provided bundle id.
 * Will silently ignore bundle ids < 0.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to start.
 * @return true if the bundle is found & correctly started. False if not.
 */
bool celix_bundleContext_startBundle(celix_bundle_context_t *ctx, long bndId);

/**
 * Returns the bundle symbolic name for the provided bundle id.
 * The caller is owner of the return string.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to retrieve the symbolic name for.
 * @return The bundle symbolic name or NULL if the bundle for the provided bundle id does not exist.
 */
char* celix_bundleContext_getBundleSymbolicName(celix_bundle_context_t *ctx, long bndId);


/**
 * track bundles
 * The add bundle callback will also be called for already installed bundles.
 *
 * The bundle tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx               The bundle context.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param add               The callback which will be called for started bundles.
 * @param remove            The callback which will be called when bundles are stopped.
 * @return                  The bundle tracker id or < 0 if unsuccessful.
 */
long celix_bundleContext_trackBundlesAsync(
        celix_bundle_context_t* ctx,
        void* callbackHandle,
        void (*onStarted)(void* handle, const celix_bundle_t *bundle),
        void (*onStopped)(void *handle, const celix_bundle_t *bundle)
);

/**
 * track bundles
 * The add bundle callback will also be called for already installed bundles.
 *
 * Note: please use celix_bundleContext_trackBundlesAsync instead.
 *
 * @param ctx               The bundle context.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param add               The callback which will be called for started bundles.
 * @param remove            The callback which will be called when bundles are stopped.
 * @return                  The bundle tracker id or < 0 if unsuccessful.
 */
long celix_bundleContext_trackBundles(
        celix_bundle_context_t* ctx,
        void* callbackHandle,
        void (*onStarted)(void* handle, const celix_bundle_t *bundle),
        void (*onStopped)(void *handle, const celix_bundle_t *bundle)
); //__attribute__((deprecated("Use celix_bundleContext_trackBundlesAsync instead!")));


/**
 * The Service Bundle Tracking options can be used to fine tune the requested bundle tracker options.
 */
typedef struct celix_bundle_tracker_options {
    /**
     * The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void* callbackHandle OPTS_INIT;

    /**
     * Tracker callback when a bundle is installed.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been installed.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onInstalled)(void *handle, const celix_bundle_t *bundle) OPTS_INIT;

    /**
     * Tracker callback when a bundle is started.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been started.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onStarted)(void *handle, const celix_bundle_t *bundle) OPTS_INIT;

    /**
     * Tracker callback when a bundle is stopped.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been stopped.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onStopped)(void *handle, const celix_bundle_t *bundle) OPTS_INIT;

    /**
     *
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param event     The bundle event. Is only valid during the callback.
     */
    void (*onBundleEvent)(void *handle, const celix_bundle_event_t *event) OPTS_INIT;

    /**
     * Default the framework bundle (bundle id 0) will not trigger the callbacks.
     * This is done, because the framework bundle is a special bundle which is generally not needed in the callbacks.
     */
    bool includeFrameworkBundle OPTS_INIT;

    /**
     * Data for the trackerCreatedCallback.
     */
    void *trackerCreatedCallbackData OPTS_INIT;

    /**
     * The callback called when the tracker has ben created (and is active) when using the
     * track bundles ascync calls.
     */
    void (*trackerCreatedCallback)(void *trackerCreatedCallbackData) OPTS_INIT;
} celix_bundle_tracking_options_t;

/**
 * C Macro to create a empty celix_service_filter_options_t type.
 */
#ifndef __cplusplus
#define CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS {.callbackHandle = NULL, .onInstalled = NULL, .onStarted = NULL, .onStopped = NULL, .onBundleEvent = NULL, .includeFrameworkBundle = false, .trackerCreatedCallbackData = NULL, .trackerCreatedCallback = NULL}
#endif

/**
 * Tracks bundles using the provided bundle tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * (i.e. can be on the stack)
 *
 * The bundle tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx   The bundle context.
 * @param opts  The pointer to the bundle tracker options.
 * @return      The bundle tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackBundlesWithOptionsAsync(
        celix_bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts
);

/**
 * Tracks bundles using the provided bundle tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * (i.e. can be on the stack)
 *
 * Note: please use celix_bundleContext_trackBundlesWithOptionsAsync instead;
 *
 * @param ctx   The bundle context.
 * @param opts  The pointer to the bundle tracker options.
 * @return      The bundle tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackBundlesWithOptions(
        celix_bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts
); //__attribute__((deprecated("Use celix_bundleContext_trackBundlesWithOptionsAsync instead!")));

/**
 * Use the bundle with the provided bundle id if it is in the active (started) state
 * The provided callback will be called if the bundle is found and in the active (started) state.
 *
 * @param ctx               The bundle context.
 * @param bundleId          The bundle id.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 * @return                  Returns true if the bundle is found and the callback is called.
 */
bool celix_bundleContext_useBundle(
        celix_bundle_context_t *ctx,
        long bundleId,
        void *callbackHandle,
        void (*use)(void *handle, const celix_bundle_t *bundle)
);

/**
 * Use the currently active (started) bundles.
 * The provided callback will be called for all the currently started bundles (excluding the framework bundle).
 *
 * @param ctx               The bundle context.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 */
void celix_bundleContext_useBundles(
        celix_bundle_context_t *ctx,
        void *callbackHandle,
        void (*use)(void *handle, const celix_bundle_t *bundle)
);



//TODO add useBundleWithOptions (e.g. which state)
//TODO findBundles

/**
 * Service Tracker Info provided to the service tracker tracker callbacks.
 */
typedef struct celix_service_tracker_info {
    /**
     * The parsed service filter, e.g. parsed "(&(objectClass=example_calc)(service.language=C)(meta.info=foo))"
     */
    celix_filter_t *filter;

    /**
     *The service name filter attribute parsed from the service filter (i.e. the value of the objectClass attribute key)
     */
    const char *serviceName;

    /**
     * The service language filter attribute parsed from the service filter. Can be null
     */
    const char *serviceLanguage;

    /**
     * Bundle id of the owner of the service tracker.
     */
    long bundleId;
} celix_service_tracker_info_t;

/**
 * Track the service tracker targeting the provided service name. This can be used to track if there is an interest
 * in a certain service and ad-hoc act on that interest.
 *
 * Note that the celix_service_tracker_info_t pointer in the trackerAdd/trackerRemove callbacks are only valid during
 * the callback.
 *
 * This tracker can be stopped with the celix_bundleContext_stopTracker function.
 *
 * The service tracker tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx The bundle context
 * @param serviceName The target service name for the service tracker to track.
 *                      If NULL is provided, add/remove callbacks will be called for all service trackers in the framework.
 * @param callbackHandle The callback handle which will be provided as handle in the trackerAdd and trackerRemove callback.
 * @param trackerAdd Called when a service tracker is added, which tracks the provided service name. Will also be called
 *                   for all existing service tracker when this tracker is started.
 * @param trackerRemove Called when a service tracker is removed, which tracks the provided service name
 * @param doneCallbackData call back data argument provided to the done callback function.
 * @param doneCallback If not NULL will be called when the service tracker tracker is created.
 * @return The tracker id or <0 if something went wrong (will log an error).
 */
long celix_bundleContext_trackServiceTrackersAsync(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info),
        void *doneCallbackData,
        void (*doneCallback)(void* doneCallbackData));

/**
 * Track the service tracker targeting the provided service name. This can be used to track if there is an interest
 * in a certain service and ad-hoc act on that interest.
 *
 * Note that the celix_service_tracker_info_t pointer in the trackerAdd/trackerRemove callbacks are only valid during
 * the callback.
 *
 * Note: Please use celix_bundleContext_trackServiceTrackersAsync instead.
 *
 * This tracker can be stopped with the celix_bundleContext_stopTracker function.
 *
 * @param ctx The bundle context
 * @param serviceName The target service name for the service tracker to track.
 *                      If NULL is provided, add/remove callbacks will be called for all service trackers in the framework.
 * @param callbackHandle The callback handle which will be provided as handle in the trackerAdd and trackerRemove callback.
 * @param trackerAdd Called when a service tracker is added, which tracks the provided service name. Will also be called
 *                   for all existing service tracker when this tracker is started.
 * @param trackerRemove Called when a service tracker is removed, which tracks the provided service name
 * @return The tracker id or <0 if something went wrong (will log an error).
 */
long celix_bundleContext_trackServiceTrackers(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info)); //__attribute__((deprecated("Use celix_bundleContext_trackServiceTrackersAsync instead!")));

/**
 * Gets the dependency manager for this bundle context.
 *
 * @return the dependency manager or NULL if unsuccessful.
 */
celix_dependency_manager_t* celix_bundleContext_getDependencyManager(celix_bundle_context_t *ctx);


/**
 * Wait till there are event for the bundle of this bundle context.
 */
void celix_bundleContext_waitForEvents(celix_bundle_context_t* ctx);


/**
 * Returns the bundle for this bundle context.
 */
celix_bundle_t* celix_bundleContext_getBundle(const celix_bundle_context_t *ctx);

celix_framework_t* celix_bundleContext_getFramework(const celix_bundle_context_t* ctx);

/**
 * Gets the config property - or environment variable if the config property does not exist - for the provided name.
 * @param key The key of the property to receive.
 * @param defaultVal The default value to use if the property is not found (can be NULL).
 * @return The property value for the provided key or the provided defaultValue is the key is not found.
 */
const char* celix_bundleContext_getProperty(celix_bundle_context_t *ctx, const char *key, const char *defaultVal);

/**
 * Gets the config property as converts it to long. If the property is not a valid long, the defaultValue will be returned.
 * The rest of the behaviour is the same as celix_bundleContext_getProperty.

 * @param key The key of the property to receive.
 * @param defaultVal The default value to use if the property is not found.
 * @return The property value for the provided key or the provided defaultValue is the key is not found.
 */
long celix_bundleContext_getPropertyAsLong(celix_bundle_context_t *ctx, const char *key, long defaultValue);

/**
 * Gets the config property as converts it to double. If the property is not a valid double, the defaultValue will be returned.
 * The rest of the behaviour is the same as celix_bundleContext_getProperty.

 * @param key The key of the property to receive.
 * @param defaultVal The default value to use if the property is not found.
 * @return The property value for the provided key or the provided defaultValue is the key is not found.
 */
double celix_bundleContext_getPropertyAsDouble(celix_bundle_context_t *ctx, const char *key, double defaultValue);

/**
 * Gets the config property as converts it to bool. If the property is not a valid bool, the defaultValue will be returned.
 * The rest of the behaviour is the same as celix_bundleContext_getProperty.

 * @param key The key of the property to receive.
 * @param defaultVal The default value to use if the property is not found.
 * @return The property value for the provided key or the provided defaultValue is the key is not found.
 */
bool celix_bundleContext_getPropertyAsBool(celix_bundle_context_t *ctx, const char *key, bool defaultValue);

//TODO getPropertyAs for int, uint, ulong, bool, etc

#undef OPTS_INIT

#ifdef __cplusplus
}
#endif

#endif //CELIX_BUNDLE_CONTEXT_H_
