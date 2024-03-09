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

#ifndef CELIX_BUNDLE_CONTEXT_H_
#define CELIX_BUNDLE_CONTEXT_H_

#include "celix_bundle_context_type.h"
#include "celix_framework_export.h"
#include "celix_filter.h"
#include "celix_service_factory.h"
#include "celix_bundle_event.h"
#include "celix_log_level.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init macro so that the opts are correctly initialized for C++ compilers
 */
#ifdef __cplusplus
#define CELIX_OPTS_INIT {}
#else
#define CELIX_OPTS_INIT
#endif

/**
 * @brief Register a service to the Celix framework.
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_registerServiceAsync(celix_bundle_context_t *ctx, void *svc, const char* serviceName, celix_properties_t *properties);

/**
 * @brief Register a service to the Celix framework.
 *
 * Note: Please use the celix_bundleContext_registerServiceAsync instead.
 *
 * @param ctx The bundle context
 * @param svc the service object. Normally a pointer to a service struct (i.e. a struct with function pointers)
 * @param serviceName the service name, cannot be NULL
 * @param properties The meta properties associated with the service. The service registration will take ownership of the properties (i.e. no destroy needed)
 * @return The serviceId (>=0) or -1 if the registration was unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_registerService(celix_bundle_context_t *ctx, void *svc, const char* serviceName, celix_properties_t *properties);

/**
 * @brief Register a service factory in the framework.
 *
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_registerServiceFactoryAsync(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char* serviceName, celix_properties_t *props);

/**
 * @brief Register a service factory in the framework.
 *
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_registerServiceFactory(celix_bundle_context_t *ctx, celix_service_factory_t *factory, const char* serviceName, celix_properties_t *props);

/**
 * @brief Service Registration Options when registering services to the Celix framework.
 */
typedef struct celix_service_registration_options {
    /**
     * @brief The service pointer.
     *
     * The actual pointer to the service. For C this is normally a pointer to a struct
     * with function pointers, but theoretically this can be a pointer to anything (e.g. a pointer to a single function,
     * or a pointer to a C++ interface implementation, or just a pointer to a data structure).
     *
     * The bundle is responsible to keep the service pointer valid as long as it is registered in the Celix framework.
     */
    void *svc CELIX_OPTS_INIT;

    /**
     * @brief The service factory pointer.
     *
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
    celix_service_factory_t *factory CELIX_OPTS_INIT;

    /**
     * @brief The required service name.
     *
     * This is used to identify the service. A fully qualified name with a namespace is
     * advisable to prevent name collision. (e.g. EXAMPLE_PRESSURE_SENSOR).
     */
    const char* serviceName CELIX_OPTS_INIT;

    /**
     * @brief The optional service properties.
     *
     * These contain meta information about the service in the
     * form of string key/values. (e.g. the location of a pressure sensor: location=left-tire).
     *
     * When a service is registered the Celix framework will take ownership of the provided properties.
     * If a registration fails, the properties will be destroyed (freed) by the Celix framework.
     */
    celix_properties_t *properties CELIX_OPTS_INIT;

    /**
     * @brief The optional service version (in the form of <MAJOR>.<MINOR>.<MICRO>.<QUALIFIER>).
     *
     * If present consumer of the service can specific which service version range of
     * a specific service they are interested in. Note that it is the responsibility of the users to ensure that
     * service in those version range are compatible (binary of source). It is advisable to use semantic versioning
     * for this.
     */
    const char* serviceVersion CELIX_OPTS_INIT;

    /**
     * @brief Async data pointer for the async register callback.
     */
     void *asyncData CELIX_OPTS_INIT;

    /**
     * @brief Async callback.
     *
     * Will be called after the a service is registered in the service registry using a async call.
     * Will be called on the Celix event loop.
     *
     * If a asyns service registration is combined with a _sync_ service unregistration, it can happen that
     * unregistration happens before the registration event is processed. In this case the asyncCallback
     * will not be called.
     */
    void (*asyncCallback)(void *data, long serviceId) CELIX_OPTS_INIT;
} celix_service_registration_options_t;

#ifndef __cplusplus
/*!
 * @brief C Macro to create a empty celix_service_registration_options_t type.
 */
#define CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS { .svc = NULL, \
    .factory = NULL, \
    .serviceName = NULL, \
    .properties = NULL, \
    .serviceVersion = NULL, \
    .asyncData = NULL, \
    .asyncCallback = NULL }
#endif

/**
 * @brief Register a service to the Celix framework using the provided service registration options.
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_registerServiceWithOptionsAsync(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts);

/**
 * @brief Register a service to the Celix framework using the provided service registration options.
 *
 * Note: Please use the celix_bundleContext_registerServiceAsyncWithOptions instead.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the registration options. The options are only in the during registration call.
 * @return The serviceId (>= 0) or -1 if the registration was unsuccessful and -2 if the registration was cancelled (@see celix_bundleContext_reserveSvcId).
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_registerServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_registration_options_t *opts);

/**
 * @brief Waits til the async service registration for the provided serviceId is done.
 *
 * Silently ignore service ids < 0.
 * Will directly return if there is no pending service registration for the provided service id.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_waitForAsyncRegistration(celix_bundle_context_t *ctx, long serviceId);

/**
 * @brief Checks whether a service for the provided service id is registered in the service registry.
 *
 * Note return false if the service for the provided service id is still pending in the event loop.
 * Silently ignore service ids < 0 (returns false).
 *
 * Returns true if the service is registered in the service registry.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_isServiceRegistered(celix_bundle_context_t *ctx, long serviceId);


/**
 * @brief Unregister the service or service factory with service id.
 *
 * The service will only be unregistered if the bundle of the bundle context is the owner of the service.
 *
 * Will log an error if service id is unknown. Will silently ignore services ids < 0.
 *
 * @param ctx The bundle context
 * @param serviceId The service id
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_unregisterService(celix_bundle_context_t *ctx, long serviceId);

/**
 * @brief Service registration guard.
 */
typedef struct celix_service_registration_guard {
    celix_bundle_context_t* ctx;
    long svcId;
} celix_service_registration_guard_t;

/**
 * @brief Initialize a a scope guard for an existing service registration.
 * @param [in] ctx The bundle context associated with the service registration.
 * @param [in] serviceId The service id.
 * @return An initialized service registration guard.
 */
static CELIX_UNUSED inline celix_service_registration_guard_t
celix_serviceRegistrationGuard_init(celix_bundle_context_t* ctx, long serviceId) {
    return (celix_service_registration_guard_t) { .ctx = ctx, .svcId = serviceId };
}

/**
 * @brief Deinitialize a service registration guard.
 * Will unregister the service if the service id is >= 0.
 * @param [in] reg  A service registration guard
 */
