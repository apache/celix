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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "celix_build_assert.h"
#include "celix_constants.h"
#include "celix_err.h"
#include "service_registration_private.h"

static bool serviceRegistration_destroy(struct celix_ref *);
static celix_status_t serviceRegistration_initializeProperties(service_registration_t*registration,
                                                               celix_properties_t* props);

static celix_status_t serviceRegistration_createInternal(registry_callback_t callback,
                                                         celix_bundle_t* bundle,
                                                         const char* serviceName,
                                                         long serviceId,
                                                         const void* serviceObject,
                                                         celix_properties_t* dictionary,
                                                         enum celix_service_type svcType,
                                                         service_registration_t** out);

service_registration_pt serviceRegistration_create(registry_callback_t callback, bundle_pt bundle, const char* serviceName, long serviceId, const void * serviceObject, celix_properties_t* dictionary) {
    service_registration_pt registration = NULL;
	serviceRegistration_createInternal(callback, bundle, serviceName, serviceId, serviceObject, dictionary, CELIX_PLAIN_SERVICE, &registration);
	return registration;
}

service_registration_pt serviceRegistration_createServiceFactory(registry_callback_t callback, bundle_pt bundle, const char* serviceName, long serviceId, const void * serviceObject, celix_properties_t* dictionary) {
    service_registration_pt registration = NULL;
    serviceRegistration_createInternal(callback, bundle, serviceName, serviceId, serviceObject, dictionary, CELIX_DEPRECATED_FACTORY_SERVICE, &registration);
    return registration;
}

static celix_status_t serviceRegistration_createInternal(registry_callback_t callback,
                                                         celix_bundle_t* bundle,
                                                         const char* serviceName,
                                                         long serviceId,
                                                         const void* serviceObject,
                                                         celix_properties_t* dictionary,
                                                         enum celix_service_type svcType,
                                                         service_registration_t** out) {
    celix_status_t status = CELIX_SUCCESS;
    service_registration_pt reg = calloc(1, sizeof(*reg));
    if (reg) {
        celix_ref_init(&reg->refCount);
        reg->callback = callback;
        reg->svcType = svcType;
        reg->className = strndup(serviceName, 1024 * 10);
        reg->bundle = bundle;
        reg->serviceId = serviceId;
        reg->svcObj = serviceObject;
        reg->isUnregistering = false;
        celixThreadRwlock_create(&reg->lock, NULL);
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
    celix_ref_get(&registration->refCount);
}

void serviceRegistration_release(service_registration_pt registration) {
    celix_ref_put(&registration->refCount, serviceRegistration_destroy);
}

static bool serviceRegistration_destroy(struct celix_ref *refCount) {
    service_registration_pt registration = (service_registration_pt )refCount;
    CELIX_BUILD_ASSERT(offsetof(service_registration_t, refCount) == 0);

    //fw_log(logger, CELIX_LOG_LEVEL_DEBUG, "Destroying service registration %p\n", registration);
    free(registration->className);
    registration->className = NULL;
    registration->callback.unregister = NULL;
    celix_properties_destroy(registration->properties);
    celixThreadRwlock_destroy(&registration->lock);
    free(registration);
    return true;
}

static celix_status_t serviceRegistration_initializeProperties(service_registration_t*registration,
                                                               celix_properties_t* props) {
    if (!props) {
        props = celix_properties_create();
    }
    if (!props) {
        return CELIX_ENOMEM;
    }

    celix_status_t status = celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, registration->serviceId);
    CELIX_DO_IF(status, status = celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, registration->className));

    if (status == CELIX_SUCCESS) {
        registration->properties = props;
    } else {
        celix_err_push("Cannot initialize service registration properties");
        celix_properties_destroy(props);
    }

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
    bool unregistering = false;
    registry_callback_t callback;
    callback.unregister = NULL;

    // Without any further need of synchronization between callers, __ATOMIC_RELAXED should be sufficient to guarantee that only one caller has a chance to run.
    // Strong form of compare-and-swap is used to avoid spurious failure.
    if(!__atomic_compare_exchange_n(&registration->isUnregistering, &unregistering /* expected*/ , true /* desired */,
                                    false /* weak */, __ATOMIC_RELAXED/*success memorder*/, __ATOMIC_RELAXED/*failure memorder*/)) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        callback = registration->callback;
        if (callback.unregister != NULL) {
            status = callback.unregister(callback.handle, registration->bundle, registration);
        }
    }

    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot unregister service registration");
	return status;
}

celix_status_t serviceRegistration_getService(service_registration_pt registration, bundle_pt bundle, const void** service) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&registration->lock);
    if(registration->svcObj == NULL) {
        *service = NULL;
        status = CELIX_ILLEGAL_STATE;
    } else if (registration->svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
        service_factory_pt factory = registration->deprecatedFactory;
        status = factory->getService(factory->handle, bundle, registration, (void **) service);
    } else if (registration->svcType == CELIX_FACTORY_SERVICE) {
        celix_service_factory_t *fac = registration->factory;
        *service = fac->getService(fac->handle, bundle, registration->properties);
    } else { //plain service
        (*service) = registration->svcObj;
    }
    celixThreadRwlock_unlock(&registration->lock);
    if(status == CELIX_SUCCESS && *service == NULL) {
        status = CELIX_SERVICE_EXCEPTION;
    }
    return status;
}

celix_status_t serviceRegistration_ungetService(service_registration_pt registration, bundle_pt bundle, const void** service) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&registration->lock);
    if(registration->svcObj == NULL) {
        status = CELIX_ILLEGAL_STATE;
    } else if (registration->svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
        service_factory_pt factory = registration->deprecatedFactory;
        status  = factory->ungetService(factory->handle, bundle, registration, (void**) service);
    } else if (registration->svcType == CELIX_FACTORY_SERVICE) {
        celix_service_factory_t *fac = registration->factory;
        fac->ungetService(fac->handle, bundle, registration->properties);
    }
    celixThreadRwlock_unlock(&registration->lock);
    *service = NULL;
    return status;
}

celix_status_t serviceRegistration_getProperties(service_registration_pt registration, celix_properties_t** properties) {
	celix_status_t status = CELIX_SUCCESS;

    if (registration != NULL) {
        *properties = registration->properties;
     } else {
          status = CELIX_ILLEGAL_ARGUMENT;
     }

    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot get registration properties");

    return status;
}

celix_status_t serviceRegistration_getBundle(service_registration_pt registration, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;
    if (registration == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    if (registration != NULL && *bundle == NULL) {
        *bundle = registration->bundle;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot get bundle");

	return status;
}

celix_status_t serviceRegistration_getServiceName(service_registration_pt registration, const char **serviceName) {
	celix_status_t status = CELIX_SUCCESS;

    if (registration != NULL && serviceName != NULL) {
        *serviceName = registration->className;
	} else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }


    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot get service name");

	return status;
}

long serviceRegistration_getServiceId(service_registration_t *registration) {
    long svcId = -1;
    if (registration != NULL) {
        svcId = registration->serviceId;
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

bool serviceRegistration_isFactoryService(service_registration_t *registration) {
    bool isFactory = false;
    if (registration != NULL) {
        isFactory = registration->svcType == CELIX_FACTORY_SERVICE || registration->svcType == CELIX_DEPRECATED_FACTORY_SERVICE;
    }
    return isFactory;
}