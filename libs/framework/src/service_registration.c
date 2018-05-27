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

static celix_status_t serviceRegistration_initializeProperties(service_registration_pt registration, properties_pt properties);
static celix_status_t serviceRegistration_createInternal(registry_callback_t callback, bundle_pt bundle, const char* serviceName, unsigned long serviceId,
        const void * serviceObject, properties_pt dictionary, enum celix_service_type svcType, service_registration_pt *registration);
static celix_status_t serviceRegistration_destroy(service_registration_pt registration);

service_registration_pt serviceRegistration_create(registry_callback_t callback, bundle_pt bundle, const char* serviceName, unsigned long serviceId, const void * serviceObject, properties_pt dictionary) {
    service_registration_pt registration = NULL;
	serviceRegistration_createInternal(callback, bundle, serviceName, serviceId, serviceObject, dictionary, CELIX_PLAIN_SERVICE, &registration);
	return registration;
}

service_registration_pt serviceRegistration_createServiceFactory(registry_callback_t callback, bundle_pt bundle, const char* serviceName, unsigned long serviceId, const void * serviceObject, properties_pt dictionary) {
    service_registration_pt registration = NULL;
    serviceRegistration_createInternal(callback, bundle, serviceName, serviceId, serviceObject, dictionary, CELIX_DEPRECATED_FACTORY_SERVICE, &registration);
    return registration;
}

static celix_status_t serviceRegistration_createInternal(registry_callback_t callback, bundle_pt bundle, const char* serviceName, unsigned long serviceId,
                                                         const void * serviceObject, properties_pt dictionary, enum celix_service_type svcType, service_registration_pt *out) {

    celix_status_t status = CELIX_SUCCESS;
	service_registration_pt  reg = calloc(1, sizeof(*reg));
    if (reg) {
        reg->callback = callback;
        reg->services = NULL;
        reg->nrOfServices = 0;
		reg->svcType = svcType;
		reg->className = strndup(serviceName, 1024*10);
		reg->bundle = bundle;
		reg->refCount = 1;
		reg->serviceId = serviceId;
	    reg->svcObj = serviceObject;

		if (svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
			reg->deprecatedFactory = (service_factory_pt) reg->svcObj;
		} else if (svcType == CELIX_FACTORY_SERVICE) {
			reg->factory = (celix_service_factory_t*) reg->svcObj;
		}

		reg->isUnregistering = false;
		celixThreadRwlock_create(&reg->lock, NULL);

		celixThreadRwlock_writeLock(&reg->lock);
		serviceRegistration_initializeProperties(reg, dictionary);
		celixThreadRwlock_unlock(&reg->lock);

	} else {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		*out = reg;
	}

	return status;
}

void serviceRegistration_retain(service_registration_pt registration) {
	celixThreadRwlock_writeLock(&registration->lock);
	registration->refCount += 1;
    celixThreadRwlock_unlock(&registration->lock);
}

void serviceRegistration_release(service_registration_pt registration) {
    celixThreadRwlock_writeLock(&registration->lock);
    assert(registration->refCount > 0);
	registration->refCount -= 1;
	if (registration->refCount == 0) {
		serviceRegistration_destroy(registration);
	} else {
        celixThreadRwlock_unlock(&registration->lock);
	}
}

static celix_status_t serviceRegistration_destroy(service_registration_pt registration) {
	//fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "Destroying service registration %p\n", registration);
    free(registration->className);
	registration->className = NULL;

    registration->callback.unregister = NULL;

	properties_destroy(registration->properties);
	celixThreadRwlock_unlock(&registration->lock);
    celixThreadRwlock_destroy(&registration->lock);
	free(registration);

	return CELIX_SUCCESS;
}

static celix_status_t serviceRegistration_initializeProperties(service_registration_pt registration, properties_pt dictionary) {
    char sId[32];

	if (dictionary == NULL) {
		dictionary = properties_create();
	}


	snprintf(sId, 32, "%lu", registration->serviceId);
	properties_set(dictionary, (char *) OSGI_FRAMEWORK_SERVICE_ID, sId);

	if (properties_get(dictionary, (char *) OSGI_FRAMEWORK_OBJECTCLASS) == NULL) {
		properties_set(dictionary, (char *) OSGI_FRAMEWORK_OBJECTCLASS, registration->className);
	}

	registration->properties = dictionary;

	return CELIX_SUCCESS;
}

void serviceRegistration_invalidate(service_registration_pt registration) {
    celixThreadRwlock_writeLock(&registration->lock);
    registration->svcObj = NULL;
    celixThreadRwlock_unlock(&registration->lock);
}

bool serviceRegistration_isValid(service_registration_pt registration) {
    bool isValid;
    if (registration != NULL) {
        celixThreadRwlock_readLock(&registration->lock);
        isValid = registration->svcObj != NULL;
        celixThreadRwlock_unlock(&registration->lock);
    } else {
        isValid = false;
    }
    return isValid;
}