static CELIX_UNUSED inline void celix_serviceRegistrationGuard_deinit(celix_service_registration_guard_t* reg) {
    if (reg->svcId >= 0) {
        celix_bundleContext_unregisterService(reg->ctx, reg->svcId);
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_service_registration_guard_t, celix_serviceRegistrationGuard_deinit)

/**
 * @brief Unregister the service or service factory with service id.
 *
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
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_unregisterServiceAsync(celix_bundle_context_t *ctx, long serviceId, void* doneData, void (*doneCallback)(void* doneData));

/**
 * @brief Waits til the async service unregistration for the provided serviceId is done.
 *
 * Silently ignore service < 0.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_waitForAsyncUnregistration(celix_bundle_context_t *ctx, long serviceId);

/**
 * @brief Finds the highest ranking service and returns the service id.
 *
 * @param ctx The bundle context
 * @param serviceName The required service name
 * @return If found a valid service id (>= 0) if not found -1.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_findService(celix_bundle_context_t *ctx, const char* serviceName);

/**
 * @brief Finds the services with the provided service name and returns a list of the found service ids.
 *
 * @param ctx The bundle context
 * @param serviceName The required service name
 * @return A array list with as value a long int.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_bundleContext_findServices(celix_bundle_context_t *ctx, const char* serviceName);

/**
 * @brief Service filter options which can be used to query for certain services.
 */
typedef struct celix_service_filter_options {
    /**
     * @brief The service name.
     *
     * If NULL is used any services which matches the filter string will be tracked.
     */
    const char* serviceName CELIX_OPTS_INIT;

    /**
     * @brief The optional version range.
     *
     * If service are registered with a service version this attribute can be used to
     * only select service with a version in the version range.
     * It uses the maven version range format, e.g. [1.0.0,2.0.0) or [1.1.1], etc.
     */
    const char* versionRange CELIX_OPTS_INIT;

    /**
     * @brief LDAP filter to use for fine tuning the filtering, e.g. (|(location=middle)(location=front))
     *
     * The filter will be applied to all the user provided and framework provided service properties.
     */
    const char* filter CELIX_OPTS_INIT;
} celix_service_filter_options_t;

#ifndef __cplusplus
/*!
 * @brief C Macro to create a empty celix_service_filter_options_t type.
 */
#define CELIX_EMPTY_SERVICE_FILTER_OPTIONS {.serviceName = NULL, .versionRange = NULL, .filter = NULL}
#endif

/**
 * @brief Finds the highest ranking service and returns the service id.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the filter options.
 * @return If found a valid service id (>= 0) if not found -1.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_findServiceWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts);

/**
 * @brief Finds the services conform the provider filter options and returns a list of the found service ids.
 *
 * @param ctx The bundle context
 * @param opts The pointer to the filter options.
 * @return A array list with as value a long int.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_bundleContext_findServicesWithOptions(celix_bundle_context_t *ctx, const celix_service_filter_options_t *opts);

/**
 * @brief Use the service with the provided service id using the provided callback. The Celix framework will ensure that
 * the targeted service cannot be removed during the callback.
 *
 * @deprecated celix_bundleContext_useService are deprecated and should be considered test utils functions. In
 * operational code use celix_bundleContext_trackService* combined with celix_bundleContext_useTrackedService* functions
 * instead.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found, the callback will not be invoked and this function will return false immediately.
 *
 * This function will block until the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param ctx The bundle context
 * @param serviceId the service id.
 * @param serviceName the service name of the service. Should match with the registered service name of the provided
 * service id (sanity check)
 * @param callbackHandle The data pointer, which will be used in the callbacks
 * @param use The callback, which will be called when service is retrieved.
 * @param bool returns true if a service was found.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT bool celix_bundleContext_useServiceWithId(celix_bundle_context_t* ctx,
                                                                            long serviceId,
                                                                            const char* serviceName /*sanity check*/,
                                                                            void* callbackHandle,
                                                                            void (*use)(void* handle, void* svc));

/**
 * @brief Use the highest ranking service with the provided service name using the provided callback.
 *
 * @deprecated celix_bundleContext_useService are deprecated and should be considered test utils functions. In
 * operational code use celix_bundleContext_trackService* combined with celix_bundleContext_useTrackedService* functions
 * instead.
 *
 * The Celix framework will ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found, the callback will not be invoked and this function will return false immediately.
 *
 * This function will block until the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context
 * @param   serviceName the required service name.
 * @param   callbackHandle The data pointer, which will be used in the callbacks
 * @param   use The callback, which will be called when service is retrieved.
 * @return  True if a service was found.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT bool celix_bundleContext_useService(
    celix_bundle_context_t *ctx,
    const char* serviceName,
    void *callbackHandle,
    void (*use)(void *handle, void *svc)
);

/**
 * @brief Use the services with the provided service name using the provided callback.
 *
 * @deprecated celix_bundleContext_useService are deprecated and should be considered test utils functions. In
 * operational code use celix_bundleContext_trackService* combined with celix_bundleContext_useTrackedService* functions
 * instead.
 *
 * The Celix framework will ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found, the callback will not be invoked and this function will return 0 immediately.
 *
 * This function will block until the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context
 * @param   serviceName the required service name.
 * @param   callbackHandle The data pointer, which will be used in the callbacks
 * @param   use The callback, which will be called for every service found.
 * @return  The number of services found and called
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT size_t celix_bundleContext_useServices(
    celix_bundle_context_t *ctx,
    const char* serviceName,
    void *callbackHandle,
    void (*use)(void *handle, void *svc)
);

/**
 * @brief Service Use Options used to fine tune which services to use and which callbacks to use.
 *
 * If multiple use callbacks are set, all set callbacks will be called for every service found.
 */
typedef struct celix_service_use_options {
    /**
     * @brief The service filter options, used to setup the filter for the service to track.
     */
    celix_service_filter_options_t filter CELIX_OPTS_INIT;

    /**
     * @brief An optional timeout (in seconds), if > 0 the use service call will block until the timeout is expired or
     * when at least one service is found. Note that it will be ignored when use service on the event loop.
     * Default (0)
     *
     * Only applicable when using the celix_bundleContext_useService or
     * celix_bundleContext_useServiceWithOptions (use single service calls).
     */
    double waitTimeoutInSeconds CELIX_OPTS_INIT;

    /**
     * @brief The optional callback pointer used in all the provided callback function (use, useWithProperties, and
     * useWithOwner).
     */
    void* callbackHandle CELIX_OPTS_INIT;

    /**
     * @brief The optional use callback will be called when for every services found conform the service filter options
     * - in case of findServices - or only for the highest ranking service found - in case of findService -.
     *
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of the highest ranking service.
     */
    void (*use)(void* handle, void* svc) CELIX_OPTS_INIT;

    /**
     * @brief The optional useWithProperties callback is handled as the use callback, but with the addition that the
     * service properties will also be provided to the callback.
     */
    void (*useWithProperties)(void* handle, void* svc, const celix_properties_t* props) CELIX_OPTS_INIT;

    /**
     * @brief The optional useWithOwner callback is handled as the yse callback, but with the addition that the service
     * properties and the bundle owning the service will also be provided to the callback.
     */
    void (*useWithOwner)(void* handle, void* svc, const celix_properties_t* props, const celix_bundle_t* svcOwner)
        CELIX_OPTS_INIT;
    /**
     * @brief Call the provided callbacks from the caller thread directly if set, otherwise the callbacks will be called
     * from the Celix event loop (most likely indirectly). Note that using blocking service in the Celix event loop is
     * generally a bad idea, which should be avoided if possible.
     */
#define CELIX_SERVICE_USE_DIRECT (1)
    /**
     * @brief Whether "service on demand" pattern is supported when CELIX_SERVICE_USE_DIRECT is set.
     * Note that it has no effect in indirect mode, in which case "service on demand" is supported.
     */
#define CELIX_SERVICE_USE_SOD (2)
    int flags CELIX_OPTS_INIT;
} celix_service_use_options_t;

