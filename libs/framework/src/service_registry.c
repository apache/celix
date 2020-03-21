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
#include <assert.h>
#include <celix_api.h>

#include "service_registry_private.h"
#include "service_registration_private.h"
#include "listener_hook_service.h"
#include "celix_constants.h"
#include "service_reference_private.h"
#include "framework_private.h"

#ifdef DEBUG
#define CHECK_DELETED_REFERENCES true
#else
#define CHECK_DELETED_REFERENCES false
#endif

static celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void * serviceObject, properties_pt dictionary, enum celix_service_type svcType, service_registration_pt *registration);
static celix_status_t serviceRegistry_addHooks(service_registry_pt registry, const char* serviceName, const void *serviceObject, service_registration_pt registration);
static celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration);
static void serviceRegistry_logWarningServiceReferenceUsageCount(service_registry_pt registry, bundle_pt bundle, service_reference_pt ref, size_t usageCount, size_t refCount);
static void serviceRegistry_logWarningServiceRegistration(service_registry_pt registry, service_registration_pt reg);
static celix_status_t serviceRegistry_checkReference(service_registry_pt registry, service_reference_pt ref,
                                                     reference_status_t *refStatus);
static void serviceRegistry_logIllegalReference(service_registry_pt registry, service_reference_pt reference,
                                                   reference_status_t refStatus);
static celix_status_t serviceRegistry_setReferenceStatus(service_registry_pt registry, service_reference_pt reference,
                                                  bool deleted);
static celix_status_t serviceRegistry_getUsingBundles(service_registry_pt registry, service_registration_pt reg, array_list_pt *bundles);
static celix_status_t serviceRegistry_getServiceReference_internal(service_registry_pt registry, bundle_pt owner, service_registration_pt registration, service_reference_pt *out);
static void celix_serviceRegistry_serviceChanged(celix_service_registry_t *registry, celix_service_event_type_t eventType, service_registration_pt registration);
static void serviceRegistry_callHooksForListenerFilter(service_registry_pt registry, celix_bundle_t *owner, const celix_filter_t *filter, bool removed);

    static celix_service_registry_listener_hook_entry_t* celix_createHookEntry(long svcId, celix_listener_hook_service_t*);
static void celix_waitAndDestroyHookEntry(celix_service_registry_listener_hook_entry_t *entry);
static void celix_increaseCountHook(celix_service_registry_listener_hook_entry_t *entry);
static void celix_decreaseCountHook(celix_service_registry_listener_hook_entry_t *entry);

static void celix_increaseCountServiceListener(celix_service_registry_service_listener_entry_t *entry);
static void celix_decreaseCountServiceListener(celix_service_registry_service_listener_entry_t *entry);
static void celix_waitAndDestroyServiceListener(celix_service_registry_service_listener_entry_t *entry);

static void celix_increasePendingRegisteredEvent(celix_service_registry_t *registry, long svcId);
static void celix_decreasePendingRegisteredEvent(celix_service_registry_t *registry, long svcId);
static void celix_waitForPendingRegisteredEvents(celix_service_registry_t *registry, long svcId);

celix_status_t serviceRegistry_create(framework_pt framework, service_registry_pt *out) {
	celix_status_t status;

	service_registry_pt reg = calloc(1, sizeof(*reg));
	if (reg == NULL) {
		status = CELIX_ENOMEM;
	} else {

        reg->callback.handle = reg;
        reg->callback.getUsingBundles = (void *)serviceRegistry_getUsingBundles;
        reg->callback.unregister = (void *) serviceRegistry_unregisterService;

		reg->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
		reg->framework = framework;
		reg->currentServiceId = 1UL;
		reg->serviceReferences = hashMap_create(NULL, NULL, NULL, NULL);

        reg->checkDeletedReferences = CHECK_DELETED_REFERENCES;
        reg->deletedServiceReferences = hashMap_create(NULL, NULL, NULL, NULL);

		reg->listenerHooks = celix_arrayList_create();
		reg->serviceListeners = celix_arrayList_create();

		celixThreadMutex_create(&reg->pendingRegisterEvents.mutex, NULL);
		celixThreadCondition_init(&reg->pendingRegisterEvents.cond, NULL);
		reg->pendingRegisterEvents.map = hashMap_create(NULL, NULL, NULL, NULL);

		status = celixThreadRwlock_create(&reg->lock, NULL);
	}

	if (status == CELIX_SUCCESS) {
		*out = reg;
	} else {
		framework_logIfError(logger, status, NULL, "Cannot create service registry");
	}

	return status;
}

