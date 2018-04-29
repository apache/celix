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
/*
 * service_tracker.h
 *
 *  \date       Apr 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_TRACKER_H_
#define SERVICE_TRACKER_H_

#include "service_listener.h"
#include "array_list.h"
#include "service_tracker_customizer.h"
#include "framework_exports.h"
#include "bundle_context.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_serviceTracker celix_service_tracker_t;

typedef struct celix_serviceTracker *service_tracker_pt;
typedef struct celix_serviceTracker service_tracker_t;

FRAMEWORK_EXPORT celix_status_t
serviceTracker_create(bundle_context_t *ctx, const char *service, service_tracker_customizer_pt customizer,
                      service_tracker_t **tracker);

FRAMEWORK_EXPORT celix_status_t
serviceTracker_createWithFilter(bundle_context_t *ctx, const char *filter, service_tracker_customizer_pt customizer,
                                service_tracker_t **tracker);

FRAMEWORK_EXPORT celix_status_t serviceTracker_open(service_tracker_t *tracker);

FRAMEWORK_EXPORT celix_status_t serviceTracker_close(service_tracker_t *tracker);

FRAMEWORK_EXPORT celix_status_t serviceTracker_destroy(service_tracker_t *tracker);

FRAMEWORK_EXPORT service_reference_pt serviceTracker_getServiceReference(service_tracker_t *tracker);

FRAMEWORK_EXPORT array_list_pt serviceTracker_getServiceReferences(service_tracker_t *tracker);

FRAMEWORK_EXPORT void *serviceTracker_getService(service_tracker_t *tracker);

FRAMEWORK_EXPORT array_list_pt serviceTracker_getServices(service_tracker_t *tracker);

FRAMEWORK_EXPORT void *serviceTracker_getServiceByReference(service_tracker_t *tracker, service_reference_pt reference);

FRAMEWORK_EXPORT void serviceTracker_serviceChanged(service_listener_pt listener, service_event_pt event);



/**********************************************************************************************************************
 **********************************************************************************************************************
 * Updated API
 **********************************************************************************************************************
 **********************************************************************************************************************/

typedef struct celix_service_tracker_options {
    const char *serviceName; //required serviceName
    const char *versionRange; //optional service range (e.g. "[1,2)" or "[1.1.0,1.2.0)")
    const char *filter;

    void *callbackHandle;

    //TODO adding for interceptors??
    void (*add)(void *handle, void *svc, const properties_t *svcProps, const bundle_t *svcOwner);
    void (*remove)(void *handle, void *svc, const properties_t *svcProps, const bundle_t *svcOwner);
    void (*modified)(void *handle, void *svc, const properties_t *svcProps, const bundle_t *svcOwner);
} celix_service_tracker_options_t;

celix_service_tracker_t* celix_serviceTracker_create(
        bundle_context_t *ctx,
        const char *serviceName,
        const char *versionRange,
        const char *filter

);

celix_service_tracker_t* celix_serviceTracker_createWithOptions(
        bundle_context_t *ctx,
        const celix_service_tracker_options_t *opts
);



/**
 * Use the highest ranking service of the service tracker.
 * If a serviceName is provided this will also be checked.
 * No match -> no call to use.
 *
 * @return bool     if the service if found and use has been called.
 */
bool celix_serviceTracker_useHighestRankingService(
        celix_service_tracker_t *tracker,
        const char *serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc, const properties_t *props, const bundle_t *owner)
);


/**
 * Calls the use callback for every services found by this tracker.
 */
void celix_serviceTracker_useServices(
        service_tracker_t *tracker,
        const char* serviceName /*sanity*/,
        void *callbackHandle,
        void (*use)(void *handle, void *svc, const properties_t *props, const bundle_t *owner)
);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_TRACKER_H_ */