#ifndef __cplusplus
/*!
 * @brief C Macro to create a empty celix_service_use_options_t type.
 */
#define CELIX_EMPTY_SERVICE_USE_OPTIONS {.filter.serviceName = NULL, \
    .filter.versionRange = NULL, \
    .filter.filter = NULL, \
    .waitTimeoutInSeconds = 0.0F, \
    .callbackHandle = NULL, \
    .use = NULL, \
    .useWithProperties = NULL, \
    .useWithOwner = NULL, \
    .flags=0}
#endif

/**
 * @brief Use the highest ranking service satisfying the provided service filter options using the provided callback.
 *
 * @deprecated celix_bundleContext_useService are deprecated and should be considered test utils functions. In
 * operational code use celix_bundleContext_trackService* combined with celix_bundleContext_useTrackedService* functions
 * instead.
 *
 * The Celix framework will ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found the callback will not be invoked. In such cases, if a non-zero waitTimeoutInSeconds is specified in opts,
 * this function will block until the timeout is expired or when at least one service is found, otherwise it will return false immediately.
 *
 * This function will block until the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context.
 * @param   opts The required options. Note that the serviceName is required.
 * @return  True if a service was found.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT bool celix_bundleContext_useServiceWithOptions(
    celix_bundle_context_t *ctx,
    const celix_service_use_options_t *opts);


/**
 * @brief Use the services with the provided service filter options using the provided callback.
 *
 * @deprecated celix_bundleContext_useService are deprecated and should be considered test utils functions. In
 * operational code use celix_bundleContext_trackService* combined with celix_bundleContext_useTrackedService* functions
 * instead.
 *
 * The Celix framework will ensure that the targeted service cannot be removed during the callback.
 *
 * The svc is should only be considered valid during the callback.
 * If no service is found, the callback will not be invoked and this function will return 0 immediately.
 * Note that waitTimeoutInSeconds in opts has no effect.
 *
 * This function will block until the callback is finished. As result it is possible to provide callback data from the
 * stack.
 *
 * @param   ctx The bundle context.
 * @param   opts The required options. Note that the serviceName is required.
 * @return  The number of services found and called
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT size_t celix_bundleContext_useServicesWithOptions(
    celix_bundle_context_t *ctx,
    const celix_service_use_options_t *opts);

/**
 * @brief Track services with the provided serviceName.
 *
 * The service tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 *                    If NULL is all service are tracked.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackServicesAsync(celix_bundle_context_t* ctx,
                                                                   const char* serviceName);

/**
 * @brief Track services with the provided serviceName.
 *
 * Note: If possible, use the celix_bundleContext_trackServicesAsync instead.
 *
 * @param ctx The bundle context.
 * @param serviceName The required service name to track
 *                    If NULL is all service are tracked.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackServices(celix_bundle_context_t* ctx, const char* serviceName);

/**
 * @brief Service Tracker Options used to fine tune which services to track and the callback to be used for the tracked
 * services.
 */
typedef struct celix_service_tracking_options {
    /**
     * @brief The service filter options, used to setup the filter for the service to track.
     */
    celix_service_filter_options_t filter CELIX_OPTS_INIT;

    /**
     * @brief The optional callback pointer used in all the provided callback function (set, add, remove,
     * setWithProperties, etc).
     */
    void* callbackHandle CELIX_OPTS_INIT;

    /**
     * @brief The optional set callback will be called when a new highest ranking service is available conform the
     * provided service filter options.
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of the highest ranking service.
     */
    void (*set)(void* handle, void* svc) CELIX_OPTS_INIT;

    /**
     * @brief The optional setWithProperties callback is handled as the set callback, but with the addition that the
     * service properties will also be provided to the callback.
     */
    void (*setWithProperties)(void* handle,
                              void* svc,
                              const celix_properties_t* props) CELIX_OPTS_INIT; // highest ranking

    /**
     * @brief The optional setWithOwner callback is handled as the set callback, but with the addition that the service
     * properties and the bundle owning the service will also be provided to the callback.
     */
    void (*setWithOwner)(void* handle, void* svc, const celix_properties_t* props, const celix_bundle_t* svcOwner)
        CELIX_OPTS_INIT;

    /**
     * @brief The optional add callback will be called for every current and future service found conform the provided
     * service filter options as long as the tracker is active.
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of a service matching the provided service filter options.
     */
    void (*add)(void* handle, void* svc) CELIX_OPTS_INIT;

    /**
     * @brief The optional addWithProperties callback is handled as the add callback, but with the addition that the
     * service properties will also be provided to the callback.
     */
    void (*addWithProperties)(void* handle, void* svc, const celix_properties_t* props) CELIX_OPTS_INIT;

    /**
     * @brief The optional addWithOwner callback is handled as the add callback, but with the addition that the service
     * properties and the bundle owning the service will also be provided to the callback.
     */
    void (*addWithOwner)(void* handle, void* svc, const celix_properties_t* props, const celix_bundle_t* svcOwner)
        CELIX_OPTS_INIT;

    /**
     * @brief The optional remove callback will be called for every service conform the provided service filter options
     * that is unregistered. When the remove call is finished the removed services should be considered invalid. This
     * means that the callback provider should ensure that the removed service is not in use or going to be used after
     * the remove callback is finished.
     *
     * @param handle The callbackHandle pointer as provided in the service tracker options.
     * @param svc The service pointer of a service matching the provided service filter options.
     */
    void (*remove)(void* handle, void* svc) CELIX_OPTS_INIT;

    /**
     * @brief The optional removeWithProperties callback is handled as the remove callback, but with the addition that
     * the service properties will also be provided to the callback.
     */
    void (*removeWithProperties)(void* handle, void* svc, const celix_properties_t* props) CELIX_OPTS_INIT;

    /**
     * @brief The optional removeWithOwner callback is handled as the remove callback, but with the addition that the
     * service properties and the bundle owning the service will also be provided to the callback.
     */
    void (*removeWithOwner)(void* handle, void* svc, const celix_properties_t* props, const celix_bundle_t* svcOwner)
        CELIX_OPTS_INIT;

    /**
     * @brief Data for the trackerCreatedCallback.
     */
    void* trackerCreatedCallbackData CELIX_OPTS_INIT;

    /**
     * @brief The callback called when the tracker has ben created (and is active) when using a async call.
     *
     * If a asyns track service is combined with a _sync_ stop tracker, it can happen that
     * "stop tracker" happens before the "create tracker" event is processed. In this case the asyncCallback
     * will not be called.
     */
    void (*trackerCreatedCallback)(void* trackerCreatedCallbackData) CELIX_OPTS_INIT;
} celix_service_tracking_options_t;

