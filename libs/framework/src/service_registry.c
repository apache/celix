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

#include "celix_bundle_private.h"
#include "celix_constants.h"
#include "celix_stdlib_cleanup.h"
#include "celix_version_range.h"
#include "framework_private.h"
#include "listener_hook_service.h"
#include "service_reference_private.h"
#include "service_registration_private.h"
#include "service_registry_private.h"

static celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void * serviceObject, celix_properties_t* dictionary, long reservedId, enum celix_service_type svcType, service_registration_pt *registration);
static celix_status_t serviceRegistry_unregisterService(service_registry_pt registry, bundle_pt bundle, service_registration_pt registration);
static bool serviceRegistry_tryRemoveServiceReference(service_registry_pt registry, service_reference_pt ref);
static celix_status_t serviceRegistry_addHooks(service_registry_pt registry, const char* serviceName, const void *serviceObject, service_registration_pt registration);
static celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration);
static void serviceRegistry_logWarningServiceReferenceUsageCount(service_registry_pt registry, bundle_pt bundle, service_reference_pt ref, size_t usageCount, size_t refCount);
static celix_status_t serviceRegistry_getUsingBundles(service_registry_pt registry, service_registration_pt reg, celix_array_list_t** bundles);
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

celix_service_registry_t* celix_serviceRegistry_create(framework_pt framework) {
    celix_service_registry_t* reg = calloc(1, sizeof(*reg));

    reg->callback.handle = reg;
    reg->callback.getUsingBundles = (void *)serviceRegistry_getUsingBundles;
    reg->callback.unregister = (void *) serviceRegistry_unregisterService;
    reg->callback.tryRemoveServiceReference = (void *) serviceRegistry_tryRemoveServiceReference;

    reg->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
    reg->framework = framework;
    reg->nextServiceId = 1L;
    reg->serviceReferences = hashMap_create(NULL, NULL, NULL, NULL);

    reg->listenerHooks = celix_arrayList_create();
    reg->serviceListeners = celix_arrayList_create();

    celixThreadMutex_create(&reg->pendingRegisterEvents.mutex, NULL);
    celixThreadCondition_init(&reg->pendingRegisterEvents.cond, NULL);
    celixThreadRwlock_create(&reg->lock, NULL);
    reg->pendingRegisterEvents.map = hashMap_create(NULL, NULL, NULL, NULL);


	return reg;
}

void celix_serviceRegistry_destroy(celix_service_registry_t* registry) {
    celixThreadRwlock_destroy(&registry->lock);

    //remove service listeners
    int size = celix_arrayList_size(registry->serviceListeners);
    if (size > 0) {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "%i dangling service listeners\n", size);
    }
    for (int i = 0; i < size; ++i) {
        celix_service_registry_service_listener_entry_t *entry = celix_arrayList_get(registry->serviceListeners, i);
        celix_decreaseCountServiceListener(entry);
        celix_waitAndDestroyServiceListener(entry);
    }
    celix_arrayList_destroy(registry->serviceListeners);

    //destroy service registration map
    size = hashMap_size(registry->serviceRegistrations);
    if (size > 0) {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "%i bundles with dangling service registration\n", size);
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
            fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "Bundle %s (bundle id: %li) still has a %s service registered\n", celix_bundle_getSymbolicName(bnd), celix_bundle_getId(bnd), svcName);
        }
    }

    assert(size == 0);
    hashMap_destroy(registry->serviceRegistrations, false, false);

    //destroy service references (double) map);
    size = hashMap_size(registry->serviceReferences);
    if (size > 0) {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "Unexpected service references left in the service registry! Nr of references: %i", size);
    }
    hashMap_destroy(registry->serviceReferences, false, false);

    //destroy listener hooks
    size = celix_arrayList_size(registry->listenerHooks);
    for (int i = 0; i < celix_arrayList_size(registry->listenerHooks); ++i) {
        celix_service_registry_listener_hook_entry_t *entry = celix_arrayList_get(registry->listenerHooks, i);
        celix_waitAndDestroyHookEntry(entry);
    }
    celix_arrayList_destroy(registry->listenerHooks);

    size = hashMap_size(registry->pendingRegisterEvents.map);
    assert(size == 0);
    celixThreadMutex_destroy(&registry->pendingRegisterEvents.mutex);
    celixThreadCondition_destroy(&registry->pendingRegisterEvents.cond);
    hashMap_destroy(registry->pendingRegisterEvents.map, false, false);

    free(registry);
}

