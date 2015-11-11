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
 * service_registry.c
 *
 *  \date       Aug 6, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service_registry_private.h"
#include "service_registration_private.h"
#include "module.h"
#include "bundle.h"
#include "listener_hook_service.h"
#include "constants.h"
#include "service_reference_private.h"
#include "framework_private.h"
#include "celix_log.h"

celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *registration);
celix_status_t serviceRegistry_addHooks(service_registry_pt registry, char *serviceName, void *serviceObject, service_registration_pt registration);
celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration);
static void serviceRegistry_logWarningServiceReferenceUsageCount(service_registry_pt registry, size_t usageCount, size_t refCount);
static void serviceRegistry_logWarningServiceRegistration(service_registry_pt registry, service_registration_pt reg);


celix_status_t serviceRegistry_create(framework_pt framework, serviceChanged_function_pt serviceChanged, service_registry_pt *registry) {
	celix_status_t status = CELIX_SUCCESS;

	*registry = malloc(sizeof(**registry));
	if (!*registry) {
		status = CELIX_ENOMEM;
	} else {

		(*registry)->serviceChanged = serviceChanged;
		(*registry)->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
		(*registry)->framework = framework;
		(*registry)->currentServiceId = 1l;
		(*registry)->serviceReferences = hashMap_create(NULL, NULL, NULL, NULL);;

		arrayList_create(&(*registry)->listenerHooks);

		status = celixThreadMutexAttr_create(&(*registry)->mutexAttr);
		status = CELIX_DO_IF(status, celixThreadMutexAttr_settype(&(*registry)->mutexAttr, CELIX_THREAD_MUTEX_RECURSIVE));
		status = CELIX_DO_IF(status, celixThreadMutex_create(&(*registry)->mutex, &(*registry)->mutexAttr));
		status = CELIX_DO_IF(status, celixThreadMutex_create(&(*registry)->referencesMapMutex, NULL));
	}

	framework_logIfError(logger, status, NULL, "Cannot create service registry");

	return status;
}

celix_status_t serviceRegistry_destroy(service_registry_pt registry) {
    hashMap_destroy(registry->serviceRegistrations, false, false);
    hashMap_destroy(registry->serviceReferences, false, false);
    arrayList_destroy(registry->listenerHooks);
    celixThreadMutex_destroy(&registry->mutex);
    celixThreadMutexAttr_destroy(&registry->mutexAttr);
    celixThreadMutex_destroy(&registry->referencesMapMutex);
    registry->framework = NULL;
    registry->listenerHooks = NULL;
    registry->serviceChanged = NULL;
    registry->serviceReferences = NULL;
    registry->serviceRegistrations = NULL;
    free(registry);

    return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getRegisteredServices(service_registry_pt registry, bundle_pt bundle, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&registry->mutex);
	array_list_pt regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		unsigned int i;
		arrayList_create(services);
		
		for (i = 0; i < arrayList_size(regs); i++) {
			service_registration_pt reg = arrayList_get(regs, i);
			if (serviceRegistration_isValid(reg)) {
				service_reference_pt reference = NULL;
				status = serviceRegistry_getServiceReference(registry, bundle, reg, &reference);
				if (status == CELIX_SUCCESS) {
					arrayList_add(*services, reference);
				}
			}
		}
	}
	celixThreadMutex_unlock(&registry->mutex);

	framework_logIfError(logger, status, NULL, "Cannot get registered services");

	return status;
}

celix_status_t serviceRegistry_registerService(service_registry_pt registry, bundle_pt bundle, char * serviceName, void * serviceObject, properties_pt dictionary, service_registration_pt *registration) {
    return serviceRegistry_registerServiceInternal(registry, bundle, serviceName, serviceObject, dictionary, false, registration);
}

celix_status_t serviceRegistry_registerServiceFactory(service_registry_pt registry, bundle_pt bundle, char * serviceName, service_factory_pt factory, properties_pt dictionary, service_registration_pt *registration) {
    return serviceRegistry_registerServiceInternal(registry, bundle, serviceName, (void *) factory, dictionary, true, registration);
}

celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *registration) {
	array_list_pt regs;
	celixThreadMutex_lock(&registry->mutex);

	if (isFactory) {
	    *registration = serviceRegistration_createServiceFactory(registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	} else {
	    *registration = serviceRegistration_create(registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	}

	serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);

	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = NULL;
		arrayList_create(&regs);
	}
	arrayList_add(regs, *registration);
	hashMap_put(registry->serviceRegistrations, bundle, regs);

	celixThreadMutex_unlock(&registry->mutex);

	if (registry->serviceChanged != NULL) {
//		service_event_pt event = (service_event_pt) malloc(sizeof(*event));
//		event->type = REGISTERED;
//		event->reference = (*registration)->reference;
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED, *registration, NULL);
//		free(event);
//		event = NULL;
	}

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_unregisterService(service_registry_pt registry, bundle_pt bundle, service_registration_pt registration) {
	// array_list_t clients;
	array_list_pt regs;

	celixThreadMutex_lock(&registry->mutex);

	serviceRegistry_removeHook(registry, registration);

	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
	}

	celixThreadMutex_unlock(&registry->mutex);

	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING, registration, NULL);
	}

	celixThreadMutex_lock(&registry->mutex);

    //invalidate service references
    hash_map_iterator_pt iter = hashMapIterator_create(registry->serviceReferences);
    while (hashMapIterator_hasNext(iter)) {
        hash_map_pt refsMap = hashMapIterator_nextValue(iter);
        service_reference_pt ref = hashMap_get(refsMap, registration);
        if (ref != NULL) {
            serviceReference_invalidate(ref);
        }
    }
    hashMapIterator_destroy(iter);

    serviceRegistration_invalidate(registration);
    serviceRegistration_release(registration);

	celixThreadMutex_unlock(&registry->mutex);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_clearServiceRegistrations(service_registry_pt registry, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt registrations = NULL;

    celixThreadMutex_lock(&registry->mutex);

    registrations = hashMap_get(registry->serviceRegistrations, bundle);
    while (registrations != NULL && arrayList_size(registrations) > 0) {
        service_registration_pt reg = arrayList_get(registrations, 0);

        serviceRegistry_logWarningServiceRegistration(registry, reg);

        if (serviceRegistration_isValid(reg)) {
            serviceRegistration_unregister(reg);
        }
        serviceRegistration_release(reg);
    }
    hashMap_remove(registry->serviceRegistrations, bundle);

    celixThreadMutex_unlock(&registry->mutex);


    return status;
}

static void serviceRegistry_logWarningServiceRegistration(service_registry_pt registry, service_registration_pt reg) {
    char *servName = NULL;
    serviceRegistration_getServiceName(reg, &servName);
    fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Dangling service registration for service %s. Look for missing serviceRegistration_unregister.", servName);
}

celix_status_t serviceRegistry_getServiceReference(service_registry_pt registry, bundle_pt owner,
                                                   service_registration_pt registration,
                                                   service_reference_pt *out) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_pt bundle = NULL;
    service_reference_pt ref = NULL;
    hash_map_pt references = NULL;

    // Lock
	celixThreadMutex_lock(&registry->referencesMapMutex);

	references = hashMap_get(registry->serviceReferences, owner);
	if (references == NULL) {
        references = hashMap_create(NULL, NULL, NULL, NULL);
        if (references != NULL) {
            hashMap_put(registry->serviceReferences, owner, references);
        } else {
            status = CELIX_BUNDLE_EXCEPTION;
            framework_logIfError(logger, status, NULL, "Cannot create hash map");
        }
    }


    if (status == CELIX_SUCCESS) {
        ref = hashMap_get(references, registration);
        if (ref == NULL) {
            status = serviceRegistration_getBundle(registration, &bundle);
            if (status == CELIX_SUCCESS) {
                status = serviceReference_create(owner, registration, &ref);
            }
            if (status == CELIX_SUCCESS) {
                hashMap_put(references, registration, ref);
            }
        } else {
            serviceReference_retain(ref);
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = ref;
    }

    // Unlock
	celixThreadMutex_unlock(&registry->referencesMapMutex);

	framework_logIfError(logger, status, NULL, "Cannot create service reference");


	return status;
}

