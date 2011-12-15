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
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>

#include "service_registry.h"
#include "service_registration.h"
#include "module.h"
#include "bundle.h"
#include "listener_hook_service.h"
#include "constants.h"
#include "service_reference.h"

struct serviceRegistry {
    FRAMEWORK framework;
	HASH_MAP serviceRegistrations;
	HASH_MAP serviceReferences;
	HASH_MAP inUseMap;
	void (*serviceChanged)(FRAMEWORK, SERVICE_EVENT_TYPE, SERVICE_REGISTRATION, PROPERTIES);
	long currentServiceId;

	ARRAY_LIST listenerHooks;

	pthread_mutex_t mutex;
};

struct usageCount {
	unsigned int count;
	SERVICE_REFERENCE reference;
	void * service;
};

typedef struct usageCount * USAGE_COUNT;

celix_status_t serviceRegistry_registerServiceInternal(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, void * serviceObject, PROPERTIES dictionary, bool isFactory, SERVICE_REGISTRATION *registration);
celix_status_t serviceRegistry_addHooks(SERVICE_REGISTRY registry, char *serviceName, void *serviceObject, SERVICE_REGISTRATION registration);
celix_status_t serviceRegistry_removeHook(SERVICE_REGISTRY registry, SERVICE_REGISTRATION registration);

apr_status_t serviceRegistry_removeReference(void *referenceP);

USAGE_COUNT serviceRegistry_getUsageCount(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference) {
	ARRAY_LIST usages = hashMap_get(registry->inUseMap, bundle);
	int i;
	for (i = 0; (usages != NULL) && (i < arrayList_size(usages)); i++) {
		USAGE_COUNT usage = arrayList_get(usages, i);
		if (usage->reference == reference) {
			return usage;
		}
	}
	return NULL;
}

USAGE_COUNT serviceRegistry_addUsageCount(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference) {
	ARRAY_LIST usages = hashMap_get(registry->inUseMap, bundle);
	USAGE_COUNT usage = (USAGE_COUNT) malloc(sizeof(*usage));
	usage->reference = reference;
	usage->count = 0;
	usage->service = NULL;

	if (usages == NULL) {
		arrayList_create(bundle->memoryPool, &usages);
		MODULE mod = NULL;
		bundle_getCurrentModule(bundle, &mod);
	}
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);
	return usage;
}

void serviceRegistry_flushUsageCount(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference) {
	ARRAY_LIST usages = hashMap_get(registry->inUseMap, bundle);
	ARRAY_LIST_ITERATOR iter = arrayListIterator_create(usages);
	while (arrayListIterator_hasNext(iter)) {
		USAGE_COUNT usage = arrayListIterator_next(iter);
		if (usage->reference == reference) {
			arrayListIterator_remove(iter);
			free(usage);
		}
	}
	arrayListIterator_destroy(iter);
	if (arrayList_size(usages) > 0) {
		hashMap_put(registry->inUseMap, bundle, usages);
	} else {
		ARRAY_LIST removed = hashMap_remove(registry->inUseMap, bundle);
		arrayList_destroy(removed);
	}
}

SERVICE_REGISTRY serviceRegistry_create(FRAMEWORK framework, void (*serviceChanged)(FRAMEWORK, SERVICE_EVENT_TYPE, SERVICE_REGISTRATION, PROPERTIES)) {
	SERVICE_REGISTRY registry;

	registry = (SERVICE_REGISTRY) apr_palloc(framework->mp, (sizeof(*registry)));
	if (registry == NULL) {
	    // no memory
	} else {
        registry->serviceChanged = serviceChanged;
        registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
        registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
        registry->framework = framework;

        arrayList_create(framework->mp, &registry->listenerHooks);

        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&registry->mutex, &mutexattr);
        registry->currentServiceId = 1l;
	}
	return registry;
}

celix_status_t serviceRegistry_destroy(SERVICE_REGISTRY registry) {
    hashMap_destroy(registry->inUseMap, false, false);
    hashMap_destroy(registry->serviceRegistrations, false, false);
    arrayList_destroy(registry->listenerHooks);
    pthread_mutex_destroy(&registry->mutex);

    return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getRegisteredServices(SERVICE_REGISTRY registry, apr_pool_t *pool, BUNDLE bundle, ARRAY_LIST *services) {
	celix_status_t status = CELIX_SUCCESS;

	ARRAY_LIST regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_create(pool, services);
		int i;
		for (i = 0; i < arrayList_size(regs); i++) {
			SERVICE_REGISTRATION reg = arrayList_get(regs, i);
			if (serviceRegistration_isValid(reg)) {
				// #todo Create SERVICE_REFERECEN for each registration
				SERVICE_REFERENCE reference = NULL;
				serviceRegistry_createServiceReference(registry, pool, reg, &reference);
				arrayList_add(*services, reference);
			}
		}
		return status;
	}
	return status;
}