#ifndef __cplusplus
/*!
 * @brief C Macro to create a empty celix_service_tracking_options_t type.
 */
#define CELIX_EMPTY_SERVICE_TRACKING_OPTIONS { .filter.serviceName = NULL, \
    .filter.versionRange = NULL, \
    .filter.filter = NULL, \
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
 * @brief Tracks services using the provided tracker options.
 *
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 *
 * The service tracker will be created async on the Celix event loop thread. This means that the function can return
 * before the tracker is created.
 *
 * @param ctx The bundle context.
 * @param opts The pointer to the tracker options.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT long
celix_bundleContext_trackServicesWithOptionsAsync(celix_bundle_context_t* ctx,
                                                  const celix_service_tracking_options_t* opts);

/**
 * @brief Tracks services using the provided tracker options.
 *
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * Note: Please use the celix_bundleContext_registerServiceFactoryAsync instead.
 *
 * @param ctx The bundle context.
 * @param opts The pointer to the tracker options.
 * @return the tracker id (>=0) or < 0 if unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackServicesWithOptions(celix_bundle_context_t* ctx,
                                                                         const celix_service_tracking_options_t* opts);

/**
 * @brief Use the highest-ranking service tracked by the specified tracker id by invoking the provided use callback.
 *
 * The callback is executed on the thread that invokes this function, and the function waits until the use callback
 * is completed before returning.
 *
 * A tracker id less than 0 is ignored without action. An invalid (non-existent) tracker id of 0 or greater results
 * in a logged error, and the function returns false.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The service tracker id.
 * @param[in] callbackHandle An optional pointer to a user-defined context or data structure, passed to the provided use
 * callback function.
 * @param[in] use The use callback invoked for highest-ranking service tracked.
 * @return True if a service was found and the provided use callback was executed; false if the tracker is not yet
 * active (asynchronous tracker creation), the are no matching services tracked or if the tracker id is invalid.
 */
CELIX_FRAMEWORK_EXPORT
bool celix_bundleContext_useTrackedService(celix_bundle_context_t* ctx,
                                           long trackerId,
                                           void* callbackHandle,
                                           void (*use)(void* handle, void* svc));

/**
 * @brief Use the services tracked by the specified tracker id by invoking the provided use callback.
 *
 * The callback is executed on the thread that invokes this function, and the function waits until the use callback,
 * called for all tracked. services, is completed before returning.
 *
 * A tracker id less than 0 is ignored without action. An invalid (non-existent) tracker id of 0 or greater results
 * in a logged error, and the function returns 0.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The service tracker id.
 * @param[in] callbackHandle An optional pointer to a user-defined context or data structure, passed to the provided use
 * callback function.
 * @param[in] use The use callback invoked for all tracked services.
 * @return The number of services found. Returns 0 if the tracker
 * is not yet active (asynchronous tracker creation), 0 services are found or if the tracker id is invalid.
 */
CELIX_FRAMEWORK_EXPORT
size_t celix_bundleContext_useTrackedServices(celix_bundle_context_t* ctx,
                                              long trackerId,
                                              void* callbackHandle,
                                              void (*use)(void* handle, void* svc));

/**
 * @brief Options for using services tracked by a service tracker. These options enable specifying callbacks
 * to be invoked for each tracked service that matches the selection criteria. If multiple callbacks are provided,
 * each will be called for every matching service.
 */
typedef struct celix_tracked_service_use_options {
    /**
     * @brief An optional pointer to a user-defined context or data structure, passed to all specified callback
     * functions (use, useWithProperties, and useWithOwner).
     *
     * Default value: NULL.
     */
    void* callbackHandle CELIX_OPTS_INIT;

    /**
     * @brief An optional callback invoked for each tracked service that matches the selection criteria.
     * This callback does not receive the service's properties or information about the service's owning bundle.
     *
     * The svc pointer is only valid during the callback.
     *
     * Default value: NULL.
     */
    void (*use)(void* handle, void* svc) CELIX_OPTS_INIT;

    /**
     * @brief An optional callback invoked for each tracked service that matches the selection criteria,
     * providing the service's properties. This enables the callback to utilize additional metadata associated
     * with the service.
     *
     * The svc and props pointers are only valid during the callback.
     *
     * Default value: NULL.
     */
    void (*useWithProperties)(void* handle, void* svc, const celix_properties_t* props) CELIX_OPTS_INIT;

    /**
     * @brief An optional callback invoked for each tracked service that matches the selection criteria,
     * along with the service's properties and the service's owning bundle.
     *
     * The svc, props, and svcOwner pointers are only valid during the callback.
     *
     * Default value: NULL.
     */
    void (*useWithOwner)(void* handle, void* svc, const celix_properties_t* props, const celix_bundle_t* svcOwner)
        CELIX_OPTS_INIT;
} celix_tracked_service_use_options_t;

#ifndef __cplusplus
/**
 * @brief Macro to initialize a celix_tracked_service_use_options_t structure with default values.
 */
#define CELIX_EMPTY_TRACKER_SERVICE_USE_OPTIONS                                                                        \
    { .callbackHandle = NULL, .use = NULL, .useWithProperties = NULL, .useWithOwner = NULL }
#endif

/**
 * @brief Use the highest-ranking service tracked by the specified tracker id by invoking the callbacks
 * specified in the provided options.
 *
 * The callbacks are executed on the thread that invokes this function, and the function waits until all callbacks
 * are completed before returning.
 *
 * A tracker id less than 0 is ignored without action. An invalid (non-existent) tracker id of 0 or greater results
 * in a logged error, and the function returns false.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The service tracker id.
 * @param[in] opts The service use options containing callbacks and additional configurations.
 * @return True if a service was found and the specified callbacks were executed; false if the tracker is not yet
 * active (asynchronous tracker creation), the are no matching services tracked  or if the tracker id is invalid.
 */
