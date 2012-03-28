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
#include "service_reference.h"

struct serviceRegistration {
	SERVICE_REGISTRY registry;
	char * className;
	// SERVICE_REFERENCE reference;
	ARRAY_LIST references;
	BUNDLE bundle;
	PROPERTIES properties;
	void * svcObj;
	long serviceId;

	apr_thread_mutex_t *mutex;
	bool isUnregistering;

	bool isServiceFactory;
	void *serviceFactory;
};

celix_status_t serviceRegistration_createInternal(apr_pool_t *pool, SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId,
        void * serviceObject, PROPERTIES dictionary, bool isFactory, SERVICE_REGISTRATION *registration);

SERVICE_REGISTRATION serviceRegistration_create(apr_pool_t *pool, SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary) {
    SERVICE_REGISTRATION registration = NULL;
	serviceRegistration_createInternal(pool, registry, bundle, serviceName, serviceId, serviceObject, dictionary, false, &registration);
	return registration;
}

SERVICE_REGISTRATION serviceRegistration_createServiceFactory(apr_pool_t *pool, SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId, void * serviceObject, PROPERTIES dictionary) {
    SERVICE_REGISTRATION registration = NULL;
    serviceRegistration_createInternal(pool, registry, bundle, serviceName, serviceId, serviceObject, dictionary, true, &registration);
    return registration;
}

celix_status_t serviceRegistration_createInternal(apr_pool_t *pool, SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, long serviceId,
        void * serviceObject, PROPERTIES dictionary, bool isFactory, SERVICE_REGISTRATION *registration) {
    celix_status_t status = CELIX_SUCCESS;
	char * sId = (char *)malloc(sizeof(serviceId) + 1);
	SERVICE_REFERENCE reference;

    *registration = (SERVICE_REGISTRATION) apr_palloc(pool, sizeof(**registration));
    (*registration)->isServiceFactory = isFactory;
    (*registration)->registry = registry;
    (*registration)->className = apr_pstrdup(pool,serviceName);
    (*registration)->bundle = bundle;
    (*registration)->references = NULL;
    arrayList_create(pool, &(*registration)->references);

	if (dictionary == NULL) {
		dictionary = properties_create();
	}

	
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

//	serviceReference_create(pool, bundle, *registration, &(*registration)->reference);

	(*registration)->isUnregistering = false;
	apr_thread_mutex_create(&(*registration)->mutex, 0, pool);

	free(sId);
	return CELIX_SUCCESS;
}

void serviceRegistration_destroy(SERVICE_REGISTRATION registration) {
	registration->className = NULL;
	registration->registry = NULL;

	properties_destroy(registration->properties);
	arrayList_destroy(registration->references);

	apr_thread_mutex_destroy(registration->mutex);

//	free(registration);
}

bool serviceRegistration_isValid(SERVICE_REGISTRATION registration) {
	return registration == NULL ? false : registration->svcObj != NULL;
}

void serviceRegistration_invalidate(SERVICE_REGISTRATION registration) {
	apr_thread_mutex_lock(registration->mutex);
	registration->svcObj = NULL;
	apr_thread_mutex_unlock(registration->mutex);
}

celix_status_t serviceRegistration_unregister(SERVICE_REGISTRATION registration) {
	celix_status_t status = CELIX_SUCCESS;
	apr_thread_mutex_lock(registration->mutex);
	if (!serviceRegistration_isValid(registration) || registration->isUnregistering) {
		printf("Service is already unregistered\n");
		status = CELIX_ILLEGAL_STATE;
	} else {
		registration->isUnregistering = true;
	}
	apr_thread_mutex_unlock(registration->mutex);

//	BUNDLE bundle = NULL;
//	status = serviceReference_getBundle(registration->reference, &bundle);
	if (status == CELIX_SUCCESS) {
		serviceRegistry_unregisterService(registration->registry, registration->bundle, registration);
	}

	return status;
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

celix_status_t serviceRegistration_getProperties(SERVICE_REGISTRATION registration, PROPERTIES *properties) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *properties == NULL) {
		*properties = registration->properties;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t serviceRegistration_getRegistry(SERVICE_REGISTRATION registration, SERVICE_REGISTRY *registry) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *registry == NULL) {
		*registry = registration->registry;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t serviceRegistration_getServiceReferences(SERVICE_REGISTRATION registration, ARRAY_LIST *references) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *references == NULL) {
		*references = registration->references;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t serviceRegistration_getBundle(SERVICE_REGISTRATION registration, BUNDLE *bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *bundle == NULL) {
		*bundle = registration->bundle;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t serviceRegistration_getServiceName(SERVICE_REGISTRATION registration, char **serviceName) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *serviceName == NULL) {
		*serviceName = registration->className;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}
