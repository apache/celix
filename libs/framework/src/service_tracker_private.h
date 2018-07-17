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
 * service_tracker_private.h
 *
 *  \date       Feb 6, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_TRACKER_PRIVATE_H_
#define SERVICE_TRACKER_PRIVATE_H_

#include "service_tracker.h"
#include "celix_types.h"

//instance for an active per open statement and removed per close statement
typedef struct celix_service_tracker_instance {
	celix_thread_mutex_t closingLock; //projects closing and activeServiceChangeCalls
	bool closing; //when true the service tracker instance is being closed and all calls from the service listener are ignored
	size_t activeServiceChangeCalls;
	celix_thread_cond_t activeServiceChangeCallsCond;

	bundle_context_t *context;
	char * filter;

	service_tracker_customizer_t customizer;
	celix_service_listener_t listener;

	void *callbackHandle;

	void (*set)(void *handle, void *svc); //highest ranking
	void (*add)(void *handle, void *svc);
	void (*remove)(void *handle, void *svc);
	void (*modified)(void *handle, void *svc);

	void (*setWithProperties)(void *handle, void *svc, const properties_t *props); //highest ranking
	void (*addWithProperties)(void *handle, void *svc, const properties_t *props);
	void (*removeWithProperties)(void *handle, void *svc, const properties_t *props);
	void (*modifiedWithProperties)(void *handle, void *svc, const properties_t *props);

	void (*setWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner); //highest ranking
	void (*addWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner);
	void (*removeWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner);
	void (*modifiedWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner);

	celix_thread_rwlock_t lock; //projects trackedServices
	array_list_t *trackedServices;

	celix_thread_mutex_t mutex; //protect current highest service id
	long currentHighestServiceId;

	celix_thread_t shutdownThread; //will be created when this instance is shutdown
} celix_service_tracker_instance_t;

struct celix_serviceTracker {
	bundle_context_t *context;

	char * filter;
	service_tracker_customizer_t *customizer;

	void *callbackHandle;

	void (*set)(void *handle, void *svc); //highest ranking
	void (*add)(void *handle, void *svc);
	void (*remove)(void *handle, void *svc);
	void (*modified)(void *handle, void *svc);

	void (*setWithProperties)(void *handle, void *svc, const properties_t *props); //highest ranking
	void (*addWithProperties)(void *handle, void *svc, const properties_t *props);
	void (*removeWithProperties)(void *handle, void *svc, const properties_t *props);
	void (*modifiedWithProperties)(void *handle, void *svc, const properties_t *props);

	void (*setWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner); //highest ranking
	void (*addWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner);
	void (*removeWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner);
	void (*modifiedWithOwner)(void *handle, void *svc, const properties_t *props, const bundle_t *owner);

	celix_thread_rwlock_t instanceLock;
	celix_service_tracker_instance_t *instance; /*NULL -> close, !NULL->open*/

};

typedef struct celix_tracked_entry {
	service_reference_pt reference;
	void *service;
	const char *serviceName;
	properties_t *properties;
	bundle_t *serviceOwner;

    celix_thread_mutex_t mutex; //protects useCount
	celix_thread_cond_t useCond;
    size_t useCount;
} celix_tracked_entry_t;


#endif /* SERVICE_TRACKER_PRIVATE_H_ */