celix_status_t serviceRegistry_destroy(service_registry_pt registry) {
    celixThreadRwlock_writeLock(&registry->lock);

    //remove service listeners
    int size = celix_arrayList_size(registry->serviceListeners);
    if (size > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "%i dangling service listeners\n", size);
    }
    for (int i = 0; i < size; ++i) {
        celix_service_registry_service_listener_entry_t *entry = celix_arrayList_get(registry->serviceListeners, i);
        celix_decreaseCountServiceListener(entry);
        celix_waitAndDestroyServiceListener(entry);
    }
    arrayList_destroy(registry->serviceListeners);

    //destroy service registration map
    size = hashMap_size(registry->serviceRegistrations);
    if (size > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "%i bundles with dangling service registration\n", size);
    }
    hash_map_iterator_t iter = hashMapIterator_construct(registry->serviceRegistrations);
    while (hashMapIterator_hasNext(&iter)) {
        hash_map_entry_t *entry = hashMapIterator_nextEntry(&iter);
        celix_bundle_t *bnd = hashMapEntry_getKey(entry);
        celix_array_list_t *registrations = hashMapEntry_getValue(entry);
        for (int i = 0; i < celix_arrayList_size(registrations); ++i) {
            service_registration_pt reg = celix_arrayList_get(registrations, i);
            const char *svcName = NULL;
            serviceRegistration_getServiceName(reg, &svcName);
            fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Bundle %s (bundle id: %li) still has a %s service registered\n", celix_bundle_getSymbolicName(bnd), celix_bundle_getId(bnd), svcName);
        }
    }

    assert(size == 0);
    hashMap_destroy(registry->serviceRegistrations, false, false);

    //destroy service references (double) map);
    size = hashMap_size(registry->serviceReferences);
    if (size > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Unexpected service references left in the service registry! Nr of references: %i", size);
    }
    hashMap_destroy(registry->serviceReferences, false, false);

    //destroy listener hooks
    size = celix_arrayList_size(registry->listenerHooks);
    for (int i = 0; i < celix_arrayList_size(registry->listenerHooks); ++i) {
        celix_service_registry_listener_hook_entry_t *entry = celix_arrayList_get(registry->listenerHooks, i);
        celix_waitAndDestroyHookEntry(entry);
    }
    celix_arrayList_destroy(registry->listenerHooks);

    hashMap_destroy(registry->deletedServiceReferences, false, false);

    size = hashMap_size(registry->pendingRegisterEvents.map);
    assert(size == 0);
    celixThreadMutex_destroy(&registry->pendingRegisterEvents.mutex);
    celixThreadCondition_destroy(&registry->pendingRegisterEvents.cond);
    hashMap_destroy(registry->pendingRegisterEvents.map, false, false);

    free(registry);

    return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getRegisteredServices(service_registry_pt registry, bundle_pt bundle, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadRwlock_writeLock(&registry->lock);

	array_list_pt regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		unsigned int i;
		arrayList_create(services);
		
		for (i = 0; i < arrayList_size(regs); i++) {
			service_registration_pt reg = arrayList_get(regs, i);
			if (serviceRegistration_isValid(reg)) {
				service_reference_pt reference = NULL;
				status = serviceRegistry_getServiceReference_internal(registry, bundle, reg, &reference);
				if (status == CELIX_SUCCESS) {
					arrayList_add(*services, reference);
				}
			}
		}
	}

	celixThreadRwlock_unlock(&registry->lock);

	framework_logIfError(logger, status, NULL, "Cannot get registered services");

	return status;
}

celix_status_t serviceRegistry_registerService(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void* serviceObject, properties_pt dictionary, service_registration_pt *registration) {
    return serviceRegistry_registerServiceInternal(registry, bundle, serviceName, serviceObject, dictionary, CELIX_PLAIN_SERVICE, registration);
}

celix_status_t serviceRegistry_registerServiceFactory(service_registry_pt registry, bundle_pt bundle, const char* serviceName, service_factory_pt factory, properties_pt dictionary, service_registration_pt *registration) {
    return serviceRegistry_registerServiceInternal(registry, bundle, serviceName, (const void *) factory, dictionary, CELIX_DEPRECATED_FACTORY_SERVICE, registration);
}

static celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void * serviceObject, properties_pt dictionary, enum celix_service_type svcType, service_registration_pt *registration) {
	array_list_pt regs;

	if (svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
        *registration = serviceRegistration_createServiceFactory(registry->callback, bundle, serviceName,
                                                                 ++registry->currentServiceId, serviceObject,
                                                                 dictionary);
    } else if (svcType == CELIX_FACTORY_SERVICE) {
        *registration = celix_serviceRegistration_createServiceFactory(registry->callback, bundle, serviceName, ++registry->currentServiceId, (celix_service_factory_t*)serviceObject, dictionary);
	} else { //plain
	    *registration = serviceRegistration_create(registry->callback, bundle, serviceName, ++registry->currentServiceId, serviceObject, dictionary);
	}
    long svcId = serviceRegistration_getServiceId(*registration);
	//printf("Registering service %li with name %s\n", svcId, serviceName);

    serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);

	celixThreadRwlock_writeLock(&registry->lock);
	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = NULL;
		arrayList_create(&regs);
        hashMap_put(registry->serviceRegistrations, bundle, regs);
    }
	arrayList_add(regs, *registration);

    //update pending register event
    celix_increasePendingRegisteredEvent(registry, svcId);
    celixThreadRwlock_unlock(&registry->lock);


    //NOTE there is a race condition with celix_serviceRegistry_addServiceListener, as result
    //a REGISTERED event can be triggered twice instead of once. The service tracker can deal with this.
    //The handling of pending registered events is to ensure that the UNREGISTERING event is always
    //after the 1 or 2 REGISTERED events.

	celix_serviceRegistry_serviceChanged(registry, OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED, *registration);
    //update pending register event count
    celix_decreasePendingRegisteredEvent(registry, svcId);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_unregisterService(service_registry_pt registry, bundle_pt bundle, service_registration_pt registration) {
	// array_list_t clients;
	celix_array_list_t *regs;

    //fprintf(stderr, "REG: Unregistering service registration with pointer %p\n", registration);

    long svcId = serviceRegistration_getServiceId(registration);
    const char *svcName = NULL;
    serviceRegistration_getServiceName(registration, &svcName);
    //printf("Unregistering service %li with name %s\n", svcId, svcName);

	serviceRegistry_removeHook(registry, registration);

	celixThreadRwlock_writeLock(&registry->lock);
	regs = (celix_array_list_t*) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
        int size = arrayList_size(regs);
        if (size == 0) {
            celix_arrayList_destroy(regs);
            hashMap_remove(registry->serviceRegistrations, bundle);
        }
	}
	celixThreadRwlock_unlock(&registry->lock);


    //check and wait for pending register events
    celix_waitForPendingRegisteredEvents(registry, svcId);

    celix_serviceRegistry_serviceChanged(registry, OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING, registration);

    celixThreadRwlock_readLock(&registry->lock);
    //invalidate service references
    hash_map_iterator_pt iter = hashMapIterator_create(registry->serviceReferences);
    while (hashMapIterator_hasNext(iter)) {
        hash_map_pt refsMap = hashMapIterator_nextValue(iter);
        service_reference_pt ref = refsMap != NULL ?
                                   hashMap_get(refsMap, (void*)registration->serviceId) : NULL;
        if (ref != NULL) {
            serviceReference_invalidate(ref);
        }
    }
    hashMapIterator_destroy(iter);
	celixThreadRwlock_unlock(&registry->lock);

	serviceRegistration_invalidate(registration);
    serviceRegistration_release(registration);

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_clearServiceRegistrations(service_registry_pt registry, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt registrations = NULL;
    bool registrationsLeft;

    celixThreadRwlock_writeLock(&registry->lock);
    registrations = hashMap_get(registry->serviceRegistrations, bundle);
    registrationsLeft = (registrations != NULL);
    if (registrationsLeft) {
        registrationsLeft = (arrayList_size(registrations) > 0);
    }
    celixThreadRwlock_unlock(&registry->lock);

    while (registrationsLeft) {
        service_registration_pt reg = arrayList_get(registrations, 0);

        serviceRegistry_logWarningServiceRegistration(registry, reg);

        if (serviceRegistration_isValid(reg)) {
            serviceRegistration_unregister(reg);
        }
        else {
            arrayList_remove(registrations, 0);
        }

        // not removed by last unregister call?
        celixThreadRwlock_writeLock(&registry->lock);
        registrations = hashMap_get(registry->serviceRegistrations, bundle);
        registrationsLeft = (registrations != NULL);
        if (registrationsLeft) {
            registrationsLeft = (arrayList_size(registrations) > 0);
        }
        celixThreadRwlock_unlock(&registry->lock);
    }

    return status;
}

