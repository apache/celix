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

#include "celix_types.h"
#include "celix_service_factory.h"
#include "celix_properties.h"
#include "celix_array_list.h"

#ifndef CELIX_BUNDLE_CONTEXT_H_
#define CELIX_BUNDLE_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

/**
* Register a service to the Celix framework.
*
* @param ctx The bundle context
* @param svc the service object. Normally a pointer to a service struct (i.e. a struct with function pointers)
* @param serviceName the service name, cannot be NULL
* @param properties The meta properties associated with the service. The service registration will take ownership of the properties (e.g. no destroy needed)
* @return The serviceId (>= 0) or < 0 if the registration was unsuccessful.
*/
long celix_bundleContext_registerService(celix_bundle_context_t *ctx, void *svc, const char *serviceName, celix_properties_t *properties);

/**
 * Register a service factory in the framework (for the C language).
 * The service factory will be called for every bundle requesting/de-requesting a service. This gives the provider the
 * option to create bundle specific service instances.
 *
 * When a service is requested for a bundle the getService of the factory service will be called. This function must
 * return a valid pointer to a service conform the registered service name or NULL.
 * When a service in no longer needed for a bundle (e.g. ending the useService(s) calls when a service tacker is stopped)
 * the ungetService function of the service factory will be called.
 *
 * @param ctx The bundle context
 * @param factory The pointer to the factory service.
 * @param serviceName The required service name of the services this factory will produce.
 * @param properties The optional service factory properties. For a service consumer this will be seen as the service properties.
 * @return The serviceId (>= 0) or < 0 if the registration was unsuccessful.
 */
long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char *serviceName, celix_properties_t *props);

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
    void *svc;

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
    celix_service_factory_t *factory;

    /**
     * The required service name. This is used to identify the service. A fully qualified name with a namespace is
     * advisable to prevent name collision. (e.g. EXAMPLE_PRESURE_SENSOR).
     */
    const char *serviceName;

    /**
     * The optional service properties. These contain meta information about the service in the
     * form of string key/values. (e.g. the location of a pressure sensor: location=left-tire).
     *
     * When a service is registered the Celix framework will take ownership of the provided properties.
     * If a registration fails, the properties will be destroyed (freed) by the Celix framework.
     */
    celix_properties_t *properties;

    /**
     * The optional service language. If this is NULL, CELIX_FRAMEWORK_SERVICE_LANGUAGE_C is used.
     */
    const char *serviceLanguage;

    /**
     * The optional service version (in the form of <MAJOR>.<MINOR>.<MICRO>.<QUALIFIER>).
     * If present consumer of the service can specific which service version range of
     * a specific service they are interested in. Note that it is the responsibility of the users to ensure that
     * service in those version range are compatible (binary of source). It is advisable to use semantic versioning
     * for this.
     */
    const char *serviceVersion;
} celix_service_registration_options_t;

/**
 * Macro to create a empty celix_service_registration_options_t type.
 */
#define CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS { .svc = NULL, \
    .serviceName = NULL, \
    .properties = NULL, \
    .serviceLanguage = NULL, \
    .serviceVersion = NULL }


/**
* Register a service to the Celix framework using the provided service registration options.
*
* @param ctx The bundle context
* @param opts The pointer to the registration options. The options are only in the during registration call.
* @return The serviceId (>= 0) or < 0 if the registration was unsuccessful.
*/
long celix_bundleContext_registerServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts);


/**
 * Unregister the service or service factory with service id.
 * The service will only be unregistered if the bundle of the bundle context is the owner of the service.
 *
 * Will log an error if service id is unknown. Will silently ignore services ids < 0.
 *
 * @param ctx The bundle context
 * @param serviceId The service id
 */
void celix_bundleContext_unregisterService(celix_bundle_context_t *ctx, long serviceId);








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
     * The required service name.
     */
    const char* serviceName;

    /**
     * The optional version range. If service are registerd with a service version this attribute can be used to
     * only select service with a version in the version range.
     * It uses the maven version range format, e.g. [1.0.0,2.0.0) or [1.1.1], etc.
     */
    const char* versionRange;

    /**
     * LDAP filter to use for fine tuning the filtering, e.g. (|(location=middle)(location=front))
     * The filter will be applied to all the user provided and framework provided service properties.
     */
    const char* filter;

    /**
     * The optional service language to filter for. If this is NULL or "" the C language will be used.
     */
    const char* serviceLanguage;
} celix_service_filter_options_t;

