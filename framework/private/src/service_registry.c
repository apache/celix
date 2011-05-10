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

struct usageCount {
	unsigned int count;
	SERVICE_REFERENCE reference;
	void * service;
};

typedef struct usageCount * USAGE_COUNT;

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
		usages = arrayList_create();
		MODULE mod = bundle_getCurrentModule(bundle);
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

SERVICE_REGISTRY serviceRegistry_create(void (*serviceChanged)(SERVICE_EVENT, PROPERTIES)) {
	SERVICE_REGISTRY registry = (SERVICE_REGISTRY) malloc(sizeof(*registry));
	registry->serviceChanged = serviceChanged;
	registry->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
	registry->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&registry->mutex, &mutexattr);
	registry->currentServiceId = 1l;
	return registry;
}

ARRAY_LIST serviceRegistry_getRegisteredServices(SERVICE_REGISTRY registry, BUNDLE bundle) {
	ARRAY_LIST regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		ARRAY_LIST refs = arrayList_create();
		int i;
		for (i = 0; i < arrayList_size(regs); i++) {
			SERVICE_REGISTRATION reg = arrayList_get(regs, i);
			if (serviceRegistration_isValid(reg)) {
				arrayList_add(refs, reg->reference);
			}
		}
		return refs;
	}
	return NULL;
}

SERVICE_REGISTRATION serviceRegistry_registerService(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, void * serviceObject, PROPERTIES dictionary) {
	SERVICE_REGISTRATION reg = NULL;

	pthread_mutex_lock(&registry->mutex);

	reg = serviceRegistration_create(registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);

	ARRAY_LIST regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = arrayList_create();
	}
	arrayList_add(regs, reg);
	hashMap_put(registry->serviceRegistrations, bundle, regs);

	pthread_mutex_unlock(&registry->mutex);

	if (registry->serviceChanged != NULL) {
		SERVICE_EVENT event = (SERVICE_EVENT) malloc(sizeof(*event));
		event->type = REGISTERED;
		event->reference = reg->reference;
		registry->serviceChanged(event, NULL);
		free(event);
		event = NULL;
	}

	return reg;
}

void serviceRegistry_unregisterService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REGISTRATION registration) {
	pthread_mutex_lock(&registry->mutex);

	ARRAY_LIST regs = (ARRAY_LIST) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
		hashMap_put(registry->serviceRegistrations, bundle, regs);
	}

	pthread_mutex_unlock(&registry->mutex);

	if (registry->serviceChanged != NULL) {
		SERVICE_EVENT event = (SERVICE_EVENT) malloc(sizeof(*event));
		event->type = UNREGISTERING;
		event->reference = registration->reference;
		registry->serviceChanged(event, NULL);
		free(event);
	}

	pthread_mutex_lock(&registry->mutex);
	// unget service

	ARRAY_LIST clients = serviceRegistry_getUsingBundles(registry, registration->reference);
	int i;
	for (i = 0; (clients != NULL) && (i < arrayList_size(clients)); i++) {
		BUNDLE client = arrayList_get(clients, i);
		while (serviceRegistry_ungetService(registry, client, registration->reference)) {
			;
		}
	}
	arrayList_destroy(clients);
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

ARRAY_LIST serviceRegistry_getServiceReferences(SERVICE_REGISTRY registry, char * serviceName, FILTER filter) {
	ARRAY_LIST references = arrayList_create();

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
					arrayList_add(references, registration->reference);
				}
			}
		}
	}
	hashMapIterator_destroy(iterator);
	hashMapValues_destroy(registrations);

	return references;
}

ARRAY_LIST serviceRegistry_getServicesInUse(SERVICE_REGISTRY registry, BUNDLE bundle) {
	ARRAY_LIST usages = hashMap_get(registry->inUseMap, bundle);
	if (usages != NULL) {
		ARRAY_LIST references = arrayList_create();
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
	SERVICE_REGISTRATION registration = reference->registration;
	void * service = NULL;
	USAGE_COUNT usage = NULL;

	pthread_mutex_lock(&registry->mutex);

	if (registration->svcObj != NULL) {
		usage = serviceRegistry_getUsageCount(registry, bundle, reference);
		if (usage == NULL) {
			usage = serviceRegistry_addUsageCount(registry, bundle, reference);
		}
		usage->count++;
		service = usage->service;
	}
	pthread_mutex_unlock(&registry->mutex);

	if ((usage != NULL) && (service == NULL)) {
		service = registration->svcObj;
	}
	pthread_mutex_lock(&registry->mutex);
	if ((registration->svcObj == NULL) || (service == NULL)) {
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	} else {
		usage->service = service;
	}
	pthread_mutex_unlock(&registry->mutex);

	return service;
}

bool serviceRegistry_ungetService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference) {
	SERVICE_REGISTRATION registration = reference->registration;
	USAGE_COUNT usage = NULL;

	pthread_mutex_lock(&registry->mutex);

	usage = serviceRegistry_getUsageCount(registry, bundle, reference);
	if (usage == NULL) {
		pthread_mutex_unlock(&registry->mutex);
		return false;
	}

	usage->count--;


	if ((registration->svcObj == NULL) || (usage->count <= 0)) {
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
	ARRAY_LIST fusages = arrayList_clone(usages);

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

ARRAY_LIST serviceRegistry_getUsingBundles(SERVICE_REGISTRY registry, SERVICE_REFERENCE reference) {
	ARRAY_LIST bundles = arrayList_create();
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
