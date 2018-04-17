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

typedef struct serviceTracker *service_tracker_pt;
typedef struct serviceTracker service_tracker_t;

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

/**
 * Locks and get the highest ranking service. This call needs to be followed with the unlockAndUngetService call
 *
 * @param tracker The service tracker
 * @param svcPropsOut The ptr to ptr of the properties output, can be NULL.
 * @param ownerOut The ptr to ptr of the bundle servic owner output, can be NULL.
 * @return the svc if found or NULL.
 */
FRAMEWORK_EXPORT void* serviceTracker_lockAndGetService(service_tracker_t *tracker, properties_t **svcPropsOut, bundle_t **ownerOut);

/**
 * Unlocks and unget the requested service.
 * @param tracker The service tracker
 * @param svc The svc for which a lock is held
 */
FRAMEWORK_EXPORT void serviceTracker_unlockAndUngetService(service_tracker_t *tracker, void *svc);


#ifdef __cplusplus
}
#endif

#endif /* SERVICE_TRACKER_H_ */