/**
 * Macro to create a empty celix_service_filter_options_t type.
 */
#define CELIX_EMPTY_SERVICE_FILTER_OPTIONS {.serviceName = NULL, .versionRange = NULL, .filter = NULL, .serviceLanguage = NULL}


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
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param set is a required callback, which will be called when a new highest ranking service is set.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackService(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*set)(void* handle, void* svc)
);

/**
 * track services with the provided serviceName.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
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
);

/**
 * Service Tracker Options used to fine tune which services to track and the callback to be used for the tracked services.
 */
typedef struct celix_service_tracker_options {
    /**
     * The service filter options, used to setup the filter for the service to track.
     */
    celix_service_filter_options_t filter;

    /**
     * The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void* callbackHandle;

    /**
     * The optional set callback will be called when a new highest ranking service is available conform the provided
     * service filter options.
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of the highest ranking service.
     */
    void (*set)(void *handle, void *svc);

    /**
     * The optional setWithProperties callback is handled as the set callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*setWithProperties)(void *handle, void *svc, const celix_properties_t *props); //highest ranking

    /**
     * The optional setWithOwner callback is handled as the set callback, but with the addition that the service properties
     * and the bundle owning the service will also be provided to the callback.
     */
    void (*setWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner); //highest ranking

    /**
     * The optional add callback will be called for every current and future service found conform the provided service filter
     * options as long as the tracker is active.
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of a service matching the provided service filter options.
     */
    void (*add)(void *handle, void *svc);

    /**
     * The optional addWithProperties callback is handled as the add callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*addWithProperties)(void *handle, void *svc, const celix_properties_t *props);

    /**
     * The optional addWithOwner callback is handled as the add callback, but with the addition that the service properties
     * and the bundle owning the service will also be provided to the callback.
     */
    void (*addWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);

    /**
     * The optional remove callback will be called for every service conform the provided service filter options that is
     * unregistered. When the remove call is finished the removed services should be considered invalid. This means
     * that the callback provider should ensure that the removed service is not in use or going to be used after the
     * remove callback is finished.
     *
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of a service matching the provided service filter options.
     */
    void (*remove)(void *handle, void *svc);

    /**
     * The optional removeWithProperties callback is handled as the remove callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*removeWithProperties)(void *handle, void *svc, const celix_properties_t *props);

    /**
    * The optional removeWithOwner callback is handled as the remove callback, but with the addition that the service properties
    * and the bundle owning the service will also be provided to the callback.
    */
    void (*removeWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);
} celix_service_tracking_options_t;

/**
 * Macro to create a empty celix_service_tracking_options_t type.
 */
#define CELIX_EMPTY_SERVICE_TRACKING_OPTIONS { .filter.serviceName = NULL, \
    .filter.versionRange = NULL, \
    .filter.filter = NULL, \
    .filter.serviceLanguage = NULL, \
    .callbackHandle = NULL, \
    .set = NULL, \
    .add = NULL, \
    .remove = NULL, \
    .setWithProperties = NULL, \
    .addWithProperties = NULL, \
    .removeWithProperties = NULL, \
    .setWithOwner = NULL, \
    .addWithOwner = NULL, \
    .removeWithOwner = NULL}

/**
 * Tracks services using the provided tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 *
 * @param ctx The bundle context.
 * @param opts The pointer to the tracker options.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts);

/**
 * Stop the tracker with the provided track id.
 * Could be a service tracker, bundle tracker or service tracker tracker.
 * Only works for the trackers owned by the bundle of the bundle context.
 *
 * Will log a error if the provided tracker id is unknown. Will silently ignore trackerId < 0.
 */
void celix_bundleContext_stopTracker(celix_bundle_context_t *ctx, long trackerId);






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
 */
