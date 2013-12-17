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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "service_registry_private.h"
#include "service_registration.h"
#include "module.h"
#include "bundle.h"
#include "listener_hook_service.h"
#include "constants.h"
#include "service_reference.h"
#include "framework_private.h"
#include "celix_log.h"

apr_status_t serviceRegistry_destroy(void *handle);

celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, char * serviceName, void * serviceObject, properties_pt dictionary, bool isFactory, service_registration_pt *registration);
celix_status_t serviceRegistry_addHooks(service_registry_pt registry, char *serviceName, void *serviceObject, service_registration_pt registration);
celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration);

apr_status_t serviceRegistry_removeReference(void *referenceP);

celix_status_t serviceRegistry_create(apr_pool_t *ppool, framework_pt framework, serviceChanged_function_pt serviceChanged, service_registry_pt *registry) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool = NULL;

	apr_pool_create(&pool, ppool);

	*registry = (service_registry_pt) apr_palloc(pool, (sizeof(**registry)));
	if (!*registry) {
		status = CELIX_ENOMEM;
	} else {
		apr_status_t aprStatus;

		apr_pool_pre_cleanup_register(pool, *registry, serviceRegistry_destroy);

		(*registry)->pool=pool;
		(*registry)->serviceChanged = serviceChanged;
		(*registry)->inUseMap = hashMap_create(NULL, NULL, NULL, NULL);
		(*registry)->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
		(*registry)->framework = framework;
		(*registry)->currentServiceId = 1l;

		arrayList_create(&(*registry)->listenerHooks);
		aprStatus = apr_thread_mutex_create(&(*registry)->mutex, APR_THREAD_MUTEX_NESTED, pool);
		if (aprStatus != APR_SUCCESS) {
			status = CELIX_FRAMEWORK_EXCEPTION;
		}
	}

	framework_logIfError(status, NULL, "Cannot create service registry");

	return status;
}