celix_status_t serviceRegistry_getRegisteredServices(service_registry_pt registry, bundle_pt bundle, celix_array_list_t** services) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadRwlock_writeLock(&registry->lock);

    celix_array_list_t* regs = (celix_array_list_t*) hashMap_get(registry->serviceRegistrations, bundle);
    if (regs != NULL) {
        unsigned int i;
        *services = celix_arrayList_create();

        for (i = 0; i < celix_arrayList_size(regs); i++) {
            service_registration_pt reg = celix_arrayList_get(regs, i);
            service_reference_pt reference = NULL;
            //assert(serviceRegistration_isValid(reg));
            status = serviceRegistry_getServiceReference_internal(registry, bundle, reg, &reference);
            if (status == CELIX_SUCCESS) {
                celix_arrayList_add(*services, reference);
            }
        }
    }

    celixThreadRwlock_unlock(&registry->lock);

    framework_logIfError(registry->framework->logger, status, NULL, "Cannot get registered services");

    return status;
}

celix_status_t serviceRegistry_registerService(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void* serviceObject, celix_properties_t* dictionary, service_registration_pt *registration) {
    return serviceRegistry_registerServiceInternal(registry, bundle, serviceName, serviceObject, dictionary, 0 /*TODO*/, CELIX_PLAIN_SERVICE, registration);
}

celix_status_t serviceRegistry_registerServiceFactory(service_registry_pt registry, bundle_pt bundle, const char* serviceName, service_factory_pt factory, celix_properties_t* dictionary, service_registration_pt *registration) {
    return serviceRegistry_registerServiceInternal(registry, bundle, serviceName, (const void *) factory, dictionary, 0 /*TODO*/, CELIX_DEPRECATED_FACTORY_SERVICE, registration);
}

static celix_status_t serviceRegistry_registerServiceInternal(service_registry_pt registry, bundle_pt bundle, const char* serviceName, const void * serviceObject, celix_properties_t* dictionary, long reservedId, enum celix_service_type svcType, service_registration_pt *registration) {
    celix_array_list_t* regs;
    long svcId = reservedId > 0 ? reservedId : celix_serviceRegistry_nextSvcId(registry);

    celix_properties_setLong(dictionary, CELIX_FRAMEWORK_SERVICE_BUNDLE_ID, celix_bundle_getId(bundle));

    if (svcType == CELIX_DEPRECATED_FACTORY_SERVICE) {
        celix_properties_set(dictionary, CELIX_FRAMEWORK_SERVICE_SCOPE, CELIX_FRAMEWORK_SERVICE_SCOPE_BUNDLE);
        *registration = serviceRegistration_createServiceFactory(registry->callback, bundle, serviceName,
                                                                 svcId, serviceObject,
                                                                 dictionary);
    } else if (svcType == CELIX_FACTORY_SERVICE) {
        celix_properties_set(dictionary, CELIX_FRAMEWORK_SERVICE_SCOPE, CELIX_FRAMEWORK_SERVICE_SCOPE_BUNDLE);
        *registration = celix_serviceRegistration_createServiceFactory(registry->callback, bundle, serviceName, svcId, (celix_service_factory_t*)serviceObject, dictionary);
    } else { //plain
        celix_properties_set(dictionary, CELIX_FRAMEWORK_SERVICE_SCOPE, CELIX_FRAMEWORK_SERVICE_SCOPE_SINGLETON);
        *registration = serviceRegistration_create(registry->callback, bundle, serviceName, svcId, serviceObject, dictionary);
    }
    //printf("Registering service %li with name %s\n", svcId, serviceName);
    if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
        serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);
    }

	celixThreadRwlock_writeLock(&registry->lock);
	regs = (celix_array_list_t*) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = celix_arrayList_create();
        hashMap_put(registry->serviceRegistrations, bundle, regs);
    }
    celix_arrayList_add(regs, *registration);

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