CELIX_FRAMEWORK_EXPORT
bool celix_bundleContext_useTrackedServiceWithOptions(celix_bundle_context_t* ctx,
                                                      long trackerId,
                                                      const celix_tracked_service_use_options_t* opts);

/**
 * @brief Use the services tracked by the specified tracker id by invoking the callbacks specified in the
 * provided options for each matching service.
 *
 * The callbacks are executed on the thread that invokes this function, and the function waits until all callbacks
 * for all found services are completed before returning.
 *
 * A tracker id less than 0 is ignored without action. An invalid (non-existent) tracker id of 0 or greater results
 * in a logged error, and the function returns 0.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The service tracker id.
 * @param[in] opts The service use options containing callbacks and additional configurations.
 * @return The number of services found and for which the specified callbacks were executed. Returns 0 if the tracker
 * is not yet active (asynchronous tracker creation), 0 services are found or if the tracker id is invalid.
 */
CELIX_FRAMEWORK_EXPORT
size_t celix_bundleContext_useTrackedServicesWithOptions(celix_bundle_context_t* ctx,
                                                         long trackerId,
                                                         const celix_tracked_service_use_options_t* opts);

/**
 * @brief Get the number of tracked services for the provided tracker id.
 *
 * Silently ignore tracker ids < 0 and invalid tracker ids.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The tracker id.
 * @return The number of tracked services or 0 if the tracker id is unknown or < 0.
 */
CELIX_FRAMEWORK_EXPORT
size_t celix_bundleContext_getTrackedServiceCount(celix_bundle_context_t *ctx, long trackerId);

/**
 * @brief Get the service name of the tracked services for the provided tracker id.
 *
 * Silently ignore tracker ids < 0 and invalid tracker ids.
 *
 * @param ctx The bundle context.
 * @param trackerId The tracker id.
 * @return The service name of the tracked services or NULL if the tracker id is unknown or < 0.
 */
CELIX_FRAMEWORK_EXPORT
const char* celix_bundleContext_getTrackedServiceName(celix_bundle_context_t *ctx, long trackerId);

/**
 * @brief Get the service filter of the tracked services for the provided tracker id.
 *
 * The returned filter is the combination of the service name and the filter from the provided service filter options.
 * For example serviceName="foo" and filter="(location=middle)" will result in a filter of
 * "(&(objectClass=foo)(location=middle))"
 *
 * Silently ignore tracker ids < 0 and invalid tracker ids.
 *
 * @param ctx The bundle context.
 * @param trackerId The tracker id.
 * @return The service filter of the tracked services or NULL if the tracker id is unknown or < 0.
 */
CELIX_FRAMEWORK_EXPORT
const char* celix_bundleContext_getTrackedServiceFilter(celix_bundle_context_t* ctx, long trackerId);

/**
 * @brief Stop the tracker with the provided track id.
 *
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
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_stopTrackerAsync(
        celix_bundle_context_t *ctx,
        long trackerId,
        void *doneCallbackData,
        void (*doneCallback)(void* doneCallbackData));

/**
 * @brief Wait, if able, for (async) creation of a tracker.
 *
 * Will silently ignore trackerId < 0 and log an error if the tracker id is unknown.
 * If called on the Apache Celix event loop thread, the function will log a warning and return immediately.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The tracker id.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_waitForAsyncTracker(celix_bundle_context_t *ctx, long trackerId);

/**
 * @brief Wait, if able, for (async) stopping of tracking.
 *
 * Will silently ignore trackerId < 0 and log an error if the tracker id is unknown.
 * If called on the Apache Celix event loop thread, the function will log a warning and return immediately.
 *
 * @param[in] ctx The bundle context.
 * @param[in] trackerId The tracker id.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_waitForAsyncStopTracker(celix_bundle_context_t *ctx, long trackerId);

/**
 * @brief Stop the tracker with the provided track id.
 *
 * Could be a service tracker, bundle tracker or service tracker tracker.
 * Only works for the trackers owned by the bundle of the bundle context.
 * Note: Please use the celix_bundleContext_stopTrackerAsync instead.
 *
 * Will log a error if the provided tracker id is unknown. Will silently ignore trackerId < 0.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_stopTracker(celix_bundle_context_t *ctx, long trackerId);

/**
 * @brief Returns true if the provided tracker id is a tracker id for an existing tracker for the provided bundle
 * context.
 * @param ctx The bundle context.
 * @param trackerId The tracker id.
 * @return True if the tracker id is valid.
 */
CELIX_FRAMEWORK_EXPORT
bool celix_bundleContext_isValidTrackerId(celix_bundle_context_t* ctx, long trackerId);

/**
 * @brief Tracker guard.
 */
typedef struct celix_tracker_guard {
    celix_bundle_context_t* ctx;
    long trackerId;
} celix_tracker_guard_t;

/**
 * @brief Initialize a scope guard for an existing bundle, service or meta tracker.
 * @param [in] ctx The bundle context associated with the service registration.
 * @param [in] trackerId The tracker id.
 * @return An initialized service registration guard.
 */
static CELIX_UNUSED inline celix_tracker_guard_t
celix_trackerGuard_init(celix_bundle_context_t* ctx, long trackerId) {
    return (celix_tracker_guard_t) { .ctx = ctx, .trackerId = trackerId };
}

/**
 * @brief De-initialize a tracker guard.
 * Will stop the tracker if the tracker id is >= 0.
 * @param [in] trackerGuard A tracker guard
 */
static CELIX_UNUSED inline void celix_trackerGuard_deinit(celix_tracker_guard_t* trackerGuard) {
    if (trackerGuard->trackerId >= 0) {
        celix_bundleContext_stopTracker(trackerGuard->ctx, trackerGuard->trackerId);
    }
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_tracker_guard_t, celix_trackerGuard_deinit)

