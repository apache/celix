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

#ifndef BUNDLE_CONTEXT_H_
#define BUNDLE_CONTEXT_H_

/**
 * A bundle's execution context within the Framework. The context is used to
 * grant access to other methods so that this bundle can interact with the
 * Framework.
 */

#include "celix_types.h"

#include "service_factory.h"
#include "celix_service_factory.h"
#include "service_listener.h"
#include "bundle_listener.h"
#include "framework_listener.h"
#include "properties.h"
#include "array_list.h"

#include "dm_dependency_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

celix_status_t
bundleContext_create(framework_pt framework, framework_logger_pt, bundle_pt bundle, bundle_context_pt *bundle_context);

celix_status_t bundleContext_destroy(bundle_context_pt context);

FRAMEWORK_EXPORT celix_status_t bundleContext_getBundle(bundle_context_pt context, bundle_pt *bundle);

FRAMEWORK_EXPORT celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework);

FRAMEWORK_EXPORT celix_status_t
bundleContext_installBundle(bundle_context_pt context, const char *location, bundle_pt *bundle);

FRAMEWORK_EXPORT celix_status_t
bundleContext_installBundle2(bundle_context_pt context, const char *location, const char *inputFile, bundle_pt *bundle);

FRAMEWORK_EXPORT celix_status_t
bundleContext_registerService(bundle_context_pt context, const char *serviceName, const void *svcObj,
                              properties_pt properties, service_registration_pt *service_registration);

FRAMEWORK_EXPORT celix_status_t
bundleContext_registerServiceFactory(bundle_context_pt context, const char *serviceName, service_factory_pt factory,
                                     properties_pt properties, service_registration_pt *service_registration);

/**
 * Get a service reference for the bundle context. When the service reference is no longer needed use bundleContext_ungetServiceReference.
 * ServiceReference are coupled to a bundle context. Do not share service reference between bundles. Exchange the service.id instead.
 * 
 * @param context The bundle context
 * @param serviceName The name of the service (objectClass) to get
 * @param service_reference _output_ The found service reference, or NULL when no service is found.
 * @return CELIX_SUCCESS on success
 */
FRAMEWORK_EXPORT celix_status_t bundleContext_getServiceReference(bundle_context_pt context, const char *serviceName,
                                                                  service_reference_pt *service_reference);

/** Same as bundleContext_getServiceReference, but than for a optional serviceName combined with a optional filter.
 * The resulting array_list should be destroyed by the caller. For all service references return a unget should be called.
 * 
 * @param context the bundle context
 * @param serviceName the serviceName, can be NULL
 * @param filter the filter, can be NULL. If present will be combined (and) with the serviceName 
 * @param service_references _output_ a array list, can be size 0. 
 * @return CELIX_SUCCESS on success
 */
FRAMEWORK_EXPORT celix_status_t
bundleContext_getServiceReferences(bundle_context_pt context, const char *serviceName, const char *filter,
                                   array_list_pt *service_references);

/**
 * Retains (increases the ref count) the provided service reference. Can be used to retain a service reference.
 * Note that this is a deviation from the OSGi spec, due to the fact that C has no garbage collect.
 * 
 * @param context the bundle context
 * @param reference the service reference to retain
 * @return CELIX_SUCCES on success
 */
FRAMEWORK_EXPORT celix_status_t
bundleContext_retainServiceReference(bundle_context_pt context, service_reference_pt reference);

/**
 * Ungets the service references. If the ref counter of the service refernce goes to 0, the reference will be destroyed.
 * This is coupled with the bundleContext_getServiceReference(s) and bundleContext_retainServiceReferenc.
 * Note: That this is a deviation from the OSGi standard, due to the fact that C has no garbage collect.
 * 
 * @param context The bundle context.
 * @param reference the service reference to unget
 * @return CELIX_SUCCESS on success.
 */
FRAMEWORK_EXPORT celix_status_t
bundleContext_ungetServiceReference(bundle_context_pt context, service_reference_pt reference);

FRAMEWORK_EXPORT celix_status_t
bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void **service_instance);

FRAMEWORK_EXPORT celix_status_t
bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result);

