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

#ifndef SERVICE_TRACKER_PRIVATE_H_
#define SERVICE_TRACKER_PRIVATE_H_

#include "service_tracker.h"
#include "celix_types.h"

enum celix_service_tracker_state {
    CELIX_SERVICE_TRACKER_OPENING,
    CELIX_SERVICE_TRACKER_OPEN,
    CELIX_SERVICE_TRACKER_CLOSING,
    CELIX_SERVICE_TRACKER_CLOSED
};

struct celix_serviceTracker {
    //const after init
    bundle_context_t* context;

    char* filter;
    char* serviceName;

    service_tracker_customizer_t customizer;
    celix_service_listener_t listener;

    void* callbackHandle;

    void (*set)(void* handle, void* svc); // highest ranking
    void (*add)(void* handle, void* svc);
    void (*remove)(void* handle, void* svc);
    void (*modified)(void* handle, void* svc);

    void (*setWithProperties)(void* handle, void* svc, const celix_properties_t* props); // highest ranking
    void (*addWithProperties)(void* handle, void* svc, const celix_properties_t* props);
    void (*removeWithProperties)(void* handle, void* svc, const celix_properties_t* props);
    void (*modifiedWithProperties)(void* handle, void* svc, const celix_properties_t* props);

    void (*setWithOwner)(void* handle,
                         void* svc,
                         const celix_properties_t* props,
                         const bundle_t* owner); // highest ranking
    void (*addWithOwner)(void* handle, void* svc, const celix_properties_t* props, const bundle_t* owner);
    void (*removeWithOwner)(void* handle, void* svc, const celix_properties_t* props, const bundle_t* owner);
    void (*modifiedWithOwner)(void* handle, void* svc, const celix_properties_t* props, const bundle_t* owner);
    //end const after init

    struct {
        celix_thread_mutex_t mutex; // projects below
        celix_thread_cond_t cond;
        bool closing;       // if closing is true, no new service registrations are added to the service tracker
        size_t activeCalls; // >0 if there is still a serviceChange for a service registration call active
    } closeSync;

    struct {
        celix_thread_mutex_t mutex; // projects below
        celix_thread_cond_t condTracked;
        celix_thread_cond_t condUntracking;
        celix_array_list_t* trackedServices;
        size_t untrackedServiceCount;
        enum celix_service_tracker_state lifecycleState;
        long currentHighestServiceId;
    } state;
};

typedef struct celix_tracked_entry {
	service_reference_pt reference;
	void *service;
    long serviceId; //cached service.id of the service
    long serviceRanking; //cached service.ranking of the service
	const char *serviceName;
	celix_properties_t *properties;
	bundle_t *serviceOwner;

    celix_thread_mutex_t mutex; //protects useCount
	celix_thread_cond_t useCond;
    size_t useCount;
} celix_tracked_entry_t;


#endif /* SERVICE_TRACKER_PRIVATE_H_ */