/**
 * @brief List the installed and started bundle ids.
 * The bundle ids does not include the framework bundle (bundle id CELIX_FRAMEWORK_BUNDLE_ID).
 *
 * @param ctx The bundle context.
 * @return A array with bundle ids (long). The caller is responsible for destroying the array.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_bundleContext_listBundles(celix_bundle_context_t *ctx);

/**
 * @brief List the installed bundle ids.
 * The bundle ids does not include the framework bundle (bundle id CELIX_FRAMEWORK_BUNDLE_ID).
 *
 * @param ctx The bundle context.
 * @return A array with bundle ids (long). The caller is responsible for destroying the array.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_bundleContext_listInstalledBundles(celix_bundle_context_t *ctx);


/**
 * @brief Check whether a bundle is installed.
 * @param ctx       The bundle context.
 * @param bndId     The bundle id to check
 * @return          true if the bundle is installed.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_isBundleInstalled(celix_bundle_context_t *ctx, long bndId);

/**
 * @brief Check whether the bundle is active.
 * @param ctx       The bundle context.
 * @param bndId     The bundle id to check
 * @return          true if the bundle is installed and active.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_isBundleActive(celix_bundle_context_t *ctx, long bndId);

/**
 * @brief Install and optional start a bundle.
 * Will silently ignore bundle ids < 0.
 *
 * If this function is called on the Celix event thread and autoStart is true,
 * the actual starting of the bundle will be done async and on a separate thread.
 * If this function is called from a different thread than the Celix event thread and the autoStart is true,
 * then the function will return after the bundle is started.
 *
 * @param ctx The bundle context
 * @param bundleUrl The bundle location to the bundle zip file.
 * @param autoStart If the bundle should also be started.
 * @return the bundleId (>= 0) or < 0 if the bundle could not be installed and possibly started.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_installBundle(celix_bundle_context_t *ctx, const char* bundleUrl, bool autoStart);

/**
 * @brief Uninstall the bundle with the provided bundle id. If needed the bundle will be stopped first.
 * Will silently ignore bundle ids < 0.
 *
 * If this function is called on the Celix event thread, the actual stopping of the bundle will be done async and
 * on a separate thread.
 * If this function is called from a different thread than the Celix event thread, then the function will return after
 * the bundle is stopped.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to uninstall.
 * @return true if the bundle is correctly uninstalled. False if not.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_uninstallBundle(celix_bundle_context_t *ctx, long bndId);

/**
 * @brief Stop the bundle with the provided bundle id.
 * Will silently ignore bundle ids < 0.
 *
 * If this function is called on the Celix event thread, the actual stopping of the bundle will be done async and
 * on a separate thread.
 * If this function is called from a different thread than the Celix event thread, then the function will return after
 * the bundle is stopped.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to stop.
 * @return true if the bundle is found & correctly stop. False if not.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_stopBundle(celix_bundle_context_t *ctx, long bndId);

/**
 * @brief Start the bundle with the provided bundle id.
 * Will silently ignore bundle ids < 0.
 *
 * If this function is called on the Celix event thread, the actual starting of the bundle will be done async and
 * on a separate thread.
 * If this function is called from a different thread than the Celix event thread, then the function will return after
 * the bundle is started.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to start.
 * @return true if the bundle is found & correctly started. False if not.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_startBundle(celix_bundle_context_t *ctx, long bndId);

/**
 * @brief Update the bundle with the provided bundle id.
 *
 * This will do the following:
 *  - unload the bundle with the specified bundle id;
 *  - reload the bundle from the specified location with the specified bundle id;
 *  - start the bundle, if it was previously active.
 *
 * Will silently ignore bundle ids < 0.
 *
 * Note if specified bundle location already exists in the bundle cache but with a different bundle id, the bundle
 * will NOT be reinstalled, and the update is cancelled.
 *
 * If this function is called on the Celix event thread, the actual updating of the bundle will be done async and
 * on a separate thread.
 * If this function is called from a different thread than the Celix event thread, then the function will
 * return after the bundle update is completed.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to update.
 * @param updatedBundleUrl The optional updated bundle url to the bundle zip file.
 * If NULL, the existing bundle url from the bundle cache will be used, and the cache will only be updated if the zip file is newer.
 * @return true if the bundle is found & correctly started. False if not.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_updateBundle(celix_bundle_context_t *ctx, long bndId, const char* updatedBundleUrl);

/**
 * @brief Returns the bundle symbolic name for the provided bundle id.
 * The caller is owner of the return string.
 *
 * @param ctx The bundle context
 * @param bndId The bundle id to retrieve the symbolic name for.
 * @return The bundle symbolic name or NULL if the bundle for the provided bundle id does not exist.
 */
CELIX_FRAMEWORK_EXPORT char* celix_bundleContext_getBundleSymbolicName(celix_bundle_context_t *ctx, long bndId);

/**
 * @brief Track bundles.
 *
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackBundlesAsync(
        celix_bundle_context_t *ctx,
        void* callbackHandle,
        void (*onStarted)(void* handle, const celix_bundle_t *bundle),
        void (*onStopped)(void *handle, const celix_bundle_t *bundle)
);

/**
 * @brief Track bundles.
 *
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackBundles(
        celix_bundle_context_t *ctx,
        void* callbackHandle,
        void (*onStarted)(void* handle, const celix_bundle_t *bundle),
        void (*onStopped)(void *handle, const celix_bundle_t *bundle)
);

/**
 * @brief The Service Bundle Tracking options can be used to fine tune the requested bundle tracker options.
 */
typedef struct celix_bundle_tracker_options {
    /**
     * @brief The optional callback pointer used in all the provided callback function (set, add, remove, setWithProperties, etc).
     */
    void* callbackHandle CELIX_OPTS_INIT;

    /**
     * @brief Tracker callback when a bundle is installed.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been installed.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onInstalled)(void *handle, const celix_bundle_t *bundle) CELIX_OPTS_INIT;

    /**
     * @brief Tracker callback when a bundle is started.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been started.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onStarted)(void *handle, const celix_bundle_t *bundle) CELIX_OPTS_INIT;

    /**
     * @brief Tracker callback when a bundle is stopped.
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param bundle    The bundle which has been stopped.
     *                  The bundle pointer is only guaranteed to be valid during the callback.
     */
    void (*onStopped)(void *handle, const celix_bundle_t *bundle) CELIX_OPTS_INIT;

    /**
     *
     * @param handle    The handle, contains the value of the callbackHandle.
     * @param event     The bundle event. Is only valid during the callback.
     */
    void (*onBundleEvent)(void *handle, const celix_bundle_event_t *event) CELIX_OPTS_INIT;

    /**
     * @brief Default the framework bundle (bundle id 0) will not trigger the callbacks.
     * This is done, because the framework bundle is a special bundle which is generally not needed in the callbacks.
     */
    bool includeFrameworkBundle CELIX_OPTS_INIT;

    /**
     * @brief Data for the trackerCreatedCallback.
     */
    void *trackerCreatedCallbackData CELIX_OPTS_INIT;

    /**
     * @brief The callback called when the tracker has ben created (and is active) when using the
     * track bundles ascync calls.
     *
     * If a asyns track service is combined with a _sync_ stop tracker, it can happen that
     * "stop tracker" happens before the "create tracker" event is processed. In this case the asyncCallback
     * will not be called.
     */
    void (*trackerCreatedCallback)(void *trackerCreatedCallbackData) CELIX_OPTS_INIT;
} celix_bundle_tracking_options_t;

#ifndef __cplusplus
/*!
 * @brief C Macro to create a empty celix_service_filter_options_t type.
 */
#define CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS {.callbackHandle = NULL, .onInstalled = NULL, .onStarted = NULL, .onStopped = NULL, .onBundleEvent = NULL, .includeFrameworkBundle = false, .trackerCreatedCallbackData = NULL, .trackerCreatedCallback = NULL}
#endif