SERVICE_REGISTRATION serviceRegistry_registerService(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, void * serviceObject, PROPERTIES dictionary) {
    SERVICE_REGISTRATION registration = NULL;
    serviceRegistry_registerServiceInternal(registry, bundle, serviceName, serviceObject, dictionary, false, &registration);
    return registration;
}

SERVICE_REGISTRATION serviceRegistry_registerServiceFactory(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, service_factory_t factory, PROPERTIES dictionary) {
    SERVICE_REGISTRATION registration = NULL;
    serviceRegistry_registerServiceInternal(registry, bundle, serviceName, (void *) factory, dictionary, true, &registration);
    return registration;
}

celix_status_t serviceRegistry_registerServiceInternal(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, void * serviceObject, PROPERTIES dictionary, bool isFactory, SERVICE_REGISTRATION *registration) {
	pthread_mutex_lock(&registry->mutex);

	apr_pool_t *pool = bundle->memoryPool;

	if (isFactory) {
	    *registration = serviceRegistration_createServiceFactory(pool, registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	} else {
	    *registration = serviceRegistration_create(pool, registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	}

	serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);

	ARRAY_LIST regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = NULL;
		arrayList_create(bundle->memoryPool, &regs);
	}
	arrayList_add(regs, *registration);
	hashMap_put(registry->serviceRegistrations, bundle, regs);

	pthread_mutex_unlock(&registry->mutex);

	if (registry->serviceChanged != NULL) {
//		SERVICE_EVENT event = (SERVICE_EVENT) malloc(sizeof(*event));
//		event->type = REGISTERED;
//		event->reference = (*registration)->reference;
		registry->serviceChanged(registry->framework, REGISTERED, *registration, NULL);
//		free(event);
//		event = NULL;
	}

	return CELIX_SUCCESS;
}

void serviceRegistry_unregisterService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REGISTRATION registration) {
	pthread_mutex_lock(&registry->mutex);

	serviceRegistry_removeHook(registry, registration);

	ARRAY_LIST regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
		hashMap_put(registry->serviceRegistrations, bundle, regs);
	}

	pthread_mutex_unlock(&registry->mutex);

	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, UNREGISTERING, registration, NULL);
	}

	pthread_mutex_lock(&registry->mutex);
	// unget service

	int i;
	for (i = 0; i < arrayList_size(registration->references); i++) {
		SERVICE_REFERENCE reference = arrayList_get(registration->references, i);
		ARRAY_LIST clients = serviceRegistry_getUsingBundles(registry, registry->framework->mp, reference);
		int j;
		for (j = 0; (clients != NULL) && (j < arrayList_size(clients)); j++) {
			BUNDLE client = arrayList_get(clients, j);
			while (serviceRegistry_ungetService(registry, client, reference)) {
				;
			}
		}
		arrayList_destroy(clients);

		serviceReference_invalidate(reference);
	}
	serviceRegistration_invalidate(registration);

	serviceRegistration_destroy(registration);

	pthread_mutex_unlock(&registry->mutex);

}

void serviceRegistry_unregisterServices(SERVICE_REGISTRY registry, BUNDLE bundle) {
	ARRAY_LIST regs = NULL;
	pthread_mutex_lock(&registry->mutex);
	regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	pthread_mutex_unlock(&registry->mutex);

	int i;
	for (i = 0; (regs != NULL) && i < arrayList_size(regs); i++) {
		SERVICE_REGISTRATION reg = arrayList_get(regs, i);
		if (serviceRegistration_isValid(reg)) {
			serviceRegistration_unregister(reg);
		}
	}

	if (regs != NULL && arrayList_isEmpty(regs)) {
		ARRAY_LIST removed = hashMap_remove(registry->serviceRegistrations, bundle);
		arrayList_destroy(removed);
		removed = NULL;
	}

	pthread_mutex_lock(&registry->mutex);
	hashMap_remove(registry->serviceRegistrations, bundle);
	pthread_mutex_unlock(&registry->mutex);
}