static void serviceRegistry_logWarningServiceRegistration(service_registry_pt registry __attribute__((unused)), service_registration_pt reg) {
    const char *servName = NULL;
    serviceRegistration_getServiceName(reg, &servName);
    fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Dangling service registration for service %s. Look for missing bundleContext_unregisterService/serviceRegistration_unregister calls.", servName);
}

celix_status_t serviceRegistry_getServiceReference(service_registry_pt registry, bundle_pt owner,
                                                   service_registration_pt registration, service_reference_pt *out) {
	celix_status_t status = CELIX_SUCCESS;

	if (celixThreadRwlock_writeLock(&registry->lock) == CELIX_SUCCESS) {
	    status = serviceRegistry_getServiceReference_internal(registry, owner, registration, out);
	    celixThreadRwlock_unlock(&registry->lock);
	}

	return status;
}

static celix_status_t serviceRegistry_getServiceReference_internal(service_registry_pt registry, bundle_pt owner,
                                                   service_registration_pt registration, service_reference_pt *out) {
	//only call after locked registry RWlock
	celix_status_t status = CELIX_SUCCESS;
	bundle_pt bundle = NULL;
    service_reference_pt ref = NULL;
    hash_map_pt references = NULL;

    references = hashMap_get(registry->serviceReferences, owner);
    if (references == NULL) {
        references = hashMap_create(NULL, NULL, NULL, NULL);
        hashMap_put(registry->serviceReferences, owner, references);
	}

    ref = hashMap_get(references, (void*)registration->serviceId);

    if (ref == NULL) {
        status = serviceRegistration_getBundle(registration, &bundle);
        if (status == CELIX_SUCCESS) {
            status = serviceReference_create(registry->callback, owner, registration, &ref);
        }
        if (status == CELIX_SUCCESS) {
            hashMap_put(references, (void*)registration->serviceId, ref);
            hashMap_put(registry->deletedServiceReferences, ref, (void *)false);
        }
    } else {
        serviceReference_retain(ref);
    }

    if (status == CELIX_SUCCESS) {
        *out = ref;
    }

	framework_logIfError(logger, status, NULL, "Cannot create service reference");


	return status;
}

celix_status_t serviceRegistry_getServiceReferences(service_registry_pt registry, bundle_pt owner, const char *serviceName, filter_pt filter, array_list_pt *out) {
	celix_status_t status;
	hash_map_iterator_pt iterator;
    array_list_pt references = NULL;
	array_list_pt matchingRegistrations = NULL;
    bool matchResult;

    status = arrayList_create(&references);
    status = CELIX_DO_IF(status, arrayList_create(&matchingRegistrations));

    celixThreadRwlock_readLock(&registry->lock);
	iterator = hashMapIterator_create(registry->serviceRegistrations);
	while (status == CELIX_SUCCESS && hashMapIterator_hasNext(iterator)) {
		array_list_pt regs = (array_list_pt) hashMapIterator_nextValue(iterator);
		unsigned int regIdx;
		for (regIdx = 0; (regs != NULL) && regIdx < arrayList_size(regs); regIdx++) {
			service_registration_pt registration = (service_registration_pt) arrayList_get(regs, regIdx);
			properties_pt props = NULL;

			status = serviceRegistration_getProperties(registration, &props);
			if (status == CELIX_SUCCESS) {
				bool matched = false;
				matchResult = false;
				if (filter != NULL) {
					filter_match(filter, props, &matchResult);
				}
				if ((serviceName == NULL) && ((filter == NULL) || matchResult)) {
					matched = true;
				} else if (serviceName != NULL) {
					const char *className = NULL;
					matchResult = false;
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
                        serviceRegistration_retain(registration);
                        arrayList_add(matchingRegistrations, registration);
					}
				}
			}
		}
	}
    celixThreadRwlock_unlock(&registry->lock);
	hashMapIterator_destroy(iterator);

    if (status == CELIX_SUCCESS) {
        unsigned int i;
        unsigned int size = arrayList_size(matchingRegistrations);

        for (i = 0; i < size; i += 1) {
            service_registration_pt reg = arrayList_get(matchingRegistrations, i);
            service_reference_pt reference = NULL;
            celix_status_t subStatus = serviceRegistry_getServiceReference(registry, owner, reg, &reference);
            if (subStatus == CELIX_SUCCESS) {
                arrayList_add(references, reference);
            } else {
                status = CELIX_BUNDLE_EXCEPTION;
            }
            serviceRegistration_release(reg);
        }
    }

    if(matchingRegistrations != NULL){
    	arrayList_destroy(matchingRegistrations);
    }

    if (status == CELIX_SUCCESS) {
        *out = references;
    } else {
        //TODO ungetServiceRefs
        arrayList_destroy(references);
        framework_logIfError(logger, status, NULL, "Cannot get service references");
    }

	return status;
}