FRAMEWORK_EXPORT celix_status_t bundleContext_getBundles(bundle_context_pt context, array_list_pt *bundles);

FRAMEWORK_EXPORT celix_status_t bundleContext_getBundleById(bundle_context_pt context, long id, bundle_pt *bundle);

FRAMEWORK_EXPORT celix_status_t
bundleContext_addServiceListener(bundle_context_pt context, service_listener_pt listener, const char *filter);

FRAMEWORK_EXPORT celix_status_t
bundleContext_removeServiceListener(bundle_context_pt context, service_listener_pt listener);

FRAMEWORK_EXPORT celix_status_t bundleContext_addBundleListener(bundle_context_pt context, bundle_listener_pt listener);

FRAMEWORK_EXPORT celix_status_t
bundleContext_removeBundleListener(bundle_context_pt context, bundle_listener_pt listener);

FRAMEWORK_EXPORT celix_status_t
bundleContext_addFrameworkListener(bundle_context_pt context, framework_listener_pt listener);

FRAMEWORK_EXPORT celix_status_t
bundleContext_removeFrameworkListener(bundle_context_pt context, framework_listener_pt listener);

/**
 * Gets the config property - or environment variable if the config property does not exist - for the provided name.
 *
 * @param context The bundle context.
 * @param name The name of the config property / environment variable to get.
 * @param value A ptr to the output value. This will be set when a value is found or else will be set to NULL.
 * @return 0 if successful.
 */
FRAMEWORK_EXPORT celix_status_t
bundleContext_getProperty(bundle_context_pt context, const char *name, const char **value);

/**
 * Gets the config property - or environment variable if the config property does not exist - for the provided name.
 *
 * @param context The bundle context.
 * @param name The name of the config property / environment variable to get.
 * @param defaultValue The default value to return if no value is found.
 * @param value A ptr to the output value. This will be set when a value is found or else will be set to NULL.
 * @return 0 if successful.
 */
FRAMEWORK_EXPORT celix_status_t
bundleContext_getPropertyWithDefault(bundle_context_pt context, const char *name, const char *defaultValue, const char **value);





















/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

/**
 * Gets the dependency manager for this bundle context.
 *
 * @return the dependency manager or NULL if unsuccessful.
 */
dm_dependency_manager_t* celix_bundleContext_getDependencyManager(celix_bundle_context_t *ctx);

/**
 * Register a C lang service to the framework.
 *
 * @param ctx The bundle context
 * @param serviceName the service name, cannot be NULL
 * @param svc the service object. Normally a pointer to a service struct (e.g. a struct with function pointers)
 * @param serviceVersion The optional service version.
 * @param properties The meta properties associated with the service. The service registration will take ownership of the properties (e.g. no destroy needed)
 * @return The serviceId, which should be >= 0. If < 0 then the registration was unsuccessful.
 */
long celix_bundleContext_registerService(celix_bundle_context_t *ctx, const char *serviceName, void *svc, const char *serviceVersion, celix_properties_t *properties);

/**
* Register a service for the specified language to the framework.
*
* @param ctx The bundle context
* @param serviceName the service name, cannot be NULL
* @param svc Pointer to the service. Normally a pointer to a service struct (e.g. a struct with function pointers)
* @param serviceVersion The optional service version.
* @param lang The service language. Will be set to CELIX_FRAMEWORK_SERVICE_LANGUAGE_C if NULL is provided.
* @param properties The meta properties associated with the service. The service registration will take ownership of the properties
* @return The serviceId, which should >= 0. If < 0 then the registration was unsuccessful.
*/
long celix_bundleContext_registerServiceForLang(celix_bundle_context_t *ctx, const char *serviceName, void *svc, const char *serviceVersion, const char* lang, celix_properties_t *properties);