/**
 * @brief Tracks bundles using the provided bundle tracker options.
 *
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
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackBundlesWithOptionsAsync(
        celix_bundle_context_t *ctx,
        const celix_bundle_tracking_options_t *opts
);

/**
 * @brief Tracks bundles using the provided bundle tracker options.
 *
 * The tracker options are only using during this call and can safely be freed/reused after this call returns.
 * (i.e. can be on the stack)
 *
 * Note: please use celix_bundleContext_trackBundlesWithOptionsAsync instead;
 *
 * @param ctx   The bundle context.
 * @param opts  The pointer to the bundle tracker options.
 * @return      The bundle tracker id (>=0) or < 0 if unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackBundlesWithOptions(
        celix_bundle_context_t *ctx,
        const celix_bundle_tracking_options_t *opts
);

/**
 * @brief Use the bundle with the provided bundle id.
 *
 * The provided callback will be called if the bundle is found (installed).
 * Call with CELIX_FRAMEWORK_BUNDLE_ID as bundleId to use the framework bundle.
 *
 * @param ctx               The bundle context.
 * @param bundleId          The bundle id.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 * @return                  Returns true if the bundle is found and the callback is called.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_useBundle(
        celix_bundle_context_t *ctx,
        long bundleId,
        void *callbackHandle,
        void (*use)(void *handle, const celix_bundle_t *bundle));

/**
 * @brief Use the currently installed bundles.
 *
 * The provided callback will be called for all the currently installed bundles, expect the framework bundle.
 *
 * @param ctx               The bundle context.
 * @param callbackHandle    The data pointer, which will be used in the callbacks
 * @param use               The callback which will be called for the currently started bundles.
 *                          The bundle pointers are only guaranteed to be valid during the callback.
 * @return                  The number of times the use callback is called (nr of installed bundles).
 */
CELIX_FRAMEWORK_EXPORT size_t celix_bundleContext_useBundles(
        celix_bundle_context_t *ctx,
        void *callbackHandle,
        void (*use)(void *handle, const celix_bundle_t *bundle));

/**
 * @brief Service Tracker Info provided to the service tracker tracker callbacks.
 */
typedef struct celix_service_tracker_info {
    /**
     * @brief The parsed service filter, e.g. parsed "(&(objectClass=example_calc)(meta.info=foo))"
     */
    celix_filter_t *filter;

    /**
     * @brief The service name filter attribute parsed from the service filter (i.e. the value of the objectClass attribute key)
     */
    const char* serviceName;

    /**
     * @brief Bundle id of the owner of the service tracker.
     */
    long bundleId;
} celix_service_tracker_info_t;

/**
 * @brief Track the service tracker targeting the provided service name.
 *
 * This can be used to track if there is an interest in a certain service and ad-hoc act on that interest.
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
 *                          If a asyns track service is combined with a _sync_ stop tracker, it can happen that
 *                          "stop tracker" happens before the "create tracker" event is processed.
 *                          In this case the doneCallback will not be called.
 * @return The tracker id or <0 if something went wrong (will log an error).
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackServiceTrackersAsync(
        celix_bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info),
        void *doneCallbackData,
        void (*doneCallback)(void* doneCallbackData));

/**
 * @brief Track the service tracker targeting the provided service name.
 *
 * This can be used to track if there is an interest in a certain service and ad-hoc act on that interest.
 *
 * Note that the celix_service_tracker_info_t pointer in the trackerAdd/trackerRemove callbacks are only valid during
 * the callback.
 *
 * Note: Please use celix_bundleContext_trackServiceTrackersAsync instead.
 *
 * This tracker can be stopped with the celix_bundleContext_stopTracker function.
 *
 * @param ctx The bundle context.
 * @param serviceName The target service name for the service tracker to track.
 *                      If NULL is provided, add/remove callbacks will be called for all service trackers in the framework.
 * @param callbackHandle The callback handle which will be provided as handle in the trackerAdd and trackerRemove callback.
 * @param trackerAdd Called when a service tracker is added, which tracks the provided service name. Will also be called
 *                   for all existing service tracker when this tracker is started.
 * @param trackerRemove Called when a service tracker is removed, which tracks the provided service name
 * @return The tracker id or <0 if something went wrong (will log an error).
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_trackServiceTrackers(
        celix_bundle_context_t *ctx,
        const char* serviceName,
        void *callbackHandle,
        void (*trackerAdd)(void *handle, const celix_service_tracker_info_t *info),
        void (*trackerRemove)(void *handle, const celix_service_tracker_info_t *info));

/**
 * @brief Gets the dependency manager for this bundle context.
 *
 * @return the dependency manager or NULL if unsuccessful.
 */
CELIX_FRAMEWORK_EXPORT celix_dependency_manager_t* celix_bundleContext_getDependencyManager(celix_bundle_context_t *ctx);


/**
 * @brief Wait until all Celix event for this bundle are completed.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_waitForEvents(celix_bundle_context_t *ctx);

/**
 * @struct celix_scheduled_event_options
 * @brief Celix scheduled event options, used for creating scheduling events with the celix framework.
 */
typedef struct celix_scheduled_event_options {
    const char* name CELIX_OPTS_INIT; /**<
                                       * @brief The name of the event, used for logging and debugging.
                                       *
                                       * Expected to be const char* that is valid during the celix_bundleContext_scheduleEvent
                                       * call. Can be NULL. */

    double initialDelayInSeconds CELIX_OPTS_INIT; /**< @brief Initial delay in seconds before the event is processed.*/

    double intervalInSeconds CELIX_OPTS_INIT; /**< @brief Schedule interval in seconds.
                                               *  0 means one-shot scheduled event.
                                               */

    void* callbackData CELIX_OPTS_INIT; /**< @brief Data passed to the callback function when a event is scheduled.*/

    void (*callback)(void* callbackData) CELIX_OPTS_INIT; /**< @brief Callback function called to process a scheduled
                                                             event. Will be called on the event thread.*/

    void* removeCallbackData
        CELIX_OPTS_INIT; /**< @brief Data passed to the done callback function when a scheduled event is removed.*/

    void (*removeCallback)(void* removeCallbackData)
        CELIX_OPTS_INIT; /**< @brief Callback function called when a scheduled event is removed. Will be called on
                                     the event thread.*/
} celix_scheduled_event_options_t;

#define CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS {NULL, 0.0, 0.0, NULL, NULL, NULL, NULL}

