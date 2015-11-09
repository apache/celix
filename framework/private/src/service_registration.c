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
 *  \date       Aug 6, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "service_registration_private.h"
#include "constants.h"
#include "service_factory.h"
#include "service_reference.h"
#include "celix_log.h"
#include "celix_threads.h"

static celix_status_t serviceRegistration_initializeProperties(service_registration_pt registration, properties_pt properties);
static celix_status_t serviceRegistration_createInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId,
        void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *registration);
static celix_status_t serviceRegistration_destroy(service_registration_pt registration);

service_registration_pt serviceRegistration_create(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary) {
    service_registration_pt registration = NULL;
	serviceRegistration_createInternal(registry, bundle, serviceName, serviceId, serviceObject, dictionary, false, &registration);
	return registration;
}

service_registration_pt serviceRegistration_createServiceFactory(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId, void * serviceObject, properties_pt dictionary) {
    service_registration_pt registration = NULL;
    serviceRegistration_createInternal(registry, bundle, serviceName, serviceId, serviceObject, dictionary, true, &registration);
    return registration;
}

static celix_status_t serviceRegistration_createInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId,
        void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *out) {
    celix_status_t status = CELIX_SUCCESS;

	service_registration_pt  reg = calloc(1, sizeof(*reg));
    if (reg) {
        reg->services = NULL;
        reg->nrOfServices = 0;
		reg->isServiceFactory = isFactory;
		reg->registry = registry;
		reg->className = strdup(serviceName);
		reg->bundle = bundle;
		reg->refCount = 1;

		reg->serviceId = serviceId;
		reg->svcObj = serviceObject;
		if (isFactory) {
			reg->serviceFactory = (service_factory_pt) reg->svcObj;
		} else {
			reg->serviceFactory = NULL;
		}

		reg->isUnregistering = false;
		celixThreadMutex_create(&reg->mutex, NULL);

		serviceRegistration_initializeProperties(reg, dictionary);
    } else {
    	status = CELIX_ENOMEM;
    }

	if (status == CELIX_SUCCESS) {
		*out = reg;
	}

	return status;
}

void serviceRegistration_retain(service_registration_pt registration) {
	celixThreadMutex_lock(&registration->mutex);
	registration->refCount += 1;
	celixThreadMutex_unlock(&registration->mutex);
}

void serviceRegistration_release(service_registration_pt registration) {
	celixThreadMutex_lock(&registration->mutex);
	assert(registration->refCount > 0);
	registration->refCount -= 1;
	if (registration->refCount == 0) {
		serviceRegistration_destroy(registration);
	} else {
		celixThreadMutex_unlock(&registration->mutex);
	}
}

static celix_status_t serviceRegistration_destroy(service_registration_pt registration) {
    free(registration->className);
	registration->className = NULL;
	registration->registry = NULL;

	properties_destroy(registration->properties);

	celixThreadMutex_destroy(&registration->mutex);

	free(registration);
	registration = NULL;

	return CELIX_SUCCESS;
}

static celix_status_t serviceRegistration_initializeProperties(service_registration_pt registration, properties_pt dictionary) {
    char sId[32];

	if (dictionary == NULL) {
		dictionary = properties_create();
	}

	registration->properties = dictionary;

	snprintf(sId, 32, "%ld", registration->serviceId);
	properties_set(dictionary, (char *) OSGI_FRAMEWORK_SERVICE_ID, sId);

	if (properties_get(dictionary, (char *) OSGI_FRAMEWORK_OBJECTCLASS) == NULL) {
		properties_set(dictionary, (char *) OSGI_FRAMEWORK_OBJECTCLASS, registration->className);
	}

	return CELIX_SUCCESS;
}

bool serviceRegistration_isValid(service_registration_pt registration) {
	return registration == NULL ? false : registration->svcObj != NULL;
}

void serviceRegistration_invalidate(service_registration_pt registration) {
	celixThreadMutex_lock(&registration->mutex);
	registration->svcObj = NULL;
	celixThreadMutex_unlock(&registration->mutex);
}

celix_status_t serviceRegistration_unregister(service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;
	celixThreadMutex_lock(&registration->mutex);
	if (!serviceRegistration_isValid(registration) || registration->isUnregistering) {
		printf("Service is already unregistered\n");
		status = CELIX_ILLEGAL_STATE;
	} else {
		registration->isUnregistering = true;
	}
	celixThreadMutex_unlock(&registration->mutex);

	if (status == CELIX_SUCCESS) {
		serviceRegistry_unregisterService(registration->registry, registration->bundle, registration);
	}

	framework_logIfError(logger, status, NULL, "Cannot unregister service registration");

	return status;
}

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, void **service) {
	int status = CELIX_SUCCESS;
    if (registration->isServiceFactory) {
        service_factory_pt factory = registration->serviceFactory;
        status = factory->getService(factory->factory, bundle, registration, service);
    } else {
        (*service) = registration->svcObj;
    }
    return status;
}

celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, void **service) {
    if (registration->isServiceFactory) {
        service_factory_pt factory = registration->serviceFactory;
        factory->ungetService(factory->factory, bundle, registration, service);
    }
    return CELIX_SUCCESS;
}

celix_status_t serviceRegistration_getProperties(service_registration_pt registration, properties_pt *properties) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *properties == NULL) {
		*properties = registration->properties;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Cannot get registration properties");

	return status;
}

celix_status_t serviceRegistration_setProperties(service_registration_pt registration, properties_pt properties) {
	properties_pt oldProps = registration->properties;

	serviceRegistration_initializeProperties(registration, properties);

	serviceRegistry_servicePropertiesModified(registration->registry, registration, oldProps);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistration_getRegistry(service_registration_pt registration, service_registry_pt *registry) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *registry == NULL) {
		*registry = registration->registry;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Cannot get registry");

	return status;
}

celix_status_t serviceRegistration_getServiceReferences(service_registration_pt registration, array_list_pt *references) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *references == NULL) {
	    serviceRegistry_getServiceReferencesForRegistration(registration->registry, registration, references);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Cannot get service reference");

	return status;
}

celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *bundle == NULL) {
		*bundle = registration->bundle;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Cannot get bundle");

	return status;
}

celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, char **serviceName) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL && *serviceName == NULL) {
		*serviceName = registration->className;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Cannot get service name");

	return status;
}
