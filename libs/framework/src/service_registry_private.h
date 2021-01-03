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
/**
 * service_registry_private.h
 *
 *  \date       Feb 7, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef SERVICE_REGISTRY_PRIVATE_H_
#define SERVICE_REGISTRY_PRIVATE_H_

#include "registry_callback_private.h"
#include "service_registry.h"
#include "listener_hook_service.h"
#include "service_reference.h"

#define CELIX_SERVICE_REGISTRY_STATIC_EVENT_QUEUE_SIZE  64

typedef struct celix_service_registry_event {
    //TODO call from framework to ensure bundle entries usage count is increased
    bool isRegistrationEvent;

    //for register event
    long serviceId;
    char *serviceName;
    void *svc;
    celix_service_factory_t* factory;
    celix_properties_t* properties;
    void* registerData;
    void (*registerCallback)(void *data, service_registration_t*);

    //for unregister event
    service_registration_t* registration;
    void* unregisterData;
    void (*unregisterCallback)(void *data);
} celix_service_registry_event_t;

struct celix_serviceRegistry {
	framework_pt framework;
	registry_callback_t callback;

    celix_thread_rwlock_t lock; //protect below

	hash_map_t *serviceRegistrations; //key = bundle (reg owner), value = list ( registration )
	hash_map_t *serviceReferences; //key = bundle, value = map (key = serviceId, value = reference)

	long nextServiceId;

	celix_array_list_t *listenerHooks; //celix_service_registry_listener_hook_entry_t*
	celix_array_list_t *serviceListeners; //celix_service_registry_service_listener_entry_t*

	/**
	 * The pending register events are introduced to ensure UNREGISTERING events are always
	 * after REGISTERED events in service listeners.
	 * When a listener is registered and it retroactively triggers registered events, it will also
	 * increase the pending registered events for the matching services.
	 * Also when a new service is registered all matching services listeners are called.
	 * This is used to ensuring unregistering events can only be triggered after all registered
	 * events - for a given service - are triggered.
	 */
	struct {
	    celix_thread_mutex_t mutex;
	    celix_thread_cond_t cond;
	    hash_map_t *map; //key = svc id, value = long (nr of pending register events)
	} pendingRegisterEvents;
};

typedef struct celix_service_registry_listener_hook_entry {
    long svcId;
    celix_listener_hook_service_t *hook;
    celix_thread_mutex_t mutex; //protects below
    celix_thread_cond_t cond;
    unsigned int useCount;
} celix_service_registry_listener_hook_entry_t;

typedef struct celix_service_registry_service_listener_entry {
    celix_bundle_t *bundle;
    celix_filter_t *filter;
    celix_service_listener_t *listener;
    celix_thread_mutex_t mutex; //protects below
    celix_thread_cond_t cond;
    unsigned int useCount;
} celix_service_registry_service_listener_entry_t;

struct usageCount {
	unsigned int count;
	service_reference_pt reference;
	void * service;
	service_registration_pt registration;
};

typedef struct usageCount * usage_count_pt;

#endif /* SERVICE_REGISTRY_PRIVATE_H_ */