apr_status_t serviceRegistry_destroy(void *handle) {
	service_registry_pt registry = (service_registry_pt) handle;
    hashMap_destroy(registry->inUseMap, false, false);
    hashMap_destroy(registry->serviceRegistrations, false, false);
    arrayList_destroy(registry->listenerHooks);
    apr_thread_mutex_destroy(registry->mutex);
    registry->framework = NULL;
    registry->inUseMap = NULL;
    registry->listenerHooks = NULL;
    registry->mutex = NULL;
    registry->pool = NULL;
    registry->serviceChanged = NULL;
    registry->serviceReferences = NULL;
    registry->serviceRegistrations = NULL;

    return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getUsageCount(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, usage_count_pt *usageCount) {
	array_list_pt usages = (array_list_pt) hashMap_get(registry->inUseMap, bundle);
	*usageCount = NULL;
	unsigned int i;
	for (i = 0; (usages != NULL) && (i < arrayList_size(usages)); i++) {
		usage_count_pt usage = (usage_count_pt) arrayList_get(usages, i);
		// @TODO use reference_equals!
		bool equals = false;
//		serviceReference_equals(usage->reference, reference, &equals);
		if (usage->reference == reference) {
			*usageCount = usage;
			break;
		}
		if (equals) {
			*usageCount = usage;
			break;
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_addUsageCount(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, usage_count_pt *usageCount) {
	apr_pool_t *pool = NULL;

	apr_pool_create(&pool, registry->pool);

	array_list_pt usages = hashMap_get(registry->inUseMap, bundle);
	usage_count_pt usage = (usage_count_pt) apr_palloc(pool, sizeof(*usage));
	usage->reference = reference;
	usage->count = 0;
	usage->service = NULL;
	usage->pool = pool;

	if (usages == NULL) {
		module_pt mod = NULL;
		arrayList_create(&usages);
		bundle_getCurrentModule(bundle, &mod);
	}
	arrayList_add(usages, usage);
	hashMap_put(registry->inUseMap, bundle, usages);
	*usageCount = usage;
	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_flushUsageCount(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference) {
	array_list_pt usages = hashMap_get(registry->inUseMap, bundle);
	if (usages != NULL) {
		array_list_iterator_pt iter = arrayListIterator_create(usages);
		while (arrayListIterator_hasNext(iter)) {
			usage_count_pt usage = arrayListIterator_next(iter);
			if (usage->reference == reference) {
				arrayListIterator_remove(iter);
				apr_pool_destroy(usage->pool);
			}
		}
		arrayListIterator_destroy(iter);
		if (arrayList_size(usages) > 0) {
			hashMap_put(registry->inUseMap, bundle, usages);
		} else {
			array_list_pt removed = hashMap_remove(registry->inUseMap, bundle);
			arrayList_destroy(removed);
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getRegisteredServices(service_registry_pt registry, apr_pool_t *pool, bundle_pt bundle, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		unsigned int i;
		arrayList_create(services);
		
		for (i = 0; i < arrayList_size(regs); i++) {
			service_registration_pt reg = arrayList_get(regs, i);
			if (serviceRegistration_isValid(reg)) {
				service_reference_pt reference = NULL;
				status = serviceRegistry_createServiceReference(registry, pool, reg, &reference);
				if (status == CELIX_SUCCESS) {
					arrayList_add(*services, reference);
				}
			}
		}
	}

	framework_logIfError(status, NULL, "Cannot get registered services");

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
	apr_pool_t *pool = NULL;
	apr_thread_mutex_lock(registry->mutex);

	bundle_getMemoryPool(bundle, &pool);

	if (isFactory) {
	    *registration = serviceRegistration_createServiceFactory(pool, registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	} else {
	    *registration = serviceRegistration_create(pool, registry, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	}

	serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);

	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = NULL;
		arrayList_create(&regs);
	}
	arrayList_add(regs, *registration);
	hashMap_put(registry->serviceRegistrations, bundle, regs);

	apr_thread_mutex_unlock(registry->mutex);

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
	unsigned int i;
	array_list_pt regs;
	array_list_pt references = NULL;

	apr_thread_mutex_lock(registry->mutex);

	serviceRegistry_removeHook(registry, registration);

	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
		hashMap_put(registry->serviceRegistrations, bundle, regs);
	}

	apr_thread_mutex_unlock(registry->mutex);

	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING, registration, NULL);
	}

	apr_thread_mutex_lock(registry->mutex);
	// unget service

	serviceRegistration_getServiceReferences(registration, &references);
	for (i = 0; i < arrayList_size(references); i++) {
		service_reference_pt reference = (service_reference_pt) arrayList_get(references, i);
		apr_pool_t *pool = NULL;
		array_list_pt clients = NULL;
		unsigned int j;

		apr_pool_create(&pool, registry->pool);
		clients = serviceRegistry_getUsingBundles(registry, pool, reference);
		for (j = 0; (clients != NULL) && (j < arrayList_size(clients)); j++) {
			bundle_pt client = (bundle_pt) arrayList_get(clients, j);
			bool ungetResult = true;
			while (ungetResult) {
				serviceRegistry_ungetService(registry, client, reference, &ungetResult);
			}
		}
		arrayList_destroy(clients);
		apr_pool_destroy(pool);

		serviceReference_invalidate(reference);
	}
	serviceRegistration_invalidate(registration);

	serviceRegistration_destroy(registration);

	apr_thread_mutex_unlock(registry->mutex);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_unregisterServices(service_registry_pt registry, bundle_pt bundle) {
	array_list_pt regs = NULL;
	unsigned int i;
	apr_thread_mutex_lock(registry->mutex);
	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	apr_thread_mutex_unlock(registry->mutex);
	
	for (i = 0; (regs != NULL) && i < arrayList_size(regs); i++) {
		service_registration_pt reg = arrayList_get(regs, i);
		if (serviceRegistration_isValid(reg)) {
			serviceRegistration_unregister(reg);
		}
	}

	if (regs != NULL && arrayList_isEmpty(regs)) {
		array_list_pt removed = hashMap_remove(registry->serviceRegistrations, bundle);
		arrayList_destroy(removed);
		removed = NULL;
	}

	apr_thread_mutex_lock(registry->mutex);
	hashMap_remove(registry->serviceRegistrations, bundle);
	apr_thread_mutex_unlock(registry->mutex);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_createServiceReference(service_registry_pt registry, apr_pool_t *pool, service_registration_pt registration, service_reference_pt *reference) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_pt bundle = NULL;
	array_list_pt references = NULL;

	apr_pool_t *spool = NULL;
	apr_pool_create(&spool, pool);

	serviceRegistration_getBundle(registration, &bundle);
	serviceReference_create(spool, bundle, registration, reference);

	apr_pool_pre_cleanup_register(spool, *reference, serviceRegistry_removeReference);

	serviceRegistration_getServiceReferences(registration, &references);
	arrayList_add(references, *reference);

	framework_logIfError(status, NULL, "Cannot create service reference");

	return status;
}

celix_status_t serviceRegistry_getServiceReferences(service_registry_pt registry, apr_pool_t *pool, const char *serviceName, filter_pt filter, array_list_pt *references) {
	celix_status_t status = CELIX_SUCCESS;
	hash_map_values_pt registrations;
	hash_map_iterator_pt iterator;
	arrayList_create(references);

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
						serviceRegistry_createServiceReference(registry, pool, registration, &reference);
						arrayList_add(*references, reference);
					}
				}
			}
		}
	}
	hashMapIterator_destroy(iterator);
	hashMapValues_destroy(registrations);

	framework_logIfError(status, NULL, "Cannot get service references");

	return status;
}

apr_status_t serviceRegistry_removeReference(void *referenceP) {
	service_reference_pt reference = referenceP;
	service_registration_pt registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);

	if (registration != NULL) {
		array_list_pt references = NULL;
		serviceRegistration_getServiceReferences(registration, &references);
		arrayList_removeElement(references, reference);
	}

	return APR_SUCCESS;
}