/**
 * @brief Add a scheduled event to the Celix framework.
 *
 * The scheduled event will be called on the Celix framework event thread, repeatedly using the provided interval or
 * once if only a initial delay is provided.
 * The event callback should be relatively fast and the scheduled event interval should be relatively long, otherwise
 * the framework event queue will be blocked and framework will not function properly.
 *
 * Scheduled events can be scheduled later than the provided initial delay and interval, because they are processed
 * after other events in the Celix event thread.
 * The target - but not guaranteed - precision of the scheduled event trigger is 1 microsecond.
 *
 * If the provided interval is 0, the scheduled event will a one-shot scheduled event and will be called once
 * after the provided initial delay. If a bundle stops before the one-shot scheduled event is called, the scheduled
 * event will be removed and not called.
 *
 * Scheduled events should be removed by the caller when not needed anymore, except for one-shot scheduled events.
 * one-shot are automatically removed after the event callback is called.
 *
 * Note during bundle stop the framework will check if all scheduled events for the bundle are removed.
 * For every not removed scheduled event that is not a one-shot event, a warning will be logged and the
 * scheduled event will be removed.
 *
 * @param[in] ctx The bundle context.
 * @param[in] options The scheduled event options, which describe the to be added scheduled event.
 * @return The scheduled event id of the scheduled event. Can be used to cancel the event.
 * @retval <0 If the event could not be added.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_scheduleEvent(celix_bundle_context_t* ctx,
                                                              const celix_scheduled_event_options_t* options);

/**
 * @brief Wakeup a scheduled event and returns immediately, not waiting for the scheduled event callback to be
 * called.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * @param[in] ctx The bundle context.
 * @param[in] scheduledEventId The scheduled event id to wakeup.
 * @return CELIX_SUCCESS if the scheduled event is woken up, CELIX_ILLEGAL_ARGUMENT if the scheduled event id is not known.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_bundleContext_wakeupScheduledEvent(celix_bundle_context_t* ctx,
                                                                               long scheduledEventId);

/**
 * @brief Wait until the next scheduled event is processed.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * @param[in] ctx The bundle context.
 * @param[in] scheduledEventId The scheduled event id to wait for.
 * @param[in] waitTimeInSeconds The maximum time to wait for the next scheduled event. If <= 0 the function will return
 *                             immediately.
 * @return CELIX_SUCCESS if the scheduled event is woken up, CELIX_ILLEGAL_ARGUMENT if the scheduled event id is not
 *         known and ETIMEDOUT if the waitTimeInSeconds is reached.
 */
CELIX_FRAMEWORK_EXPORT celix_status_t celix_bundleContext_waitForScheduledEvent(celix_bundle_context_t* ctx,
                                                                                long scheduledEventId,
                                                                                double waitTimeInSeconds);

/**
 * @brief Cancel and remove a scheduled event.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * This function will block until a possible in-progress scheduled event callback is finished, the scheduled event
 * is removed and, if configured, the remove callback is called.
 *
 * @param[in] ctx The bundle context.
 * @param[in] scheduledEventId The scheduled event id to cancel and remove.
 * @return true if a scheduled event is cancelled, false if the scheduled event id is not known.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_removeScheduledEvent(celix_bundle_context_t* ctx,
                                                                      long scheduledEventId);

/**
 * @brief Cancel and remove a scheduled event asynchronously.
 *
 * When this function returns, no new scheduled event callbacks will be called, but it is not guaranteed that there
 * is still a scheduled event callback in progress and that the remove callback is called.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * @param[in] ctx The bundle context.
 * @param[in] scheduledEventId The scheduled event id to cancel and remove.
 * @return true if a scheduled event is cancelled, false if the scheduled event id is not known.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_removeScheduledEventAsync(celix_bundle_context_t* ctx,
                                                                     long scheduledEventId);

/**
 * @brief Try to cancel and remove a scheduled event asynchronously.
 *
 * Silently ignored if the scheduled event ids < 0.
 *
 * When this function returns, no new scheduled event callbacks will be called, but it is not guaranteed that there
 * is still a scheduled event callback in progress and that the remove callback is called.
 * Will not log an error if the scheduled event id is not known.
 *
 * @param[in] ctx The bundle context.
 * @param[in] scheduledEventId The scheduled event id to cancel.
 * @return true if a scheduled event is cancelled, false if the scheduled event id is not known.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_tryRemoveScheduledEventAsync(celix_bundle_context_t* ctx,
                                                                        long scheduledEventId);

/**
 * @brief Returns the bundle for this bundle context.
 */
CELIX_FRAMEWORK_EXPORT celix_bundle_t* celix_bundleContext_getBundle(const celix_bundle_context_t *ctx);


/**
 * @brief Returns the bundle if for the bundle of this bundle context.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_getBundleId(const celix_bundle_context_t *ctx);

CELIX_FRAMEWORK_EXPORT celix_framework_t* celix_bundleContext_getFramework(const celix_bundle_context_t *ctx);

/**
 * @brief Logs a message to Celix framework logger with the provided log level.
 * @param ctx       The bundle context
 * @param level     The log level to use
 * @param format    printf style format string
 * @param ...       printf style format arguments
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_log(
        const celix_bundle_context_t *ctx,
        celix_log_level_e level,
        const char* format,
        ...) __attribute__((format(printf,3,4)));

/**
 * @brief Logs a message to Celix framework logger with the provided log level.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_vlog(
        const celix_bundle_context_t *ctx,
        celix_log_level_e level,
        const char* format,
        va_list formatArgs) __attribute__((format(printf,3,0)));

/**
 *
 * @brief Logs celix thread-specific storage error messages(celix_err) ith the provided celix log level.
 * Silently ignores log level CELIX_LOG_LEVEL_DISABLED.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundleContext_logTssErrors(const celix_bundle_context_t* ctx,
                                                             celix_log_level_e level);

/**
 * @brief Get the config property for the given key.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param ctx The bundle context.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @return The property value or the default value if the property is not found.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_bundleContext_getProperty(celix_bundle_context_t *ctx, const char* key, const char* defaultVal);

/**
 * @brief Get the config property for the given key converted as long value.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @return The property value or the default value if the property is not found or the property value cannot be converted
 *         to a long value.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundleContext_getPropertyAsLong(celix_bundle_context_t *ctx, const char* name, long defaultValue);

/**
 * @brief Get the config property for the given key converted as double value.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @return The property value or the default value if the property is not found or the property value cannot be converted
 *         to a double value.
 */
CELIX_FRAMEWORK_EXPORT double celix_bundleContext_getPropertyAsDouble(celix_bundle_context_t *ctx, const char* name, double defaultValue);

/**
 * @brief Get the config property for the given key converted as bool value.
 *
 * The config property is a property from the framework configuration or a system property.
 * If a system property is found, the system property is returned.
 * Otherwise the framework configuration property - if found - is returned.
 *
 * @param framework The framework.
 * @param name The name of the property.
 * @param defaultValue The default value if the property is not found.
 * @return The property value or the default value if the property is not found or the property value cannot be converted
 *         to a bool value.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundleContext_getPropertyAsBool(celix_bundle_context_t *ctx, const char* name, bool defaultValue);

#undef CELIX_OPTS_INIT

#ifdef __cplusplus
}
#endif

#endif //CELIX_BUNDLE_CONTEXT_H_