celix_status_t serviceRegistration_unregister(service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

    bool notValidOrUnregistering;
    celixThreadRwlock_readLock(&registration->lock);
    notValidOrUnregistering = !serviceRegistration_isValid(registration) || registration->isUnregistering;
    celixThreadRwlock_unlock(&registration->lock);

    registry_callback_t callback;
    callback.unregister = NULL;
    bundle_pt bundle = NULL;

	if (notValidOrUnregistering) {
		status = CELIX_ILLEGAL_STATE;
	} else {
        celixThreadRwlock_writeLock(&registration->lock);
        registration->isUnregistering = true;
        bundle = registration->bundle;
        callback = registration->callback;
        celixThreadRwlock_unlock(&registration->lock);
    }

	if (status == CELIX_SUCCESS && callback.unregister != NULL) {
        callback.unregister(callback.handle, bundle, registration);
	}

	framework_logIfError(logger, status, NULL, "Cannot unregister service registration");

	return status;
}

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, const void** service) {
    celixThreadRwlock_readLock(&registration->lock);
    if (registration->svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
        service_factory_pt factory = registration->deprecatedFactory;
        factory->getService(factory->handle, bundle, registration, (void **) service);
    } else if (registration->svcType == CELIX_FACTORY_SERVICE) {
        celix_service_factory_t *fac = registration->factory;
        *service = fac->getService(fac->handle, bundle, registration->properties);
    } else { //plain service
        (*service) = registration->svcObj;
    }
    celixThreadRwlock_unlock(&registration->lock);

    return CELIX_SUCCESS;
}

celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, const void** service) {
    celixThreadRwlock_readLock(&registration->lock);
    if (registration->svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
        service_factory_pt factory = registration->deprecatedFactory;
        factory->ungetService(factory->handle, bundle, registration, (void**) service);
    } else if (registration->svcType == CELIX_FACTORY_SERVICE) {
        celix_service_factory_t *fac = registration->factory;
        fac->ungetService(fac->handle, bundle, registration->properties);
    }
    celixThreadRwlock_unlock(&registration->lock);
    return CELIX_SUCCESS;
}

celix_status_t serviceRegistration_getProperties(service_registration_pt registration, properties_pt *properties) {
	celix_status_t status = CELIX_SUCCESS;

    if (registration != NULL) {
        celixThreadRwlock_readLock(&registration->lock);
        *properties = registration->properties;
        celixThreadRwlock_unlock(&registration->lock);
     } else {
          status = CELIX_ILLEGAL_ARGUMENT;
     }

    framework_logIfError(logger, status, NULL, "Cannot get registration properties");

    return status;
}

celix_status_t serviceRegistration_setProperties(service_registration_pt registration, properties_pt properties) {
    celix_status_t status;

    properties_pt oldProperties = NULL;
    registry_callback_t callback;

    celixThreadRwlock_writeLock(&registration->lock);
    oldProperties = registration->properties;
    status = serviceRegistration_initializeProperties(registration, properties);
    callback = registration->callback;
    celixThreadRwlock_unlock(&registration->lock);

    if (status == CELIX_SUCCESS && callback.modified != NULL) {
        callback.modified(callback.handle, registration, oldProperties);
    }

	return status;
}


celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;
    if (registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    if (registration != NULL && *bundle == NULL) {
        celixThreadRwlock_readLock(&registration->lock);
        *bundle = registration->bundle;
        celixThreadRwlock_unlock(&registration->lock);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

    framework_logIfError(logger, status, NULL, "Cannot get bundle");

	return status;
}

celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, const char **serviceName) {
	celix_status_t status = CELIX_SUCCESS;

    if (registration != NULL && serviceName != NULL) {
        celixThreadRwlock_readLock(&registration->lock);
        *serviceName = registration->className;
        celixThreadRwlock_unlock(&registration->lock);
	} else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }


    framework_logIfError(logger, status, NULL, "Cannot get service name");

	return status;
}

long serviceRegistration_getServiceId(service_registration_t *registration) {
    long svcId = -1;
    if (registration != NULL) {
        celixThreadRwlock_readLock(&registration->lock);
        svcId = registration->serviceId;
        celixThreadRwlock_unlock(&registration->lock);
    }
    return svcId;
}


service_registration_t* celix_serviceRegistration_createServiceFactory(
        registry_callback_t callback,
        const celix_bundle_t *bnd,
        const char *serviceName,
        long svcId,
        celix_service_factory_t* factory,
        celix_properties_t *props) {
    service_registration_pt registration = NULL;
    serviceRegistration_createInternal(callback, (celix_bundle_t*)bnd, serviceName, svcId, factory, props, CELIX_FACTORY_SERVICE, &registration);
    return registration;
}