celix_status_t serviceRegistry_getServiceReferences(service_registry_pt registry, bundle_pt owner, const char *serviceName, filter_pt filter, array_list_pt *references) {
	celix_status_t status = CELIX_SUCCESS;
	hash_map_values_pt registrations;
	hash_map_iterator_pt iterator;
	arrayList_create(references);

	celixThreadMutex_lock(&registry->mutex);
	registrations = hashMapValues_create(registry->serviceRegistrations);
	iterator = hashMapValues_iterator(registrations);
	while (hashMapIterator_hasNext(iterator)) {
		array_list_pt regs = (array_list_pt) hashMapIterator_nextValue(iterator);
		unsigned int regIdx;
		for (regIdx = 0; (regs != NULL) && regIdx < arrayList_size(regs); regIdx++) {
			service_registration_pt registration = (service_registration_pt) arrayList_get(regs, regIdx);
			properties_pt props = NULL;

			status = serviceRegistration_getProperties(registration, &props);
			if (status == CELIX_SUCCESS) {
				bool matched = false;
				bool matchResult = false;
				if (filter != NULL) {
					filter_match(filter, props, &matchResult);
				}
				if ((serviceName == NULL) && ((filter == NULL) || matchResult)) {
					matched = true;
				} else if (serviceName != NULL) {
					char *className = NULL;
					bool matchResult = false;
					serviceRegistration_getServiceName(registration, &className);
					if (filter != NULL) {
						filter_match(filter, props, &matchResult);
					}
					if ((strcmp(className, serviceName) == 0) && ((filter == NULL) || matchResult)) {
						matched = true;
					}
				}
				if (matched) {
					if (serviceRegistration_isValid(registration)) {
						service_reference_pt reference = NULL;
                        serviceRegistry_getServiceReference(registry, owner, registration, &reference);
						arrayList_add(*references, reference);
					}
				}
			}
		}
	}
	hashMapIterator_destroy(iterator);
	hashMapValues_destroy(registrations);
	celixThreadMutex_unlock(&registry->mutex);

	framework_logIfError(logger, status, NULL, "Cannot get service references");

	return status;
}

celix_status_t serviceRegistry_ungetServiceReference(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    bool destroyed = false;
    size_t count = 0;
    service_registration_pt reg = NULL;

    celixThreadMutex_lock(&registry->mutex);
    serviceReference_getUsageCount(reference, &count);
    serviceReference_getServiceRegistration(reference, &reg);
    serviceReference_release(reference, &destroyed);
    if (destroyed) {
        if (count > 0) {
            serviceRegistry_logWarningServiceReferenceUsageCount(registry, 0, count);
        }
        hash_map_pt refsMap = hashMap_get(registry->serviceReferences, bundle);
        hashMap_remove(refsMap, reg);
    }
    celixThreadMutex_unlock(&registry->mutex);
    return status;
}

void serviceRegistry_logWarningServiceReferenceUsageCount(service_registry_pt registry, size_t usageCount, size_t refCount) {
    if (usageCount > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Service Reference destroyed will usage count is %zu. Look for missing bundleContext_ungetService calls.", usageCount);
    }
    if (refCount > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Dangling service reference. Reference count is %zu. Look for missing bundleContext_ungetServiceReference.", refCount);
    }
}


celix_status_t serviceRegistry_clearReferencesFor(service_registry_pt registry, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&registry->mutex);
    hash_map_pt refsMap = hashMap_remove(registry->serviceReferences, bundle);
    celixThreadMutex_unlock(&registry->mutex);

    if (refsMap != NULL) {
        hash_map_iterator_pt iter = hashMapIterator_create(refsMap);
        while (hashMapIterator_hasNext(iter)) {
            service_reference_pt ref = hashMapIterator_nextValue(iter);
            size_t refCount;
            size_t usageCount;

            serviceReference_getUsageCount(ref, &usageCount);
            serviceReference_getReferenceCount(ref, &refCount);

            serviceRegistry_logWarningServiceReferenceUsageCount(registry, usageCount, refCount);

            while (usageCount > 0) {
                serviceReference_decreaseUsage(ref, &usageCount);
            }

            bool destroyed = false;
            while (!destroyed) {
                serviceReference_release(ref, &destroyed);
            }

        }
        hashMapIterator_destroy(iter);
    }

    return status;
}


