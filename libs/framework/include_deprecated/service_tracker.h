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

#ifndef SERVICE_TRACKER_H_
#define SERVICE_TRACKER_H_

#include "celix_service_listener.h"
#include "service_tracker_customizer.h"
#include "bundle_context.h"
#include "celix_bundle_context.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_serviceTracker celix_service_tracker_t;

typedef struct celix_serviceTracker *service_tracker_pt;
typedef struct celix_serviceTracker service_tracker_t;

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceTracker_create(celix_bundle_context_t *ctx, const char *service, service_tracker_customizer_pt customizer,
                      service_tracker_t **tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t
serviceTracker_createWithFilter(celix_bundle_context_t *ctx, const char *filter, service_tracker_customizer_pt customizer,
                                service_tracker_t **tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceTracker_open(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceTracker_close(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t serviceTracker_destroy(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT service_reference_pt serviceTracker_getServiceReference(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *serviceTracker_getServiceReferences(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void *serviceTracker_getService(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_array_list_t *serviceTracker_getServices(service_tracker_t *tracker);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void *serviceTracker_getServiceByReference(service_tracker_t *tracker, service_reference_pt reference);

CELIX_FRAMEWORK_DEPRECATED_EXPORT size_t serviceTracker_nrOfTrackedServices(service_tracker_t *tracker);




/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API. Note API is still deprecated for external usage, the api will become private in the future.
 **********************************************************************************************************************
 **********************************************************************************************************************/

/**
 * Creates and starts (open) a service tracker.
 * Note that is different from the serviceTracker_create function, because is also starts the service tracker
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_service_tracker_t* celix_serviceTracker_create(
        celix_bundle_context_t *ctx,
        const char *serviceName,
        const char *versionRange,
        const char *filter

);

/**
 * Creates a closed service tracker.
 * It is similar to serviceTracker_create.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_service_tracker_t* celix_serviceTracker_createClosedWithOptions(
    celix_bundle_context_t *ctx,
    const celix_service_tracking_options_t *opts
);

/**
 * Creates and starts (open) a service tracker.
 * Note that is different from the serviceTracker_create function, because is also starts the service tracker
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_service_tracker_t* celix_serviceTracker_createWithOptions(
        celix_bundle_context_t *ctx,
        const celix_service_tracking_options_t *opts
);


/**
 * Stops (close) and destroys a service tracker.
 * Note that is different from the serviceTracker_destroy function, because is also stops the service tracker
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT void celix_serviceTracker_destroy(celix_service_tracker_t *tracker);

/**
 * Use the highest ranking service of the service tracker.
 * If a serviceName is provided this will also be checked.
 * No match -> no call to use.
 *
 * If waitForSvcTimeoutInSec is > 0. The call will block until a service is found or the timeout is expired.
 * (with a precision of roughly 1 microsecond)
 *
 * @return bool     if the service if found and use has been called.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT bool celix_serviceTracker_useHighestRankingService(
        celix_service_tracker_t *tracker,
        const char *serviceName /*sanity*/,
        double waitTimeoutInSeconds /*0 -> do not wait */,
        void *callbackHandle,
        void (*use)(void *handle, void *svc),
        void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
        void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)
);

/**
 * Calls the use callback for every services found by this tracker.
 * Returns the number of called services
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT size_t celix_serviceTracker_useServices(
        service_tracker_t *tracker,
        const char* serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc),
        void (*useWithProperties)(void *handle, void *svc, const celix_properties_t *props),
        void (*useWithOwner)(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner)
);

/**
 * @brief Get the number of tracked services.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT size_t celix_serviceTracker_getTrackedServiceCount(celix_service_tracker_t* tracker);

/**
 * @brief Get the tracker service name.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT const char*
celix_serviceTracker_getTrackedServiceName(celix_service_tracker_t* tracker);

/**
 * @brief Get the tracker filter.
 */
CELIX_FRAMEWORK_DEPRECATED_EXPORT const char*
celix_serviceTracker_getTrackedServiceFilter(celix_service_tracker_t* tracker);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_TRACKER_H_ */