celix_status_t serviceRegistry_retainServiceReference(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    reference_status_t refStatus;
    bundle_pt refBundle = NULL;
    
    celixThreadRwlock_writeLock(&registry->lock);
    serviceRegistry_checkReference(registry, reference, &refStatus);
    if (refStatus == REF_ACTIVE) {
        serviceReference_getOwner(reference, &refBundle);
        if (refBundle == bundle) {
            serviceReference_retain(reference);
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
            fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "cannot retain a service reference from an other bundle (in ref %p) (provided %p).", refBundle, bundle);
        }
    } else {
        serviceRegistry_logIllegalReference(registry, reference, refStatus);
    }
    celixThreadRwlock_unlock(&registry->lock);

    return status;
}


celix_status_t serviceRegistry_ungetServiceReference(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;
    bool destroyed = false;
    size_t count = 0;
    reference_status_t refStatus;

    celixThreadRwlock_writeLock(&registry->lock);
    serviceRegistry_checkReference(registry, reference, &refStatus);
    if (refStatus == REF_ACTIVE) {
        serviceReference_getUsageCount(reference, &count);
        serviceReference_release(reference, &destroyed);
        if (destroyed) {

            if (count > 0) {
                serviceRegistry_logWarningServiceReferenceUsageCount(registry, bundle, reference, count, 0);
            }

            hash_map_pt refsMap = hashMap_get(registry->serviceReferences, bundle);

            unsigned long refId = 0UL;
            service_reference_pt ref = NULL;

            if (refsMap != NULL) {
                hash_map_iterator_t iter = hashMapIterator_construct(refsMap);
                while (hashMapIterator_hasNext(&iter)) {
                    hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
                    refId = (unsigned long) hashMapEntry_getKey(entry); //note could be invalid e.g. freed
                    ref = hashMapEntry_getValue(entry);

                    if (ref == reference) {
                        break;
                    } else {
                        ref = NULL;
                        refId = 0UL;
                    }
                }
            }

            if (ref != NULL) {
                hashMap_remove(refsMap, (void*)refId);
                int size = hashMap_size(refsMap);
                if (size == 0) {
                    hashMap_destroy(refsMap, false, false);
                    hashMap_remove(registry->serviceReferences, bundle);
                }
                serviceRegistry_setReferenceStatus(registry, reference, true);
            } else {
                fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Cannot find reference %p in serviceReferences map",
                       reference);
            }
        }
    } else {
        serviceRegistry_logIllegalReference(registry, reference, refStatus);
    }
    celixThreadRwlock_unlock(&registry->lock);

    return status;
}

static celix_status_t serviceRegistry_setReferenceStatus(service_registry_pt registry, service_reference_pt reference,
                                                  bool deleted) {
    //precondition write locked on registry->lock
    if (registry->checkDeletedReferences) {
        hashMap_put(registry->deletedServiceReferences, reference, (void *) deleted);
    }
    return CELIX_SUCCESS;
}

static void serviceRegistry_logIllegalReference(service_registry_pt registry __attribute__((unused)), service_reference_pt reference,
                                                   reference_status_t refStatus) {
    fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR,
           "Error handling service reference %p, status is %i",reference, refStatus);
}

static celix_status_t serviceRegistry_checkReference(service_registry_pt registry, service_reference_pt ref,
                                              reference_status_t *out) {
    //precondition read or write locked on registry->lock
    celix_status_t status = CELIX_SUCCESS;

    if (registry->checkDeletedReferences) {
        reference_status_t refStatus = REF_UNKNOWN;

        if (hashMap_containsKey(registry->deletedServiceReferences, ref)) {
            bool deleted = (bool) hashMap_get(registry->deletedServiceReferences, ref);
            refStatus = deleted ? REF_DELETED : REF_ACTIVE;
        }

        *out = refStatus;
    } else {
        *out = REF_ACTIVE;
    }

    return status;
}

static void serviceRegistry_logWarningServiceReferenceUsageCount(service_registry_pt registry __attribute__((unused)), bundle_pt bundle, service_reference_pt ref, size_t usageCount, size_t refCount) {
    if (usageCount > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Service Reference destroyed with usage count is %zu, expected 0. Look for missing bundleContext_ungetService calls.", usageCount);
    }
    if (refCount > 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Dangling service reference. Reference count is %zu, expected 1.  Look for missing bundleContext_ungetServiceReference calls.", refCount);
    }

    if(usageCount > 0 || refCount > 0) {
        const char* bundle_name = celix_bundle_getSymbolicName(bundle);
        const char* service_name = "unknown";
        const char* bundle_provider_name = "unknown";
        if (refCount > 0 && ref != NULL) {
            serviceReference_getProperty(ref, OSGI_FRAMEWORK_OBJECTCLASS, &service_name);
            service_registration_pt reg = NULL;
            bundle_pt providedBnd = NULL;
            serviceReference_getServiceRegistration(ref, &reg);
            serviceRegistration_getBundle(reg, &providedBnd);
            bundle_provider_name = celix_bundle_getSymbolicName(providedBnd);
        }

        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Previous Dangling service reference warnings caused by bundle '%s', for service '%s', provided by bundle '%s'", bundle_name, service_name, bundle_provider_name);
    }
}