void celix_bundleContext_useServices(
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
    celix_service_filter_options_t filter;

    /**
     * The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void *callbackHandle;

    /**
     * The optional use callback will be called when for every services found conform the service filter options
     * - in case of findServices - or only for the highest ranking service found - in case of findService -.
     *
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of the highest ranking service.
     */
    void (*use)(void *handle, void *svc);

    /**
     * The optional useWithProperties callback is handled as the use callback, but with the addition that the service properties
     * will also be provided to the callback.
     */
    void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props);

    /**
     * The optional useWithOwner callback is handled as the yse callback, but with the addition that the service properties
     * and the bundle owning the service will also be provided to the callback.
     */
    void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);
} celix_service_use_options_t;

/**
 * Macro to create a empty celix_service_use_options_t type.
 */
#define CELIX_EMPTY_SERVICE_USE_OPTIONS {.filter.serviceName = NULL, \
    .filter.versionRange = NULL, \
    .filter.filter = NULL, \
    .filter.serviceLanguage = NULL, \
    .callbackHandle = NULL, \
    .use = NULL, \
    .useWithProperties = NULL, \
    .useWithOwner = NULL}

/**
 * Use the services with the provided service filter optons using the provided callback. The Celix framework will
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
 * Use the services with the provided service filter optons using the provided callback. The Celix framework will
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
 */
void celix_bundleContext_useServicesWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts);







/**
 * Install and optional start a bundle.
 *
 * @param ctx The bundle context
 * @param bundleLoc The bundle location to the bundle zip file.
 * @param autoStart If the bundle should also be started.
 * @return the bundleId (>= 0) or < 0 if the bundle could not be installed and possibly started.
 */
long celix_bundleContext_installBundle(celix_bundle_context_t *ctx, const char *bundleLoc, bool autoStart);

/**
 * Uninstall the bundle with the provided bundle id. If needed the bundle will be stopped first.
 *
 * @param ctx The bundle context
 * @param bundleId The bundle id to stop
 * @return true if the bundle is correctly uninstalled. False if not.
 */
bool celix_bundleContext_uninstallBundle(celix_bundle_context_t *ctx, long bundleId);

/**
 * track bundles
 * The add bundle callback will also be called for already installed bundles.
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
);


/**
 * The Service Bundle Tracking options can be used to fine tune the requested bundle tracker options.
 */
typedef struct celix_bundle_tracker_options {
    /**
     * The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void* callbackHandle;

    /**
     * Tracker callback when a bundle is installed.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been started.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onStarted)(void *handle, const celix_bundle_t *bundle); //highest ranking

    /**
     * Tracker callback when a bundle is removed.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been started.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onStopped)(void *handle, const celix_bundle_t *bundle);

    //TODO callback for on installed, resolved, uninstalled ??

    /**
     *
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param event     The bundle event. Is only valid during the callback.
     */
    void (*onBundleEvent)(void *handle, const celix_bundle_event_t *event);
} celix_bundle_tracking_options_t;

/**
 * Macro to create a empty celix_service_filter_options_t type.
 */
#define CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS {.callbackHandle = NULL, .onStarted = NULL, .onStopped = NULL, .onBundleEvent = NULL}

/**
 * Tracks bundles using the provided bundle tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * (i.e. can be on the stack)
 *
 * @param ctx   The bundle context.
 * @param opts  The pointer to the bundle tracker options.
 * @return      The bundle tracker id (>=0) or < 0 if unsuccessful.
 */
long celix_bundleContext_trackBundlesWithOptions(
        celix_bundle_context_t* ctx,
        const celix_bundle_tracking_options_t *opts
);

/**
 * Use the bundle with the provided bundle id if it is in the active (started) state
 * The provided callback will be called if the bundle is found and in the active (started) state.
 *
 * @param ctx               The bundle context.
 * @param bundleId          The bundle id.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 */
void celix_bundleContext_useBundle(
        celix_bundle_context_t *ctx,
        long bundleId,
        void *callbackHandle,
        void (*use)(void *handle, const celix_bundle_t *bundle)
);

/**
 * Use the currently active (started) bundles.
 * The provided callback will be called for all the currently started bundles.
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
//TODO trackServiceTracker

/**
 * Gets the dependency manager for this bundle context.
 *
 * @return the dependency manager or NULL if unsuccessful.
 */
dm_dependency_manager_t* celix_bundleContext_getDependencyManager(celix_bundle_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif //CELIX_BUNDLE_CONTEXT_H_