celix_status_t serviceRegistry_getServicesInUse(service_registry_pt registry, bundle_pt bundle, array_list_pt *out) {

    array_list_pt result = NULL;
    arrayList_create(&result);

    //LOCK
    celixThreadMutex_lock(&registry->mutex);

    hash_map_pt refsMap = hashMap_get(registry->serviceReferences, bundle);

    hash_map_iterator_pt iter = hashMapIterator_create(refsMap);
    while (hashMapIterator_hasNext(iter)) {
        service_reference_pt ref = hashMapIterator_nextValue(iter);
        arrayList_add(result, ref);
    }
    hashMapIterator_destroy(iter);

    //UNLOCK
	celixThreadMutex_unlock(&registry->mutex);

    *out = result;

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, void **out) {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
    size_t count = 0;
    void *service = NULL;

	serviceReference_getServiceRegistration(reference, &registration);
	celixThreadMutex_lock(&registry->mutex);


	if (serviceRegistration_isValid(registration)) {
        serviceReference_increaseUsage(reference, &count);
        if (count == 1) {
            serviceRegistration_getService(registration, bundle, &service);
            serviceReference_setService(reference, service);
        }

        serviceReference_getService(reference, out);
	} else {
        *out = NULL; //invalid service registration
    }

	celixThreadMutex_unlock(&registry->mutex);


	return status;
}

celix_status_t serviceRegistry_ungetService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, bool *result) {
	celix_status_t status = CELIX_SUCCESS;
    service_registration_pt reg = NULL;
    void *service = NULL;
    size_t count = 0;

    celix_status_t subStatus = serviceReference_decreaseUsage(reference, &count);

    if (count == 0) {
        serviceReference_getService(reference, &service);
        serviceReference_getServiceRegistration(reference, &reg);
        serviceRegistration_ungetService(reg, bundle, &service);
    }

    if (result) {
        *result = (subStatus == CELIX_SUCCESS);
    }

	return status;
}

celix_status_t serviceRegistry_addHooks(service_registry_pt registry, char *serviceName, void *serviceObject, service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
	    celixThreadMutex_lock(&registry->mutex);
		arrayList_add(registry->listenerHooks, registration);
		celixThreadMutex_unlock(&registry->mutex);
	}

	return status;
}

celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;
	char *serviceName = NULL;

	properties_pt props = NULL;
	serviceRegistration_getProperties(registration, &props);
	serviceName = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
	if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
	    celixThreadMutex_lock(&registry->mutex);
		arrayList_removeElement(registry->listenerHooks, registration);
		celixThreadMutex_unlock(&registry->mutex);
	}

	return status;
}

celix_status_t serviceRegistry_getListenerHooks(service_registry_pt registry, bundle_pt owner, array_list_pt *hooks) {
	celix_status_t status = CELIX_SUCCESS;

	if (registry == NULL || *hooks != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		status = arrayList_create(hooks);
		if (status == CELIX_SUCCESS) {
			unsigned int i;
			celixThreadMutex_lock(&registry->mutex);
			for (i = 0; i < arrayList_size(registry->listenerHooks); i++) {
				service_registration_pt registration = arrayList_get(registry->listenerHooks, i);
				service_reference_pt reference = NULL;
                serviceRegistry_getServiceReference(registry, owner, registration, &reference);
				arrayList_add(*hooks, reference);
			}
			celixThreadMutex_unlock(&registry->mutex);
		}
	}

	framework_logIfError(logger, status, NULL, "Cannot get listener hooks");

	return status;
}

celix_status_t serviceRegistry_servicePropertiesModified(service_registry_pt registry, service_registration_pt registration, properties_pt oldprops) {
	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED, registration, oldprops);
	}

	return CELIX_SUCCESS;
}