celix_status_t serviceRegistry_clearReferencesFor(service_registry_pt registry, bundle_pt bundle) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadRwlock_writeLock(&registry->lock);

    hash_map_pt refsMap = hashMap_remove(registry->serviceReferences, bundle);
    if (refsMap != NULL) {
        hash_map_iterator_pt iter = hashMapIterator_create(refsMap);
        while (hashMapIterator_hasNext(iter)) {
            service_reference_pt ref = hashMapIterator_nextValue(iter);
            size_t refCount;
            size_t usageCount;

            serviceReference_getUsageCount(ref, &usageCount);
            serviceReference_getReferenceCount(ref, &refCount);
            serviceRegistry_logWarningServiceReferenceUsageCount(registry, bundle, ref, usageCount, refCount);

            while (usageCount > 0) {
                serviceReference_decreaseUsage(ref, &usageCount);
            }

            bool destroyed = false;
            while (!destroyed) {
                serviceReference_release(ref, &destroyed);
            }
            serviceRegistry_setReferenceStatus(registry, ref, true);

        }
        hashMapIterator_destroy(iter);
        hashMap_destroy(refsMap, false, false);
    }

    celixThreadRwlock_unlock(&registry->lock);

    return status;
}


celix_status_t serviceRegistry_getServicesInUse(service_registry_pt registry, bundle_pt bundle, array_list_pt *out) {

    array_list_pt result = NULL;
    arrayList_create(&result);

    //LOCK
    celixThreadRwlock_readLock(&registry->lock);

    hash_map_pt refsMap = hashMap_get(registry->serviceReferences, bundle);

    if(refsMap) {
        hash_map_iterator_pt iter = hashMapIterator_create(refsMap);
        while (hashMapIterator_hasNext(iter)) {
            service_reference_pt ref = hashMapIterator_nextValue(iter);
            arrayList_add(result, ref);
        }
        hashMapIterator_destroy(iter);
    }

    //UNLOCK
    celixThreadRwlock_unlock(&registry->lock);

    *out = result;

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_getService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, const void **out) {
	celix_status_t status = CELIX_SUCCESS;
	service_registration_pt registration = NULL;
    size_t count = 0;
    const void *service = NULL;
    bool valid = false;
    reference_status_t refStatus;


    celixThreadRwlock_readLock(&registry->lock);
    serviceRegistry_checkReference(registry, reference, &refStatus);
    if (refStatus == REF_ACTIVE) {
        serviceReference_getServiceRegistration(reference, &registration);
        valid = serviceRegistration_isValid(registration);
        if (valid) {
            serviceRegistration_retain(registration);
            serviceReference_increaseUsage(reference, &count);
        } else {
            *out = NULL; //invalid service registration
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    if (valid && refStatus == REF_ACTIVE) {
        if (count == 1) {
            serviceRegistration_getService(registration, bundle, &service);
            serviceReference_setService(reference, service);
        }
        serviceRegistration_release(registration);

        /* NOTE the out argument of sr_getService should be 'const void**'
           To ensure backwards compatibility a cast is made instead.
        */
        serviceReference_getService(reference, (void **)out);
    } else {
        if (refStatus != REF_ACTIVE) {
            serviceRegistry_logIllegalReference(registry, reference, refStatus);
        }
        status = CELIX_BUNDLE_EXCEPTION;
    }

	return status;
}

celix_status_t serviceRegistry_ungetService(service_registry_pt registry, bundle_pt bundle, service_reference_pt reference, bool *result) {
	celix_status_t status = CELIX_SUCCESS;
    service_registration_pt reg = NULL;
    const void *service = NULL;
    size_t count = 0;
    celix_status_t subStatus = CELIX_SUCCESS;
    reference_status_t refStatus;

    celixThreadRwlock_readLock(&registry->lock);
    serviceRegistry_checkReference(registry, reference, &refStatus);
    celixThreadRwlock_unlock(&registry->lock);

    if (refStatus == REF_ACTIVE) {
        subStatus = serviceReference_decreaseUsage(reference, &count);
        if (count == 0) {
            /*NOTE the argument service of sr_getService should be 'const void**'
              To ensure backwards compatibility a cast is made instead.
              */
            serviceReference_getService(reference, (void**)&service);
            serviceReference_getServiceRegistration(reference, &reg);
            if (reg != NULL) {
                serviceRegistration_ungetService(reg, bundle, &service);
            }
        }
    } else {
        serviceRegistry_logIllegalReference(registry, reference, refStatus);
        status = CELIX_BUNDLE_EXCEPTION;
    }

    if (result) {
        *result = (subStatus == CELIX_SUCCESS);
    }


	return status;
}

static celix_status_t serviceRegistry_addHooks(service_registry_pt registry, const char* serviceName, const void* serviceObject __attribute__((unused)), service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
        celixThreadRwlock_writeLock(&registry->lock);
        long svcId = serviceRegistration_getServiceId(registration);
        celix_service_registry_listener_hook_entry_t *entry = celix_createHookEntry(svcId, (celix_listener_hook_service_t*)serviceObject);
		arrayList_add(registry->listenerHooks, entry);
        celixThreadRwlock_unlock(&registry->lock);
	}

	return status;
}

static celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;
	const char* serviceName = NULL;

	celix_service_registry_listener_hook_entry_t *removedEntry = NULL;

	properties_pt props = NULL;
	serviceRegistration_getProperties(registration, &props);
	serviceName = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
	long svcId = serviceRegistration_getServiceId(registration);
	if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
        celixThreadRwlock_writeLock(&registry->lock);
        for (int i = 0; i < celix_arrayList_size(registry->listenerHooks); ++i) {
            celix_service_registry_listener_hook_entry_t *visit = celix_arrayList_get(registry->listenerHooks, i);
            if (visit->svcId == svcId) {
                removedEntry = visit;
                celix_arrayList_removeAt(registry->listenerHooks, i);
                break;
            }
        }
        celixThreadRwlock_unlock(&registry->lock);
	}

	if (removedEntry != NULL) {
        celix_waitAndDestroyHookEntry(removedEntry);
	}

	return status;
}