static celix_status_t serviceRegistry_unregisterService(service_registry_pt registry,
                                                        bundle_pt bundle,
                                                        service_registration_pt registration) {
    // array_list_t clients;
    celix_array_list_t* regs;

    // fprintf(stderr, "REG: Unregistering service registration with pointer %p\n", registration);

    long svcId = serviceRegistration_getServiceId(registration);
    const char* svcName = NULL;
    serviceRegistration_getServiceName(registration, &svcName);
    // printf("Unregistering service %li with name %s\n", svcId, svcName);

    if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, svcName) == 0) {
        serviceRegistry_removeHook(registry, registration);
    }

    celixThreadRwlock_writeLock(&registry->lock);
    regs = (celix_array_list_t*)hashMap_get(registry->serviceRegistrations, bundle);
    if (regs != NULL) {
        celix_arrayList_remove(regs, registration);
        int size = celix_arrayList_size(regs);
        if (size == 0) {
            celix_arrayList_destroy(regs);
            hashMap_remove(registry->serviceRegistrations, bundle);
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    // check and wait for pending register events
    celix_waitForPendingRegisteredEvents(registry, svcId);

    celix_serviceRegistry_serviceChanged(registry, OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING, registration);

    celixThreadRwlock_readLock(&registry->lock);
    // invalidate service references
    hash_map_iterator_pt iter = hashMapIterator_create(registry->serviceReferences);
    while (hashMapIterator_hasNext(iter)) {
        hash_map_pt refsMap = hashMapIterator_nextValue(iter);
        service_reference_pt ref = refsMap != NULL ? hashMap_get(refsMap, (void*)registration->serviceId) : NULL;
        if (ref != NULL) {
            serviceReference_invalidateCache(ref);
        }
    }
    hashMapIterator_destroy(iter);
    serviceRegistration_invalidate(registration);
    celixThreadRwlock_unlock(&registry->lock);
    serviceRegistration_release(registration);

    return CELIX_SUCCESS;
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
    service_reference_pt ref = NULL;
    hash_map_pt references = NULL;

    references = hashMap_get(registry->serviceReferences, owner);
    if (references == NULL) {
        references = hashMap_create(NULL, NULL, NULL, NULL);
        hashMap_put(registry->serviceReferences, owner, references);
	}

    ref = hashMap_get(references, (void*)registration->serviceId);

    if (ref == NULL) {
        status = serviceReference_create(registry->callback, owner, registration, &ref);
        if (status == CELIX_SUCCESS) {
            hashMap_put(references, (void*)registration->serviceId, ref);
        }
    } else {
        serviceReference_retain(ref);
    }

    if (status == CELIX_SUCCESS) {
        *out = ref;
    }

	framework_logIfError(registry->framework->logger, status, NULL, "Cannot create service reference");


	return status;
}

celix_status_t serviceRegistry_getServiceReferences(service_registry_pt registry,
                                                    bundle_pt owner,
                                                    const char* serviceName,
                                                    filter_pt filter,
                                                    celix_array_list_t** out) {
    bool matchResult;
    celix_autoptr(celix_array_list_t) references = celix_arrayList_create();
    celix_autoptr(celix_array_list_t) matchingRegistrations = celix_arrayList_create();

    if (!references || !matchingRegistrations) {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot create service references, out of memory");
        return CELIX_ENOMEM;
    }

    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&registry->lock);
    hash_map_iterator_t iterator = hashMapIterator_construct(registry->serviceRegistrations);
    while (status == CELIX_SUCCESS && hashMapIterator_hasNext(&iterator)) {
        celix_array_list_t* regs = hashMapIterator_nextValue(&iterator);
        unsigned int regIdx;
        for (regIdx = 0; (regs != NULL) && regIdx < celix_arrayList_size(regs); regIdx++) {
            service_registration_pt registration = celix_arrayList_get(regs, regIdx);
            celix_properties_t* props = NULL;

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
                    const char* className = NULL;
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
                    // assert(serviceRegistration_isValid(registration));
                    serviceRegistration_retain(registration);
                    celix_arrayList_add(matchingRegistrations, registration);
                }
            }
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    if (status == CELIX_SUCCESS) {
        unsigned int i;
        unsigned int size = celix_arrayList_size(matchingRegistrations);

        for (i = 0; i < size; i += 1) {
            service_registration_pt reg = celix_arrayList_get(matchingRegistrations, i);
            service_reference_pt reference = NULL;
            celix_status_t subStatus = serviceRegistry_getServiceReference(registry, owner, reg, &reference);
            if (subStatus == CELIX_SUCCESS) {
                celix_arrayList_add(references, reference);
            } else {
                status = CELIX_BUNDLE_EXCEPTION;
            }
            serviceRegistration_release(reg);
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = celix_steal_ptr(references);
    } else {
        // TODO: test this branch using malloc mock
        unsigned int size = celix_arrayList_size(references);
        for (unsigned int i = 0; i < size; i++) {
            serviceReference_release(celix_arrayList_get(references, i), NULL);
        }
        framework_logIfError(registry->framework->logger, status, NULL, "Cannot get service references");
    }

    return status;
}

static bool serviceRegistry_tryRemoveServiceReference(service_registry_pt registry, service_reference_pt reference) {
    size_t refCount = 0;
    size_t usageCount = 0;
    service_reference_pt ref = NULL;
    celixThreadRwlock_writeLock(&registry->lock);
    serviceReference_getReferenceCount(reference, &refCount);
    if (refCount == 0) {
        serviceReference_getUsageCount(reference, &usageCount);
        if (usageCount > 0) {
            serviceRegistry_logWarningServiceReferenceUsageCount(registry, reference->referenceOwner, reference,
                                                                 usageCount, refCount);
        }

        hash_map_pt refsMap = hashMap_get(registry->serviceReferences, reference->referenceOwner);

        unsigned long refId = 0UL;

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
            hashMap_remove(refsMap, (void *) refId);
            int size = hashMap_size(refsMap);
            if (size == 0) {
                hashMap_destroy(refsMap, false, false);
                hashMap_remove(registry->serviceReferences, reference->referenceOwner);
            }
        }
    }
    celixThreadRwlock_unlock(&registry->lock);
    return refCount == 0 && ref != NULL;

}

