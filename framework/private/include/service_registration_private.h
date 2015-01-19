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
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#ifndef SERVICE_REGISTRATION_PRIVATE_H_
#define SERVICE_REGISTRATION_PRIVATE_H_

#include "service_registration.h"

struct service {
	char *name;
	void *serviceStruct;
};

struct serviceRegistration {
	service_registry_pt registry;
	char * className;
	bundle_pt bundle;
	properties_pt properties;
	void * svcObj;
	long serviceId;

	celix_thread_mutex_t mutex;
	bool isUnregistering;

	bool isServiceFactory;
	void *serviceFactory;

	struct service *services;
	int nrOfServices;
};

service_registration_pt serviceRegistration_create(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary);
service_registration_pt serviceRegistration_createServiceFactory(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary);
celix_status_t serviceRegistration_destroy(service_registration_pt registration);

bool serviceRegistration_isValid(service_registration_pt registration);
void serviceRegistration_invalidate(service_registration_pt registration);

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, void **service);
celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, void **service);
celix_status_t serviceRegistration_getProperties(service_registration_pt registration, properties_pt *properties);
celix_status_t serviceRegistration_getRegistry(service_registration_pt registration, service_registry_pt *registry);
celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle);
celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, char **serviceName);

#endif /* SERVICE_REGISTRATION_PRIVATE_H_ */