celix_status_t serviceRegistry_createServiceReference(SERVICE_REGISTRY registry, apr_pool_t *pool, SERVICE_REGISTRATION registration, SERVICE_REFERENCE *reference) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *spool = NULL;
	apr_pool_create(&spool, pool);

	serviceReference_create(spool, registration->bundle, registration, reference);

	apr_pool_pre_cleanup_register(spool, *reference, serviceRegistry_removeReference);

	arrayList_add(registration->references, *reference);

	return status;
}

celix_status_t serviceRegistry_getServiceReferences(SERVICE_REGISTRY registry, apr_pool_t *pool, const char *serviceName, FILTER filter, ARRAY_LIST *references) {
	celix_status_t status = CELIX_SUCCESS;

	arrayList_create(pool, references);

	HASH_MAP_VALUES registrations = hashMapValues_create(registry->serviceRegistrations);
	HASH_MAP_ITERATOR iterator = hashMapValues_iterator(registrations);
	while (hashMapIterator_hasNext(iterator)) {
		ARRAY_LIST regs = (ARRAY_LIST) hashMapIterator_nextValue(iterator);
		int regIdx;
		for (regIdx = 0; (regs != NULL) && regIdx < arrayList_size(regs); regIdx++) {
			SERVICE_REGISTRATION registration = (SERVICE_REGISTRATION) arrayList_get(regs, regIdx);

			bool matched = false;
			if ((serviceName == NULL) && ((filter == NULL) || filter_match(filter, registration->properties))) {
				matched = true;
			} else if (serviceName != NULL) {
				if ((strcmp(registration->className, serviceName) == 0) && ((filter == NULL) || filter_match(filter, registration->properties))) {
					matched = true;
				}
			}
			if (matched) {
				if (serviceRegistration_isValid(registration)) {
					SERVICE_REFERENCE reference = NULL;
					serviceRegistry_createServiceReference(registry, pool, registration, &reference);
					arrayList_add(*references, reference);
				}
			}
		}
	}
	hashMapIterator_destroy(iterator);
	hashMapValues_destroy(registrations);

	return status;
}

apr_status_t serviceRegistry_removeReference(void *referenceP) {
	SERVICE_REFERENCE reference = referenceP;
	SERVICE_REGISTRATION registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);

	if (registration != NULL) {
		printf("SERVICE_REGISTRY: Destroy: %s\n", registration->className);
		arrayList_removeElement(registration->references, reference);
	}

	return APR_SUCCESS;
}

//ARRAY_LIST serviceRegistry_getServiceReferences(SERVICE_REGISTRY registry, const char * serviceName, FILTER filter) {
//	ARRAY_LIST references = NULL;
//	arrayList_create(registry->framework->mp, &references);
//
//	HASH_MAP_VALUES registrations = hashMapValues_create(registry->serviceRegistrations);
//	HASH_MAP_ITERATOR iterator = hashMapValues_iterator(registrations);
//	while (hashMapIterator_hasNext(iterator)) {
//		ARRAY_LIST regs = (ARRAY_LIST) hashMapIterator_nextValue(iterator);
//		int regIdx;
//		for (regIdx = 0; (regs != NULL) && regIdx < arrayList_size(regs); regIdx++) {
//			SERVICE_REGISTRATION registration = (SERVICE_REGISTRATION) arrayList_get(regs, regIdx);
//
//			bool matched = false;
//			if ((serviceName == NULL) && ((filter == NULL) || filter_match(filter, registration->properties))) {
//				matched = true;
//			} else if (serviceName != NULL) {
//				if ((strcmp(registration->className, serviceName) == 0) && ((filter == NULL) || filter_match(filter, registration->properties))) {
//					matched = true;
//				}
//			}
//			if (matched) {
//				if (serviceRegistration_isValid(registration)) {
//					arrayList_add(references, registration->reference);
//				}
//			}
//		}
//	}
//	hashMapIterator_destroy(iterator);
//	hashMapValues_destroy(registrations);
//
//	return references;
//}

ARRAY_LIST serviceRegistry_getServicesInUse(SERVICE_REGISTRY registry, BUNDLE bundle) {
	ARRAY_LIST usages = hashMap_get(registry->inUseMap, bundle);
	if (usages != NULL) {
		ARRAY_LIST references = NULL;
		arrayList_create(bundle->memoryPool, &references);
		int i;
		for (i = 0; i < arrayList_size(usages); i++) {
			USAGE_COUNT usage = arrayList_get(usages, i);
			arrayList_add(references, usage->reference);
		}
		return references;
	}
	return NULL;
}