static void serviceRegistry_logWarningServiceReferenceUsageCount(service_registry_pt registry, bundle_pt bundle, service_reference_pt ref, size_t usageCount, size_t refCount) {
    if (usageCount > 0) {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_WARNING, "Service Reference destroyed with usage count is %zu, expected 0. Look for missing bundleContext_ungetService calls.", usageCount);
    }
    if (refCount > 0) {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_WARNING, "Dangling service reference. Reference count is %zu, expected 1.  Look for missing bundleContext_ungetServiceReference calls.", refCount);
    }

    if (usageCount > 0 || refCount > 0) {
        const char* bundle_name = celix_bundle_getSymbolicName(bundle);
        const char* service_name = "unknown";
        const char* bundle_provider_name = "unknown";
        if (refCount > 0 && ref != NULL) {
            serviceReference_getProperty(ref, CELIX_FRAMEWORK_SERVICE_NAME, &service_name);
            service_registration_pt reg = NULL;
            bundle_pt providedBnd = NULL;
            serviceReference_getServiceRegistration(ref, &reg);
            if (reg != NULL) {
                serviceRegistration_getBundle(reg, &providedBnd);
                if (providedBnd != NULL) {
                    bundle_provider_name = celix_bundle_getSymbolicName(providedBnd);
                }
            }
        }

        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_WARNING, "Previous Dangling service reference warnings caused by bundle '%s', for service '%s', provided by bundle '%s'", bundle_name, service_name, bundle_provider_name);
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

            bool destroyed = false;
            while (!destroyed) {
                serviceReference_release(ref, &destroyed);
            }
        }
        hashMapIterator_destroy(iter);
        hashMap_destroy(refsMap, false, false);
    }

    celixThreadRwlock_unlock(&registry->lock);

    return status;
}