/**
 * Register a service factory in the framework (for the C language).
 * THe service factory will be called for every bundle requesting/de-requesting a service. This gives the provider the
 * option to create bundle specific service instances.
 *
 * When a service is requested for a bundle the getService of the factory service will be called. This function must
 * return a valid pointer to a service conform the registered service name or NULL.
 * When a service in no longer needed for a bundle (e.g. ending the useService(s) calls when a service tacker is stopped)
 * the ungetService function of the service factory will be called.
 *
 * @param ctx The bundle context
 * @param serviceName The required service name of the services this factory will produce.
 * @param factory The pointer to the factory service.
 * @param serviceVersion The optional service version.
 * @param properties The optional service factory properties. For a service consumer this will be seen as the service properties.
 * @return The service id of the service factory or < 0 if the registration was unsuccessful.
 */
long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, const char *serviceName, celix_service_factory_t *factory, const char *serviceVersion, celix_properties_t *properties);

/**
 * Register a service factory in the framework.
 * THe service factory will be called for every bundle requesting/de-requesting a service. This gives the provider the
 * option to create bundle specific service instances.
 *
 * When a service is requested for a bundle the getService of the factory service will be called. This function must
 * return a valid pointer to a service conform the registered service name or NULL.
 * When a service in no longer needed for a bundle (e.g. ending the useService(s) calls when a service tacker is stopped)
 * the ungetService function of the service factory will be called.
 *
 * @param ctx The bundle context
 * @param serviceName The required service name of the services this factory will produce.
 * @param factory The pointer to the factory service.
 * @param serviceVersion The optional service version.
 * @param lang The d service language. Will be set to CELIX_FRAMEWORK_SERVICE_LANGUAGE_C if NULL is provided.
 * @param properties The optional service factory properties. For a service consumer this will be seen as the service properties.
 * @return The service id of the service factory or < 0 if the registration was unsuccessful.
 */
long celix_bundleContext_registerServiceFactorForLang(celix_bundle_context_t *ctx, const char *serviceName, celix_service_factory_t *factory, const char *serviceVersion, const char *lang, celix_properties_t *properties);

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


//TODO 
//typedef struct celix_service_find_filter_options {
// servicname, versionrange, filter, lang
//} celix_service_find_filter_options_t;
//celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char *serviceName);
//celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char *serviceName);
//alts WithOptions


/**
 * track service for the provided serviceName and/or filter.
 * The highest ranking services will used for the callback.
 * If a new and higher ranking services the callback with be called again with the new service.
 * If a service is removed a the callback with be called with next highest ranking service or NULL as service.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 * @param serviceVersionRange Optional the service version range to track
 * @param filter Optional the LDAP filter to use
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param set is a required callback, which will be called when a new highest ranking service is set.
 * @return the tracker id or < 0 if unsuccessful.
 */
long celix_bundleContext_trackService(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*set)(void* handle, void* svc)
);

/**
 * track services for the provided serviceName and/or filter.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 * @param serviceVersionRange Optional the service version range to track
 * @param filter Optional the LDAP filter to use
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param add is a required callback, which will be called when a service is added and initially for the existing service.
 * @param remove is a required callback, which will be called when a service is removed
 * @return the tracker id or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServices(
        celix_bundle_context_t* ctx,
        const char* serviceName,
        void* callbackHandle,
        void (*add)(void* handle, void* svc),
        void (*remove)(void* handle, void* svc)
);

typedef struct celix_service_tracker_options {
    //service filter options
    const char* serviceName;
    const char* versionRange;
    const char* filter;
    const char* lang; //NULL -> 'CELIX_LANG_C'

    //callback options
    void* callbackHandle;

    void (*set)(void *handle, void *svc); //highest ranking
    void (*add)(void *handle, void *svc);
    void (*remove)(void *handle, void *svc);
    void (*modified)(void *handle, void *svc);

    void (*setWithProperties)(void *handle, void *svc, const celix_properties_t *props); //highest ranking
    void (*addWithProperties)(void *handle, void *svc, const celix_properties_t *props);
    void (*removeWithProperties)(void *handle, void *svc, const celix_properties_t *props);
    void (*modifiedWithProperties)(void *handle, void *svc, const celix_properties_t *props);

    void (*setWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner); //highest ranking
    void (*addWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);
    void (*removeWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);
    void (*modifiedWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);
} celix_service_tracking_options_t;

/**
 * Tracks services using the provided tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 *
 * @param ctx The bundle context.
 * @param opts The pointer to the tracker options.
 * @return the tracker id or < 0 if unsuccessful.
 */
