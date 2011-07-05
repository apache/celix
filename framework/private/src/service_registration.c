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
 * service_registration.c
 *
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service_registration.h"
#include "constants.h"
#include "service_factory.h"

celix_status_t serviceRegistration_createInternal(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId,
        void * serviceObject, PROPERTIES dictionary, bool isFactory, SERVICE_REGISTRATION *registration);

SERVICE_REGISTRATION serviceRegistration_create(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary) {
    SERVICE_REGISTRATION registration = NULL;
	serviceRegistration_createInternal(registry, bundle, serviceName, serviceId, serviceObject, dictionary, false, &registration);
	return registration;
}

SERVICE_REGISTRATION serviceRegistration_createServiceFactory(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary) {
    SERVICE_REGISTRATION registration = NULL;
    serviceRegistration_createInternal(registry, bundle, serviceName, serviceId, serviceObject, dictionary, true, &registration);
    return registration;
}

celix_status_t serviceRegistration_createInternal(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId,
        void * serviceObject, PROPERTIES dictionary, bool isFactory, SERVICE_REGISTRATION *registration) {
    celix_status_t status = CELIX_SUCCESS;

    *registration = (SERVICE_REGISTRATION) malloc(sizeof(**registration));
    (*registration)->isServiceFactory = isFactory;
    (*registration)->registry = registry;
    (*registration)->className = serviceName;

	if (dictionary == NULL) {
		dictionary = properties_create();
	}

	char sId[sizeof(serviceId) + 1];
	sprintf(sId, "%ld", serviceId);
	properties_set(dictionary, (char *) SERVICE_ID, sId);
	properties_set(dictionary, (char *) OBJECTCLASS, serviceName);

	(*registration)->properties = dictionary;

	(*registration)->serviceId = serviceId;
	(*registration)->svcObj = serviceObject;
	if (isFactory) {
	    (*registration)->serviceFactory = (service_factory_t) (*registration)->svcObj;
	} else {
	    (*registration)->serviceFactory = NULL;
	}

	SERVICE_REFERENCE reference = (SERVICE_REFERENCE) malloc(sizeof(*reference));
	reference->bundle = bundle;
	reference->registration = *registration;

	(*registration)->reference = reference;

	(*registration)->isUnregistering = false;
	pthread_mutex_init(&(*registration)->mutex, NULL);

	return CELIX_SUCCESS;
}

void serviceRegistration_destroy(SERVICE_REGISTRATION registration) {
	registration->className = NULL;
	registration->registry = NULL;

	registration->reference->bundle = NULL;
	registration->reference->registration = NULL;
	free(registration->reference);

	properties_destroy(registration->properties);

	pthread_mutex_destroy(&registration->mutex);

	free(registration);
}

bool serviceRegistration_isValid(SERVICE_REGISTRATION registration) {
	return registration->svcObj != NULL;
}

void serviceRegistration_invalidate(SERVICE_REGISTRATION registration) {
	pthread_mutex_lock(&registration->mutex);
	registration->svcObj = NULL;
	pthread_mutex_unlock(&registration->mutex);
}

void serviceRegistration_unregister(SERVICE_REGISTRATION registration) {
	pthread_mutex_lock(&registration->mutex);
	if (!serviceRegistration_isValid(registration) || registration->isUnregistering) {
		printf("Service is already unregistered\n");
		return;
	}
	registration->isUnregistering = true;
	pthread_mutex_unlock(&registration->mutex);

	serviceRegistry_unregisterService(registration->registry, registration->reference->bundle, registration);

	// Unregister service cleans up the registration
//	pthread_mutex_lock(&registration->mutex);
//	registration->svcObj = NULL;
//	pthread_mutex_unlock(&registration->mutex);
}

celix_status_t serviceRegistration_getService(SERVICE_REGISTRATION registration, BUNDLE bundle, void **service) {
    if (registration->isServiceFactory) {
        service_factory_t factory = registration->serviceFactory;
        factory->getService(registration->serviceFactory, bundle, registration, service);
    } else {
        (*service) = registration->svcObj;
    }
    return CELIX_SUCCESS;
}