celix_status_t
serviceRegistry_getServicesInUse(service_registry_pt registry, bundle_pt bundle, celix_array_list_t** out) {
    celix_array_list_t* result = celix_arrayList_create();

    // LOCK
    celixThreadRwlock_readLock(&registry->lock);

    hash_map_pt refsMap = hashMap_get(registry->serviceReferences, bundle);

    if (refsMap) {
        hash_map_iterator_pt iter = hashMapIterator_create(refsMap);
        while (hashMapIterator_hasNext(iter)) {
            service_reference_pt ref = hashMapIterator_nextValue(iter);
            celix_arrayList_add(result, ref);
        }
        hashMapIterator_destroy(iter);
    }

    // UNLOCK
    celixThreadRwlock_unlock(&registry->lock);

    *out = result;

    return CELIX_SUCCESS;
}

static celix_status_t serviceRegistry_addHooks(service_registry_pt registry, const char* serviceName, const void* serviceObject, service_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;

    struct listener_hook_info info = { .context = NULL, .filter = NULL, .removed = false };
    celix_array_list_t* infos = NULL;
    celix_array_list_t* listeners = NULL;
    celix_service_registry_listener_hook_entry_t* entry = NULL;


    infos = celix_arrayList_create();
    celix_arrayList_add(infos, &info);
    listeners = celix_arrayList_create();

    celixThreadRwlock_writeLock(&registry->lock);
    long svcId = serviceRegistration_getServiceId(registration);
    entry = celix_createHookEntry(svcId, (celix_listener_hook_service_t*)serviceObject);
    celix_increaseCountHook(entry);
    celix_arrayList_add(registry->listenerHooks, entry);

    for (int i = 0; i < celix_arrayList_size(registry->serviceListeners); ++i) {
        celix_service_registry_service_listener_entry_t *listenerEntry = celix_arrayList_get(registry->serviceListeners, i);
        celix_increaseCountServiceListener(listenerEntry);
        celix_arrayList_add(listeners, listenerEntry);
    }
    celixThreadRwlock_unlock(&registry->lock);

    for (int i = 0; i < celix_arrayList_size(listeners); ++i) {
        celix_service_registry_service_listener_entry_t *listenerEntry = celix_arrayList_get(listeners, i);
        info.context = celix_bundle_getContext(listenerEntry->bundle);
        info.filter = celix_filter_getFilterString(listenerEntry->filter);
        entry->hook->added(entry->hook->handle, infos);
        celix_decreaseCountServiceListener(listenerEntry);
    }
    celix_decreaseCountHook(entry);
    celix_arrayList_destroy(listeners);
    celix_arrayList_destroy(infos);

    return status;
}

static celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration) {
    celix_status_t status = CELIX_SUCCESS;

    long svcId = serviceRegistration_getServiceId(registration);

    struct listener_hook_info info = { .context = NULL, .filter = NULL, .removed = true };
    celix_array_list_t* infos = NULL;
    celix_array_list_t* listeners = NULL;
    celix_service_registry_listener_hook_entry_t *removedEntry = NULL;

    celixThreadRwlock_writeLock(&registry->lock);
    for (int i = 0; i < celix_arrayList_size(registry->listenerHooks); ++i) {
        celix_service_registry_listener_hook_entry_t *visit = celix_arrayList_get(registry->listenerHooks, i);
        if (visit->svcId == svcId) {
            removedEntry = visit;
            celix_arrayList_removeAt(registry->listenerHooks, i);
            celix_increaseCountHook(removedEntry);
            break;
        }
    }
    if (removedEntry != NULL) {
        infos = celix_arrayList_create();
        celix_arrayList_add(infos, &info);
        listeners = celix_arrayList_create();
        for (int i = 0; i < celix_arrayList_size(registry->serviceListeners); ++i) {
            celix_service_registry_service_listener_entry_t *listenerEntry = celix_arrayList_get(registry->serviceListeners, i);
            celix_increaseCountServiceListener(listenerEntry);
            celix_arrayList_add(listeners, listenerEntry);
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    if (removedEntry != NULL) {
        for (int i = 0; i < celix_arrayList_size(listeners); ++i) {
            celix_service_registry_service_listener_entry_t *listenerEntry = celix_arrayList_get(listeners, i);
            info.context = celix_bundle_getContext(listenerEntry->bundle);
            info.filter = celix_filter_getFilterString(listenerEntry->filter);
            removedEntry->hook->removed(removedEntry->hook->handle, infos);
            celix_decreaseCountServiceListener(listenerEntry);
        }
        celix_decreaseCountHook(removedEntry);
        celix_arrayList_destroy(listeners);
        celix_arrayList_destroy(infos);

        celix_waitAndDestroyHookEntry(removedEntry);
    }

    return status;
}

static void serviceRegistry_callHooksForListenerFilter(service_registry_pt registry, celix_bundle_t *owner, const celix_filter_t *filter, bool removed) {
    celix_bundle_context_t *ctx = celix_bundle_getContext(owner);

    struct listener_hook_info info;
    info.context = ctx;
    info.removed = removed;
    info.filter = celix_filter_getFilterString(filter);
    celix_array_list_t *infos = celix_arrayList_create();
    celix_arrayList_add(infos, &info);

    celix_array_list_t *hookRegistrations = celix_arrayList_create();

    celixThreadRwlock_readLock(&registry->lock);
    unsigned size = celix_arrayList_size(registry->listenerHooks);
    for (int i = 0; i < size; ++i) {
        celix_service_registry_listener_hook_entry_t* entry = celix_arrayList_get(registry->listenerHooks, i);
        if (entry != NULL) {
            celix_increaseCountHook(entry); //increate use count to ensure the hook cannot be removed, untill used
            celix_arrayList_add(hookRegistrations, entry);
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    for (int i = 0; i < celix_arrayList_size(hookRegistrations); ++i) {
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
    unsigned size = celix_arrayList_size(registry->listenerHooks);
    celixThreadRwlock_unlock(&registry->lock);
    return (size_t) size;
}

static celix_status_t serviceRegistry_getUsingBundles(service_registry_pt registry, service_registration_pt registration, celix_array_list_t** out) {
    celix_array_list_t* bundles = NULL;
    hash_map_iterator_pt iter;

    bundles = celix_arrayList_create();
    if (bundles == NULL) {
        return CELIX_ENOMEM;
    }

    celixThreadRwlock_readLock(&registry->lock);
    iter = hashMapIterator_create(registry->serviceReferences);
    while (hashMapIterator_hasNext(iter)) {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
        bundle_pt registrationUser = hashMapEntry_getKey(entry);
        hash_map_pt regMap = hashMapEntry_getValue(entry);
        if (hashMap_containsKey(regMap, (void*)registration->serviceId)) {
            celix_arrayList_add(bundles, registrationUser);
        }
    }
    hashMapIterator_destroy(iter);
    celixThreadRwlock_unlock(&registry->lock);

    *out = bundles;

    return CELIX_SUCCESS;
}

celix_status_t
celix_serviceRegistry_registerServiceFactory(
        celix_service_registry_t *reg,
        const celix_bundle_t *bnd,
        const char *serviceName,
        celix_service_factory_t *factory,
        celix_properties_t* props,
        long reserveId,
        service_registration_t **registration) {
    return serviceRegistry_registerServiceInternal(reg, (celix_bundle_t*)bnd, serviceName, (const void *) factory, props, reserveId, CELIX_FACTORY_SERVICE, registration);
}

celix_status_t
celix_serviceRegistry_registerService(
        celix_service_registry_t *reg,
        const celix_bundle_t *bnd,
        const char *serviceName,
        void* service,
        celix_properties_t* props,
        long reserveId,
        service_registration_t **registration) {
    return serviceRegistry_registerServiceInternal(reg, (celix_bundle_t*)bnd, serviceName, (const void *) service, props, reserveId, CELIX_PLAIN_SERVICE, registration);
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
                fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_WARNING,
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

char* celix_serviceRegistry_createFilterFor(celix_service_registry_t* registry, const char* serviceName, const char* versionRangeStr, const char* additionalFilterIn) {
    char* filter = NULL;

    if (serviceName == NULL) {
        serviceName = "*";
    }

    celix_autofree char* versionRange = NULL;
    if (versionRangeStr != NULL) {
        celix_autoptr(celix_version_range_t) range = celix_versionRange_parse(versionRangeStr);
        if(range == NULL) {
            celix_framework_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__,
                          "Error incorrect version range.");
            return NULL;
        }
        versionRange = celix_versionRange_createLDAPFilter(range, CELIX_FRAMEWORK_SERVICE_VERSION);
        if (versionRange == NULL) {
            celix_framework_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__,
                          "Error creating LDAP filter.");
            return NULL;
        }
    }

    //setting filter
    if (additionalFilterIn != NULL && versionRange != NULL) {
        asprintf(&filter, "(&(%s=%s)%s%s)", CELIX_FRAMEWORK_SERVICE_NAME, serviceName, versionRange, additionalFilterIn);
    } else if (versionRange != NULL) {
        asprintf(&filter, "(&(%s=%s)%s)", CELIX_FRAMEWORK_SERVICE_NAME, serviceName, versionRange);
    } else if (additionalFilterIn != NULL) {
        asprintf(&filter, "(&(%s=%s)%s)", CELIX_FRAMEWORK_SERVICE_NAME, serviceName, additionalFilterIn);
    } else {
        asprintf(&filter, "(&(%s=%s))", CELIX_FRAMEWORK_SERVICE_NAME, serviceName);
    }

    return filter;
}

static int celix_serviceRegistry_compareRegistrations(celix_array_list_entry_t a, celix_array_list_entry_t b) {
    const service_registration_t* regA = a.voidPtrVal;
    const service_registration_t* regB = b.voidPtrVal;

    celix_properties_t* propsA = NULL;
    celix_properties_t* propsB = NULL;
    serviceRegistration_getProperties((service_registration_t*)regA, &propsA);
    serviceRegistration_getProperties((service_registration_t*)regB, &propsB);

    long servIdA = celix_properties_getAsLong(propsA, CELIX_FRAMEWORK_SERVICE_ID, 0);
    long servIdB = celix_properties_getAsLong(propsB, CELIX_FRAMEWORK_SERVICE_ID, 0);

    long servRankingA = celix_properties_getAsLong(propsA, CELIX_FRAMEWORK_SERVICE_RANKING, 0);
    long servRankingB = celix_properties_getAsLong(propsB, CELIX_FRAMEWORK_SERVICE_RANKING, 0);

    return celix_utils_compareServiceIdsAndRanking(servIdA, servRankingA, servIdB, servRankingB);
}

celix_array_list_t* celix_serviceRegistry_findServices(
        celix_service_registry_t* registry,
        const char* filterStr) {

    celix_autoptr(celix_filter_t) filter = celix_filter_create(filterStr);
    if (filter == NULL) {
        celix_framework_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, __FUNCTION__, __BASE_FILE__, __LINE__,
                      "Error incorrect filter.");
        return NULL;
    }

    celix_array_list_t *result = celix_arrayList_create();
    celix_array_list_t* matchedRegistrations = celix_arrayList_create();

    celixThreadRwlock_readLock(&registry->lock);

    hash_map_iterator_t iter = hashMapIterator_construct(registry->serviceRegistrations);
    while (hashMapIterator_hasNext(&iter)) {
        celix_array_list_t *regs = hashMapIterator_nextValue(&iter);
        for (int i = 0; i < celix_arrayList_size(regs); ++i) {
            service_registration_t *reg = celix_arrayList_get(regs, i);
            celix_properties_t* svcProps = NULL;
            serviceRegistration_getProperties(reg, &svcProps);
            if (svcProps != NULL && celix_filter_match(filter, svcProps)) {
                celix_arrayList_add(matchedRegistrations, reg);
            }
        }
    }

    //sort matched registration and add the svc id to the result list.
    if (celix_arrayList_size(matchedRegistrations) > 1) {
        celix_arrayList_sortEntries(matchedRegistrations, celix_serviceRegistry_compareRegistrations);
    }
    for (int i = 0; i < celix_arrayList_size(matchedRegistrations); ++i) {
        service_registration_t* reg = celix_arrayList_get(matchedRegistrations, i);
        celix_arrayList_addLong(result, serviceRegistration_getServiceId(reg));
    }
    celixThreadRwlock_unlock(&registry->lock);

    celix_arrayList_destroy(matchedRegistrations);
    return result;
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
            fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot add service listener filter '%s' is invalid", stringFilter);
            celix_framework_logTssErrors(registry->framework->logger, CELIX_LOG_LEVEL_ERROR);
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

    celix_array_list_t *references =  celix_arrayList_create();

    celixThreadRwlock_writeLock(&registry->lock);
    celix_arrayList_add(registry->serviceListeners, entry); //use count 1

    //find already registered services
    hash_map_iterator_t iter = hashMapIterator_construct(registry->serviceRegistrations);
    while (hashMapIterator_hasNext(&iter)) {
        celix_array_list_t *regs = (celix_array_list_t*) hashMapIterator_nextValue(&iter);
        for (int regIdx = 0; (regs != NULL) && regIdx < celix_arrayList_size(regs); ++regIdx) {
            service_registration_pt registration = celix_arrayList_get(regs, regIdx);
            celix_properties_t* props = NULL;
            serviceRegistration_getProperties(registration, &props);
            if (celix_filter_match(filter, props)) {
                long svcId = serviceRegistration_getServiceId(registration);
                service_reference_pt ref = NULL;
                serviceRegistry_getServiceReference_internal(registry, bundle, registration, &ref);
                celix_arrayList_add(references, ref);
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

    for (int i = 0; i < celix_arrayList_size(references); ++i) {
        service_reference_pt ref = celix_arrayList_get(references, i);
        long svcId = serviceReference_getServiceId(ref);
        celix_service_event_t event;
        event.reference = ref;
        event.type = OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED;
        listener->serviceChanged(listener->handle, &event);
        serviceReference_release(ref, NULL);
        //update pending register event count
        celix_decreasePendingRegisteredEvent(registry, svcId);
    }
    celix_arrayList_destroy(references);

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
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot remove service listener, listener not found");
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
        serviceReference_release(reference, NULL);
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

long celix_serviceRegistry_nextSvcId(celix_service_registry_t* registry) {
    long scvId = __atomic_fetch_add(&registry->nextServiceId, 1, __ATOMIC_RELAXED);
    return scvId;
}

bool celix_serviceRegistry_isServiceRegistered(celix_service_registry_t* reg, long serviceId) {
    bool isRegistered = false;
    if (serviceId >= 0) {
        celixThreadRwlock_readLock(&reg->lock);
        hash_map_iterator_t iter = hashMapIterator_construct(reg->serviceRegistrations);
        while (!isRegistered && hashMapIterator_hasNext(&iter)) {
            celix_array_list_t *regs = hashMapIterator_nextValue(&iter);
            for (int i = 0; i < celix_arrayList_size(regs); ++i) {
                service_registration_t* r = celix_arrayList_get(regs, i);
                if (serviceId == serviceRegistration_getServiceId(r)) {
                    isRegistered = true;
                    break;
                }
            }
        }
        celixThreadRwlock_unlock(&reg->lock);
    }
    return isRegistered;
}

void celix_serviceRegistry_unregisterService(celix_service_registry_t* registry, celix_bundle_t* bnd, long serviceId) {
    service_registration_t *reg = NULL;
    celixThreadRwlock_readLock(&registry->lock);
    celix_array_list_t* registrations = hashMap_get(registry->serviceRegistrations, (void*)bnd);
    if (registrations != NULL) {
        for (int i = 0; i < celix_arrayList_size(registrations); ++i) {
            service_registration_t *entry = celix_arrayList_get(registrations, i);
            if (serviceRegistration_getServiceId(entry) == serviceId) {
                reg = entry;
                serviceRegistration_retain(reg); // protect against concurrently unregistering the same serviceId multiple times
                break;
            }
        }
    }
    celixThreadRwlock_unlock(&registry->lock);

    if (reg != NULL) {
        serviceRegistration_unregister(reg);
        serviceRegistration_release(reg);
    } else {
        fw_log(registry->framework->logger, CELIX_LOG_LEVEL_ERROR, "Cannot unregister service for service id %li. This id is not present or owned by the provided bundle (bnd id %li)", serviceId, celix_bundle_getId(bnd));
    }
}