long celix_bundleContext_trackServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_tracking_options_t *opts);



/**
 * Get and lock the service with the provided service id
 * Invokes the provided callback with the found service.
 * The svc, props and owner in the callback are only valid during the callback.
 * If no service is found the callback will not be invoked
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
 * Get and lock the current highest ranking service conform serviceName, versionRange an filter.
 * Invokes the provided callback with the found service.
 * The svc, props and owner in the callback are only valid during the callback.
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
 * Get and lock the current services conform serviceName, versionRange an filter.
 * Invokes the provided callback with the found services.
 * The svc, props and owner in the callback are only valid during the callback.
 * If no services are found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context
 * @param   serviceName the required service name.
 * @param   serviceRange the optional service version range (e.g. '[1.0.0,2.0.0)' )
 * @param   filter the optional filter.
 * @param   callbackHandle The data pointer, which will be used in the callbacks
 * @param   use The callback, which will be called for every service found.
 */
void celix_bundleContext_useServices(
        celix_bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc)
);


typedef struct celix_service_use_options {
    /**
     * service filter options. Note the serviceName is required.
     */
    const char *serviceName; //REQUIRED
    const char *versionRange; //default will be empty
    const char *filter; //default will be empty
    const char *lang; //default will be LANG_C

    /**
     * Callback info
     */
     void *callbackHandle;

     void (*use)(void *handle, void *svc);
     void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props);
     void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner);
} celix_service_use_options_t;

/**
 * Get and lock the current highest ranking service conform the service filter info from the provided options.
 *
 * Invokes the provided callback with the found service.
 * The svc, props and owner in the callback are only valid during the callback.
 * If no service is found the callback will not be invoked.
 *
 * This function will block till the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context.
 * @param   serviceName the required service name.
 * @param   opts The required options. Note that the serviceName is required.
 * @return  True if a service was found.
 */
bool celix_bundleContext_useServiceWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_use_options_t *opts);


/**
 * Get and lock the current services conform the service filter info from the provided options.
 *
 * Invokes the provided callback with the found services.
 * The svc, props and owner in the callback are only valid during the callback.
 * If no services are found the callback will not be invoked.
 * At least a serviceName needs to be provided, if not the callback is not invoked.
 *
 * This function will block till all the callbacks are finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context.
 * @param   serviceName the required service name.
 * @param   opts The required options. Note that the serviceName is required.
 * @param   callbackHandle The data pointer, which will be used in the callbacks.
 * @param   use The callback, which will be called for every service found.
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
 * @return the bundleId >= 0 or < 0 if the bundle could not be installed and possibly started.
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
 * Service tracker options. This struct can be used to fine grained tune the
 * requested bundle tracker ootions.
 */
typedef struct celix_bundle_tracker_options {
    //TODO options to track framework & own bundle

    /**
     * Handle used in the tracker callbacks.
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
} celix_bundle_tracker_options_t;

/**
 * Tracks bundles using the provided bundle tracker options.
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * (i.e. can be on the stack)
 *
 * @param ctx   The bundle context.
 * @param opts  The pointer to the bundle tracker options.
 * @return      The bundle tracker id or < 0 if unsuccessful.
 */
long celix_bundleContext_trackBundlesWithOptions(
        celix_bundle_context_t* ctx,
        const celix_bundle_tracker_options_t *opts
);

//TODO except framework & own bundle
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

//TODO add useBundleWithOptions (e.g. which state)


//TODO findBundles


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


//TODO trackServiceTracker

/**
 * Stop the tracker with the provided track id.
 * Could be a service tracker, bundle tracker or service tracker tracker.
 * Only works for the trackers owned by the bundle of the bundle context.
 *
 * Will log a error if the provided tracker id is unknown. Will silently ignore trackerId < 0.
 */
void celix_bundleContext_stopTracker(celix_bundle_context_t *ctx, long trackerId);



#ifdef __cplusplus
}
#endif

#endif /* BUNDLE_CONTEXT_H_ */
