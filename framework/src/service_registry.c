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
#include <assert.h>

#include "service_registry_private.h"
#include "service_registration_private.h"
#include "listener_hook_service.h"
#include "constants.h"
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

celix_status_t serviceRegistry_create(framework_pt framework, serviceChanged_function_pt serviceChanged, service_registry_pt *out) {
	celix_status_t status;

	service_registry_pt reg = calloc(1, sizeof(*reg));
	if (reg == NULL) {
		status = CELIX_ENOMEM;
	} else {

        reg->callback.handle = reg;
        reg->callback.getUsingBundles = (void *)serviceRegistry_getUsingBundles;
        reg->callback.unregister = (void *) serviceRegistry_unregisterService;
        reg->callback.modified = (void *) serviceRegistry_servicePropertiesModified;

        reg->serviceChanged = serviceChanged;
		reg->serviceRegistrations = hashMap_create(NULL, NULL, NULL, NULL);
		reg->framework = framework;
		reg->currentServiceId = 1UL;
		reg->serviceReferences = hashMap_create(NULL, NULL, NULL, NULL);

        reg->checkDeletedReferences = CHECK_DELETED_REFERENCES;
        reg->deletedServiceReferences = hashMap_create(NULL, NULL, NULL, NULL);

		arrayList_create(&reg->listenerHooks);

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

    //destroy service registration map
    int size = hashMap_size(registry->serviceRegistrations);
    assert(size == 0);
    hashMap_destroy(registry->serviceRegistrations, false, false);

    //destroy service references (double) map);
    //FIXME. The framework bundle does not (yet) call clearReferences, as result the size could be > 0 for test code.
    //size = hashMap_size(registry->serviceReferences);
    //assert(size == 0);
    hashMap_destroy(registry->serviceReferences, false, false);

    //destroy listener hooks
    size = arrayList_size(registry->listenerHooks);
    if (size == 0)
    	arrayList_destroy(registry->listenerHooks);

    hashMap_destroy(registry->deletedServiceReferences, false, false);

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

    //long id;
    //bundle_getBundleId(bundle, &id);
    //fprintf(stderr, "REG: Registering service '%s' for bundle id %li with reg pointer %p\n", serviceName, id, *registration);


    serviceRegistry_addHooks(registry, serviceName, serviceObject, *registration);

	celixThreadRwlock_writeLock(&registry->lock);
	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs == NULL) {
		regs = NULL;
		arrayList_create(&regs);
        hashMap_put(registry->serviceRegistrations, bundle, regs);
    }
	arrayList_add(regs, *registration);
	celixThreadRwlock_unlock(&registry->lock);

	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_REGISTERED, *registration, NULL);
	}

	return CELIX_SUCCESS;
}

celix_status_t serviceRegistry_unregisterService(service_registry_pt registry, bundle_pt bundle, service_registration_pt registration) {
	// array_list_t clients;
	array_list_pt regs;

    //fprintf(stderr, "REG: Unregistering service registration with pointer %p\n", registration);

	serviceRegistry_removeHook(registry, registration);

	celixThreadRwlock_writeLock(&registry->lock);
	regs = (array_list_pt) hashMap_get(registry->serviceRegistrations, bundle);
	if (regs != NULL) {
		arrayList_removeElement(regs, registration);
        int size = arrayList_size(regs);
        if (size == 0) {
            arrayList_destroy(regs);
            hashMap_remove(registry->serviceRegistrations, bundle);
        }
	}
	celixThreadRwlock_unlock(&registry->lock);

	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING, registration, NULL);
	}


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

	if(celixThreadRwlock_writeLock(&registry->lock) == CELIX_SUCCESS) {
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
        module_pt module_ptr = NULL;
        bundle_getCurrentModule(bundle, &module_ptr);
        const char* bundle_name = NULL;
        module_getSymbolicName(module_ptr, &bundle_name);

        const char* service_name = "unknown";
        const char* bundle_provider_name = "unknown";
        if (refCount > 0 && ref != NULL) {
            serviceReference_getProperty(ref, OSGI_FRAMEWORK_OBJECTCLASS, &service_name);
            service_registration_pt reg = NULL;
            bundle_pt bundle = NULL;
            module_pt mod = NULL;
            serviceReference_getServiceRegistration(ref, &reg);
            serviceRegistration_getBundle(reg, &bundle);
            bundle_getCurrentModule(bundle, &mod);
            module_getSymbolicName(mod, &bundle_provider_name);
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
    reference_status_t refStatus;



    celixThreadRwlock_readLock(&registry->lock);
    serviceRegistry_checkReference(registry, reference, &refStatus);
    if (refStatus == REF_ACTIVE) {
        serviceReference_getServiceRegistration(reference, &registration);

        if (serviceRegistration_isValid(registration)) {
            serviceReference_increaseUsage(reference, &count);
            if (count == 1) {
                serviceRegistration_getService(registration, bundle, &service);
                serviceReference_setService(reference, service);
            }

            /* NOTE the out argument of sr_getService should be 'const void**'
               To ensure backwards compatability a cast is made instead.
            */
            serviceReference_getService(reference, (void **)out);
        } else {
            *out = NULL; //invalid service registration
        }
    } else {
        serviceRegistry_logIllegalReference(registry, reference, refStatus);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    celixThreadRwlock_unlock(&registry->lock);

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
              TO ensure backwards compatability a cast is made instead.
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
		arrayList_add(registry->listenerHooks, registration);
        celixThreadRwlock_unlock(&registry->lock);
	}

	return status;
}

static celix_status_t serviceRegistry_removeHook(service_registry_pt registry, service_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;
	const char* serviceName = NULL;

	properties_pt props = NULL;
	serviceRegistration_getProperties(registration, &props);
	serviceName = properties_get(props, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
	if (strcmp(OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, serviceName) == 0) {
        celixThreadRwlock_writeLock(&registry->lock);
		arrayList_removeElement(registry->listenerHooks, registration);
        celixThreadRwlock_unlock(&registry->lock);
	}

	return status;
}

celix_status_t serviceRegistry_getListenerHooks(service_registry_pt registry, bundle_pt owner, array_list_pt *out) {
	celix_status_t status;
    array_list_pt result;

    status = arrayList_create(&result);
    if (status == CELIX_SUCCESS) {
        unsigned int i;
        unsigned size = arrayList_size(registry->listenerHooks);

        for (i = 0; i < size; i += 1) {
            celixThreadRwlock_readLock(&registry->lock);
            service_registration_pt registration = arrayList_get(registry->listenerHooks, i);
            serviceRegistration_retain(registration);
            celixThreadRwlock_unlock(&registry->lock);

            service_reference_pt reference = NULL;
            serviceRegistry_getServiceReference(registry, owner, registration, &reference);
            arrayList_add(result, reference);
            serviceRegistration_release(registration);
        }
    }

    if (status == CELIX_SUCCESS) {
        *out = result;
    } else {
        if (result != NULL) {
            arrayList_destroy(result);
        }
        framework_logIfError(logger, status, NULL, "Cannot get listener hooks");
    }

	return status;
}

celix_status_t serviceRegistry_servicePropertiesModified(service_registry_pt registry, service_registration_pt registration, properties_pt oldprops) {
	if (registry->serviceChanged != NULL) {
		registry->serviceChanged(registry->framework, OSGI_FRAMEWORK_SERVICE_EVENT_MODIFIED, registration, oldprops);
	}
	return CELIX_SUCCESS;
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