celix_status_t serviceRegistry_getServicesInUse(service_registry_pt registry, bundle_pt bundle, array_list_pt *services) {
	array_list_pt usages = hashMap_get(registry->inUseMap, bundle);
	if (usages != NULL) {
		unsigned int i;
		arrayList_create(services);
		
		for (i = 0; i < arrayList_size(usages); i++) {
			usage_count_pt usage = arrayList_get(usages, i);
			arrayList_add(*services, usage->reference);
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
	*service = NULL;
	usage_count_pt usage = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	
	apr_thread_mutex_lock(registry->mutex);

	if (serviceRegistration_isValid(registration)) {
		status = serviceRegistry_getUsageCount(registry, bundle, reference, &usage);
		if (usage == NULL) {
			status = serviceRegistry_addUsageCount(registry, bundle, reference, &usage);
		}
		usage->count++;
		*service = usage->service;
	}
	apr_thread_mutex_unlock(registry->mutex);

	if ((usage != NULL) && (*service == NULL)) {
		serviceRegistration_getService(registration, bundle, service);
	}
	apr_thread_mutex_lock(registry->mutex);
	if ((!serviceRegistration_isValid(registration)) || (*service == NULL)) {
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	} else {
		usage->service = *service;
	}
	apr_thread_mutex_unlock(registry->mutex);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_ungetService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, bool *result) {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
	usage_count_pt usage = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	
	apr_thread_mutex_lock(registry->mutex);

	status = serviceRegistry_getUsageCount(registry, bundle, reference, &usage);
	if (usage == NULL) {
		apr_thread_mutex_unlock(registry->mutex);
		*result = false;
		return CELIX_SUCCESS;
	}

	usage->count--;


	if ((serviceRegistration_isValid(registration)) || (usage->count <= 0)) {
		usage->service = NULL;
		serviceRegistry_flushUsageCount(registry, bundle, reference);
	}

	apr_thread_mutex_unlock(registry->mutex);

	*result = true;

	return CELIX_SUCCESS;
}

void serviceRegistry_ungetServices(service_registry_pt registry, bundle_pt bundle) {
	array_list_pt fusages;
	array_list_pt usages;
	unsigned int i;

	apr_pool_t *pool = NULL;
	bundle_getMemoryPool(bundle, &pool);

	apr_thread_mutex_lock(registry->mutex);
	usages = hashMap_get(registry->inUseMap, bundle);
	apr_thread_mutex_unlock(registry->mutex);

	if (usages == NULL || arrayList_isEmpty(usages)) {
		return;
	}

	// usage arrays?
	fusages = arrayList_clone(usages);
	
	for (i = 0; i < arrayList_size(fusages); i++) {
		usage_count_pt usage = arrayList_get(fusages, i);
		service_reference_pt reference = usage->reference;
		bool ungetResult = true;
		while (ungetResult) {
			serviceRegistry_ungetService(registry, bundle, reference, &ungetResult);
		}
	}

	arrayList_destroy(fusages);
}

array_list_pt serviceRegistry_getUsingBundles(service_registry_pt registry, apr_pool_t *pool, service_reference_pt reference) {
	array_list_pt bundles = NULL;
	hash_map_iterator_pt iter;
	arrayList_create(&bundles);
	iter = hashMapIterator_create(registry->inUseMap);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		bundle_pt bundle = hashMapEntry_getKey(entry);
		array_list_pt usages = hashMapEntry_getValue(entry);
		unsigned int i;
		for (i = 0; i < arrayList_size(usages); i++) {
			usage_count_pt usage = arrayList_get(usages, i);
			if (usage->reference == reference) {
				arrayList_add(bundles, bundle);
			}
		}
	}
	hashMapIterator_destroy(iter);
	return bundles;
}

celix_status_t serviceRegistry_addHooks(service_registry_pt registry, char *serviceName, void *serviceObject, service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
		arrayList_add(registry->listenerHooks, registration);
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
		arrayList_removeElement(registry->listenerHooks, registration);
	}

	return status;
}

celix_status_t serviceRegistry_getListenerHooks(service_registry_pt registry, apr_pool_t *pool, array_list_pt *hooks) {
	celix_status_t status = CELIX_SUCCESS;

	if (registry == NULL || *hooks != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		status = arrayList_create(hooks);
		if (status == CELIX_SUCCESS) {
			unsigned int i;
			for (i = 0; i < arrayList_size(registry->listenerHooks); i++) {
				service_registration_pt registration = arrayList_get(registry->listenerHooks, i);
				service_reference_pt reference = NULL;
				serviceRegistry_createServiceReference(registry, pool, registration, &reference);
				arrayList_add(*hooks, reference);
			}
		}
	}

	framework_logIfError(status, NULL, "Cannot get listener hooks");

	return status;
}

celix_status_t serviceRegistry_servicePropertiesModified(service_registry_pt registry, service_registration_pt registration, properties_pt oldprops) {
	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED, registration, oldprops);
	}

	return CELIX_SUCCESS;
}
