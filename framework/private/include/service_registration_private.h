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
 * service_registration_private.h
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_REGISTRATION_PRIVATE_H_
#define SERVICE_REGISTRATION_PRIVATE_H_

#include "service_registration.h"

typedef struct registry_callback_struct {
	void *handle;
	celix_status_t (*unregister)(void *handle, bundle_pt bundle, service_registration_pt reg);
	celix_status_t (*modified)(void *handle, service_registration_pt registration, properties_pt oldProperties);
} registry_callback_t;

struct serviceRegistration {
    registry_callback_t callback;

	char * className;
	bundle_pt bundle;
	properties_pt properties;
	void * svcObj;
	long serviceId;

	bool isUnregistering;

	bool isServiceFactory;
	void *serviceFactory;

	struct service *services;
	int nrOfServices;

	size_t refCount; //protected by mutex

	celix_thread_rwlock_t lock;
};

service_registration_pt serviceRegistration_create(registry_callback_t callback, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary);
service_registration_pt serviceRegistration_createServiceFactory(registry_callback_t callback, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary);

void serviceRegistration_retain(service_registration_pt registration);
void serviceRegistration_release(service_registration_pt registration);

bool serviceRegistration_isValid(service_registration_pt registration);
void serviceRegistration_invalidate(service_registration_pt registration);

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, void **service);
celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, void **service);

celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle);
celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, char **serviceName);

#endif /* SERVICE_REGISTRATION_PRIVATE_H_ */