static void serviceRegistry_callHooksForListenerFilter(service_registry_pt registry, celix_bundle_t *owner, const celix_filter_t *filter, bool removed) {
    celix_bundle_context_t *ctx;
    bundle_getContext(owner, &ctx);

    struct listener_hook_info info;
    info.context = ctx;
    info.removed = removed;
    info.filter = celix_filter_getFilterString(filter);
    celix_array_list_t *infos = celix_arrayList_create();
    celix_arrayList_add(infos, &info);

    celix_array_list_t *hookRegistrations = celix_arrayList_create();

    celixThreadRwlock_readLock(&registry->lock);
    unsigned size = arrayList_size(registry->listenerHooks);
    for (int i = 0; i < size; ++i) {
        celix_service_registry_listener_hook_entry_t* entry = arrayList_get(registry->listenerHooks, i);
        if (entry != NULL) {
            celix_increaseCountHook(entry); //increate use count to ensure the hook cannot be removed, untill used
            celix_arrayList_add(hookRegistrations, entry);
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    for (int i = 0; i < arrayList_size(hookRegistrations); ++i) {
        celix_service_registry_listener_hook_entry_t* entry = celix_arrayList_get(hookRegistrations, i);
        if (removed) {
            entry->hook->removed(entry->hook->handle, infos);
        } else {
            entry->hook->added(entry->hook->handle, infos);
        }
        celix_decreaseCountHook(entry); //done using hook. decrease count
    }
    celix_arrayList_destroy(hookRegistrations);
    celix_arrayList_destroy(infos);
}

size_t serviceRegistry_nrOfHooks(service_registry_pt registry) {
    celixThreadRwlock_readLock(&registry->lock);
    unsigned size = arrayList_size(registry->listenerHooks);
    celixThreadRwlock_unlock(&registry->lock);
    return (size_t) size;
}

static celix_status_t serviceRegistry_getUsingBundles(service_registry_pt registry, service_registration_pt registration, array_list_pt *out) {
    celix_status_t status;
    array_list_pt bundles = NULL;
    hash_map_iterator_pt iter;

    status = arrayList_create(&bundles);
    if (status == CELIX_SUCCESS) {
        celixThreadRwlock_readLock(&registry->lock);
        iter = hashMapIterator_create(registry->serviceReferences);
        while (hashMapIterator_hasNext(iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
            bundle_pt registrationUser = hashMapEntry_getKey(entry);
            hash_map_pt regMap = hashMapEntry_getValue(entry);
            if (hashMap_containsKey(regMap, (void*)registration->serviceId)) {
                arrayList_add(bundles, registrationUser);
            }
        }
        hashMapIterator_destroy(iter);
        celixThreadRwlock_unlock(&registry->lock);
    }

    if (status == CELIX_SUCCESS) {
        *out = bundles;
    } else {
        if (bundles != NULL) {
            arrayList_destroy(bundles);
        }
    }

    return status;
}

celix_status_t
celix_serviceRegistry_registerServiceFactory(
        celix_service_registry_t *reg,
        const celix_bundle_t *bnd,
        const char *serviceName,
        celix_service_factory_t *factory,
        celix_properties_t* props,
        service_registration_t **registration) {
    return serviceRegistry_registerServiceInternal(reg, (celix_bundle_t*)bnd, serviceName, (const void *) factory, props, CELIX_FACTORY_SERVICE, registration);
}

static celix_service_registry_listener_hook_entry_t* celix_createHookEntry(long svcId, celix_listener_hook_service_t *hook) {
    celix_service_registry_listener_hook_entry_t* entry = calloc(1, sizeof(*entry));
    entry->svcId = svcId;
    entry->hook = hook;
    celixThreadMutex_create(&entry->mutex, NULL);
    celixThreadCondition_init(&entry->cond, NULL);
    return entry;
}

static void celix_waitAndDestroyHookEntry(celix_service_registry_listener_hook_entry_t *entry) {
    if (entry != NULL) {
        celixThreadMutex_lock(&entry->mutex);
        int waitCount = 0;
        while (entry->useCount > 0) {
            celixThreadCondition_timedwaitRelative(&entry->cond, &entry->mutex, 1, 0); //wait for 1 second
            waitCount += 1;
            if (waitCount >= 5) {
                fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING,
                        "Still waiting for service listener hook use count to become zero. Waiting for %i seconds. Use Count is %i, svc id is %li", waitCount, (int)entry->useCount, entry->svcId);
            }
        }
        celixThreadMutex_unlock(&entry->mutex);

        //done waiting, use count is on 0
        celixThreadCondition_destroy(&entry->cond);
        celixThreadMutex_destroy(&entry->mutex);
        free(entry);
    }
}
static void celix_increaseCountHook(celix_service_registry_listener_hook_entry_t *entry) {
    if (entry != NULL) {
        celixThreadMutex_lock(&entry->mutex);
        entry->useCount += 1;
        celixThreadCondition_broadcast(&entry->cond);
        celixThreadMutex_unlock(&entry->mutex);
    }
}
static void celix_decreaseCountHook(celix_service_registry_listener_hook_entry_t *entry) {
    if (entry != NULL) {
        celixThreadMutex_lock(&entry->mutex);
        entry->useCount -= 1;
        celixThreadCondition_broadcast(&entry->cond);
        celixThreadMutex_unlock(&entry->mutex);
    }
}

static void celix_increaseCountServiceListener(celix_service_registry_service_listener_entry_t *entry) {
    if (entry != NULL) {
        celixThreadMutex_lock(&entry->mutex);
        entry->useCount += 1;
        celixThreadCondition_broadcast(&entry->cond);
        celixThreadMutex_unlock(&entry->mutex);
    }
}

static void celix_decreaseCountServiceListener(celix_service_registry_service_listener_entry_t *entry) {
    if (entry != NULL) {
        celixThreadMutex_lock(&entry->mutex);
        entry->useCount -= 1;
        celixThreadCondition_broadcast(&entry->cond);
        celixThreadMutex_unlock(&entry->mutex);
    }
}

static inline void celix_waitAndDestroyServiceListener(celix_service_registry_service_listener_entry_t *entry) {
    celixThreadMutex_lock(&entry->mutex);
    while (entry->useCount != 0) {
        celixThreadCondition_wait(&entry->cond, &entry->mutex);
    }
    celixThreadMutex_unlock(&entry->mutex);

    //use count == 0 -> safe to destroy.
    //destroy
    celixThreadMutex_destroy(&entry->mutex);
    celixThreadCondition_destroy(&entry->cond);
    celix_filter_destroy(entry->filter);
    free(entry);
}


celix_array_list_t* celix_serviceRegistry_listServiceIdsForOwner(celix_service_registry_t* registry, long bndId) {
    celix_array_list_t *result = celix_arrayList_create();
    celixThreadRwlock_readLock(&registry->lock);
    celix_bundle_t *bundle = framework_getBundleById(registry->framework, bndId);
    celix_array_list_t *registrations = bundle != NULL ? hashMap_get(registry->serviceRegistrations, bundle) : NULL;
    if (registrations != NULL) {
        for (int i = 0; i < celix_arrayList_size(registrations); ++i) {
            service_registration_t *reg = celix_arrayList_get(registrations, i);
            long svcId = serviceRegistration_getServiceId(reg);
            celix_arrayList_addLong(result, svcId);
        }
    }
    celixThreadRwlock_unlock(&registry->lock);
    return result;
}

bool celix_serviceRegistry_getServiceInfo(
        celix_service_registry_t* registry,
        long svcId,
        long bndId,
        char **outServiceName,
        celix_properties_t **outServiceProperties,
        bool *outIsFactory) {
    bool found = false;

    celixThreadRwlock_readLock(&registry->lock);
    celix_bundle_t *bundle = framework_getBundleById(registry->framework, bndId);
    celix_array_list_t *registrations = bundle != NULL ? hashMap_get(registry->serviceRegistrations, bundle) : NULL;
    if (registrations != NULL) {
        for (int i = 0; i < celix_arrayList_size(registrations); ++i) {
            service_registration_t *reg = celix_arrayList_get(registrations, i);
            if (svcId == serviceRegistration_getServiceId(reg)) {
                found = true;
                if (outServiceName != NULL) {
                    const char *s = NULL;
                    serviceRegistration_getServiceName(reg, &s);
                    *outServiceName = celix_utils_strdup(s);
                }
                if (outServiceProperties != NULL) {
                    celix_properties_t *p = NULL;
                    serviceRegistration_getProperties(reg, &p);
                    *outServiceProperties = celix_properties_copy(p);
                }
                if (outIsFactory != NULL) {
                    *outIsFactory = serviceRegistration_isFactoryService(reg);
                }
                break;
            }
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    return found;
}

celix_status_t celix_serviceRegistry_addServiceListener(celix_service_registry_t *registry, celix_bundle_t *bundle, const char *stringFilter, celix_service_listener_t *listener) {

    celix_filter_t *filter = NULL;
    if (stringFilter != NULL) {
        filter = celix_filter_create(stringFilter);
        if (filter == NULL) {
            fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Cannot add service listener filter '%s' is invalid", stringFilter);
            return CELIX_ILLEGAL_ARGUMENT;
        }
    }

    celix_service_registry_service_listener_entry_t *entry = calloc(1, sizeof(*entry));
    entry->bundle = bundle;
    entry->filter = filter;
    entry->listener = listener;
    entry->useCount = 1; //new entry -> count on 1
    celixThreadMutex_create(&entry->mutex, NULL);
    celixThreadCondition_init(&entry->cond, NULL);

    celix_array_list_t *registrations =  celix_arrayList_create();

    celixThreadRwlock_writeLock(&registry->lock);
    celix_arrayList_add(registry->serviceListeners, entry); //use count 1

    //find already registered services
    hash_map_iterator_t iter = hashMapIterator_construct(registry->serviceRegistrations);
    while (hashMapIterator_hasNext(&iter)) {
        celix_array_list_t *regs = (array_list_pt) hashMapIterator_nextValue(&iter);
        for (int regIdx = 0; (regs != NULL) && regIdx < celix_arrayList_size(regs); ++regIdx) {
            service_registration_pt registration = celix_arrayList_get(regs, regIdx);
            properties_pt props = NULL;
            serviceRegistration_getProperties(registration, &props);
            if (celix_filter_match(filter, props)) {
                serviceRegistration_retain(registration);
                long svcId = serviceRegistration_getServiceId(registration);
                celix_arrayList_add(registrations, registration);
                //update pending register event count
                celix_increasePendingRegisteredEvent(registry, svcId);
            }
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    //NOTE there is a race condition with serviceRegistry_registerServiceInternal, as result
    //a REGISTERED event can be triggered twice instead of once. The service tracker can deal with this.
    //The handling of pending registered events is to ensure that the UNREGISTERING event is always
    //after the 1 or 2 REGISTERED events.

    for (int i = 0; i < celix_arrayList_size(registrations); ++i) {
        service_registration_pt reg = celix_arrayList_get(registrations, i);
        long svcId = serviceRegistration_getServiceId(reg);
        service_reference_pt ref = NULL;
        serviceRegistry_getServiceReference_internal(registry, bundle, reg, &ref);
        celix_service_event_t event;
        event.reference = ref;
        event.type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;
        listener->serviceChanged(listener->handle, &event);
        serviceReference_release(ref, NULL);
        serviceRegistration_release(reg);

        //update pending register event count
        celix_decreasePendingRegisteredEvent(registry, svcId);
    }
    celix_arrayList_destroy(registrations);

    serviceRegistry_callHooksForListenerFilter(registry, bundle, entry->filter, false);

    celix_decreaseCountServiceListener(entry); //use count decreased, can be 0
    return CELIX_SUCCESS;
}

celix_status_t celix_serviceRegistry_removeServiceListener(celix_service_registry_t *registry, celix_service_listener_t *listener) {
    celix_service_registry_service_listener_entry_t *entry = NULL;

    celixThreadRwlock_writeLock(&registry->lock);
    for (int i = 0; i < celix_arrayList_size(registry->serviceListeners); ++i) {
        celix_service_registry_service_listener_entry_t *visit = celix_arrayList_get(registry->serviceListeners, i);
        if (visit->listener == listener) {
            entry = visit;
            celix_arrayList_removeAt(registry->serviceListeners, i);
            break;
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    if (entry != NULL) {
        serviceRegistry_callHooksForListenerFilter(registry, entry->bundle, entry->filter, true);
        celix_waitAndDestroyServiceListener(entry);
    } else {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Cannot remove service listener, listener not found");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return CELIX_SUCCESS;
}

static void celix_serviceRegistry_serviceChanged(celix_service_registry_t *registry, celix_service_event_type_t eventType, service_registration_pt registration) {
    celix_service_registry_service_listener_entry_t *entry;

    celix_array_list_t* retainedEntries = celix_arrayList_create();
    celix_array_list_t* matchedEntries = celix_arrayList_create();

    celixThreadRwlock_readLock(&registry->lock);
    for (int i = 0; i < celix_arrayList_size(registry->serviceListeners); ++i) {
        entry = celix_arrayList_get(registry->serviceListeners, i);
        celix_arrayList_add(retainedEntries, entry);
        celix_increaseCountServiceListener(entry); //ensure that use count > 0, so that the listener cannot be destroyed until all pending event are handled.
    }
    celixThreadRwlock_unlock(&registry->lock);

    for (int i = 0; i < celix_arrayList_size(retainedEntries); ++i) {
        entry = celix_arrayList_get(retainedEntries, i);
        int matched = 0;
        celix_properties_t *props = NULL;
        bool matchResult = false;
        serviceRegistration_getProperties(registration, &props);
        if (entry->filter != NULL) {
            filter_match(entry->filter, props, &matchResult);
        }
        matched = (entry->filter == NULL) || matchResult;
        if (matched) {
            celix_arrayList_add(matchedEntries, entry);
        } else {
            celix_decreaseCountServiceListener(entry); //Not a match -> release entry
        }
    }
    celix_arrayList_destroy(retainedEntries);

    /*
     * TODO FIXME, A deadlock can happen when (e.g.) a service is deregistered, triggering this fw_serviceChanged and
     * one of the matching service listener callbacks tries to remove an other matched service listener.
     * The remove service listener will call the listener_waitForDestroy and the fw_serviceChanged part keeps the
     * usageCount on > 0.
     *
     * Not sure how to prevent/handle this.
     */

    for (int i = 0; i < celix_arrayList_size(matchedEntries); ++i) {
        entry = celix_arrayList_get(matchedEntries, i);
        service_reference_pt reference = NULL;
        celix_service_event_t event;
        serviceRegistry_getServiceReference(registry, entry->bundle, registration, &reference);
        event.type = eventType;
        event.reference = reference;
        entry->listener->serviceChanged(entry->listener->handle, &event);
        serviceRegistry_ungetServiceReference(registry, entry->bundle, reference);
        celix_decreaseCountServiceListener(entry); //decrease usage, so that the listener can be destroyed (if use count is now 0)
    }
    celix_arrayList_destroy(matchedEntries);
}


static void celix_increasePendingRegisteredEvent(celix_service_registry_t *registry, long svcId) {
    celixThreadMutex_lock(&registry->pendingRegisterEvents.mutex);
    long count = (long)hashMap_get(registry->pendingRegisterEvents.map, (void*)svcId);
    count += 1;
    hashMap_put(registry->pendingRegisterEvents.map, (void*)svcId, (void*)count);
    celixThreadMutex_unlock(&registry->pendingRegisterEvents.mutex);
}

static void celix_decreasePendingRegisteredEvent(celix_service_registry_t *registry, long svcId) {
    celixThreadMutex_lock(&registry->pendingRegisterEvents.mutex);
    long count = (long)hashMap_get(registry->pendingRegisterEvents.map, (void*)svcId);
    assert(count >= 1);
    count -= 1;
    if (count > 0) {
        hashMap_put(registry->pendingRegisterEvents.map, (void *)svcId, (void *)count);
    } else {
        hashMap_remove(registry->pendingRegisterEvents.map, (void*)svcId);
    }
    celixThreadCondition_signal(&registry->pendingRegisterEvents.cond);
    celixThreadMutex_unlock(&registry->pendingRegisterEvents.mutex);
}

static void celix_waitForPendingRegisteredEvents(celix_service_registry_t *registry, long svcId) {
    celixThreadMutex_lock(&registry->pendingRegisterEvents.mutex);
    long count = (long)hashMap_get(registry->pendingRegisterEvents.map, (void*)svcId);
    while (count > 0) {
        celixThreadCondition_wait(&registry->pendingRegisterEvents.cond, &registry->pendingRegisterEvents.mutex);
        count = (long)hashMap_get(registry->pendingRegisterEvents.map, (void*)svcId);
    }
    celixThreadMutex_unlock(&registry->pendingRegisterEvents.mutex);
}