void * serviceRegistry_getService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference) {
	SERVICE_REGISTRATION registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	void * service = NULL;
	USAGE_COUNT usage = NULL;

	pthread_mutex_lock(&registry->mutex);

	if (serviceRegistration_isValid(registration)) {
		usage = serviceRegistry_getUsageCount(registry, bundle, reference);
		if (usage == NULL) {
			usage = serviceRegistry_addUsageCount(registry, bundle, reference);
		}
		usage->count++;
		service = usage->service;
	}
	pthread_mutex_unlock(&registry->mutex);

	if ((usage != NULL) && (service == NULL)) {
		serviceRegistration_getService(registration, bundle, &service);
	}
	pthread_mutex_lock(&registry->mutex);
	if ((!serviceRegistration_isValid(registration)) || (service == NULL)) {
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	} else {
		usage->service = service;
	}
	pthread_mutex_unlock(&registry->mutex);

	return service;
}

bool serviceRegistry_ungetService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference) {
	SERVICE_REGISTRATION registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	USAGE_COUNT usage = NULL;

	pthread_mutex_lock(&registry->mutex);

	usage = serviceRegistry_getUsageCount(registry, bundle, reference);
	if (usage == NULL) {
		pthread_mutex_unlock(&registry->mutex);
		return false;
	}

	usage->count--;


	if ((serviceRegistration_isValid(registration)) || (usage->count <= 0)) {
		usage->service = NULL;
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	}

	pthread_mutex_unlock(&registry->mutex);

	return true;
}

void serviceRegistry_ungetServices(SERVICE_REGISTRY registry, BUNDLE bundle) {
	pthread_mutex_lock(&registry->mutex);
	ARRAY_LIST usages = hashMap_get(registry->inUseMap, bundle);
	pthread_mutex_unlock(&registry->mutex);

	if (usages == NULL || arrayList_isEmpty(usages)) {
		return;
	}

	// usage arrays?
	ARRAY_LIST fusages = arrayList_clone(bundle->memoryPool, usages);

	int i;
	for (i = 0; i < arrayList_size(fusages); i++) {
		USAGE_COUNT usage = arrayList_get(fusages, i);
		SERVICE_REFERENCE reference = usage->reference;
		while (serviceRegistry_ungetService(registry, bundle, reference)) {
			//
		}
	}

	arrayList_destroy(fusages);
}

ARRAY_LIST serviceRegistry_getUsingBundles(SERVICE_REGISTRY registry, apr_pool_t *pool, SERVICE_REFERENCE reference) {
	ARRAY_LIST bundles = NULL;
	apr_pool_t *npool;
	apr_pool_create(&npool, pool);
	arrayList_create(npool, &bundles);
	HASH_MAP_ITERATOR iter = hashMapIterator_create(registry->inUseMap);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		BUNDLE bundle = hashMapEntry_getKey(entry);
		ARRAY_LIST usages = hashMapEntry_getValue(entry);
		int i;
		for (i = 0; i < arrayList_size(usages); i++) {
			USAGE_COUNT usage = arrayList_get(usages, i);
			if (usage->reference == reference) {
				arrayList_add(bundles, bundle);
			}
		}
	}
	hashMapIterator_destroy(iter);
	return bundles;
}

celix_status_t serviceRegistry_addHooks(SERVICE_REGISTRY registry, char *serviceName, void *serviceObject, SERVICE_REGISTRATION registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (strcmp(listener_hook_service_name, serviceName) == 0) {
		arrayList_add(registry->listenerHooks, registration);
	}

	return status;
}

celix_status_t serviceRegistry_removeHook(SERVICE_REGISTRY registry, SERVICE_REGISTRATION registration) {
	celix_status_t status = CELIX_SUCCESS;

	char *serviceName = properties_get(registration->properties, (char *) OBJECTCLASS);
	if (strcmp(listener_hook_service_name, serviceName) == 0) {
		arrayList_removeElement(registry->listenerHooks, registration);
	}

	return status;
}

celix_status_t serviceRegistry_getListenerHooks(SERVICE_REGISTRY registry, apr_pool_t *pool, ARRAY_LIST *hooks) {
	celix_status_t status = CELIX_SUCCESS;

	if (registry == NULL || *hooks != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		status = arrayList_create(pool, hooks);
		if (status == CELIX_SUCCESS) {
			int i;
			for (i = 0; i < arrayList_size(registry->listenerHooks); i++) {
				SERVICE_REGISTRATION registration = arrayList_get(registry->listenerHooks, i);
				SERVICE_REFERENCE reference = NULL;
				serviceRegistry_createServiceReference(registry, pool, registration, &reference);
				arrayList_add(*hooks, reference);
			}
		}
	}

	return status;
}
