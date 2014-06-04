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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <apr_strings.h>

#include "service_registration_private.h"
#include "constants.h"
#include "service_factory.h"
#include "service_reference.h"
#include "celix_log.h"
#include "celix_threads.h"

static celix_status_t serviceRegistration_initializeProperties(service_registration_pt registration, properties_pt properties);

celix_status_t serviceRegistration_createInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId,
        void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *registration);

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

celix_status_t serviceRegistration_createInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, long serviceId,
        void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *registration) {
    celix_status_t status = CELIX_SUCCESS;

    *registration = malloc(sizeof(**registration));
    if (*registration) {
		(*registration)->isServiceFactory = isFactory;
		(*registration)->registry = registry;
		(*registration)->className = strdup(serviceName);
		(*registration)->bundle = bundle;
		(*registration)->references = NULL;
		arrayList_create(&(*registration)->references);

		(*registration)->serviceId = serviceId;
		(*registration)->svcObj = serviceObject;
		if (isFactory) {
			(*registration)->serviceFactory = (service_factory_pt) (*registration)->svcObj;
		} else {
			(*registration)->serviceFactory = NULL;
		}

		//	serviceReference_create(pool, bundle, *registration, &(*registration)->reference);

		(*registration)->isUnregistering = false;
		celixThreadMutex_create(&(*registration)->mutex, NULL);

		serviceRegistration_initializeProperties(*registration, dictionary);
    } else {
    	status = CELIX_ENOMEM;
    }

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistration_destroy(service_registration_pt registration) {
	registration->className = NULL;
	registration->registry = NULL;

	properties_destroy(registration->properties);
	arrayList_destroy(registration->references);

	celixThreadMutex_destroy(&registration->mutex);

	free(registration->className);
	free(registration);

	return CELIX_SUCCESS;
}

static celix_status_t serviceRegistration_initializeProperties(service_registration_pt registration, properties_pt dictionary) {
	char * sId = (char *)malloc(sizeof(registration->serviceId) + 1);

	if (dictionary == NULL) {
		dictionary = properties_create();
	}

	registration->properties = dictionary;

	sprintf(sId, "%ld", registration->serviceId);
	properties_set(dictionary, (char *) OSGI_FRAMEWORK_SERVICE_ID, sId);
	properties_set(dictionary, (char *) OSGI_FRAMEWORK_OBJECTCLASS, registration->className);
	free(sId);

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

//	bundle_pt bundle = NULL;
//	status = serviceReference_getBundle(registration->reference, &bundle);
	if (status == CELIX_SUCCESS) {
		serviceRegistry_unregisterService(registration->registry, registration->bundle, registration);
	}

	framework_logIfError(logger, status, NULL, "Cannot unregister service registration");

	return status;
}

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, void **service) {
    if (registration->isServiceFactory) {
        service_factory_pt factory = registration->serviceFactory;
        factory->getService(registration->serviceFactory, bundle, registration, service);
    } else {
        (*service) = registration->svcObj;
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
		*references = registration->references;
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
