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
 * service_reference.c
 *
 *  \date       Jul 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <constants.h>
#include <stdint.h>
#include <utils.h>
#include <assert.h>

#include "service_reference.h"

#include "service_reference_private.h"
#include "service_registration_private.h"

static void serviceReference_destroy(service_reference_pt);
static void serviceReference_logWarningUsageCountBelowZero(service_reference_pt ref);

celix_status_t serviceReference_create(registry_callback_t callback, bundle_pt referenceOwner, service_registration_pt registration,  service_reference_pt *out) {
	celix_status_t status = CELIX_SUCCESS;

	service_reference_pt ref = calloc(1, sizeof(*ref));
	if (!ref) {
		status = CELIX_ENOMEM;
	} else {
        ref->callback = callback;
		ref->referenceOwner = referenceOwner;
		ref->registration = registration;
        ref->service = NULL;
        serviceRegistration_getBundle(registration, &ref->registrationBundle);
		celixThreadRwlock_create(&ref->lock, NULL);
		ref->refCount = 1;
        ref->usageCount = 0;

        serviceRegistration_retain(ref->registration);
	}

	if (status == CELIX_SUCCESS) {
		*out = ref;
	}

    framework_logIfError(logger, status, NULL, "Cannot create service reference");

	return status;
}

celix_status_t serviceReference_retain(service_reference_pt ref) {
    celixThreadRwlock_writeLock(&ref->lock);
    ref->refCount += 1;
    celixThreadRwlock_unlock(&ref->lock);
    return CELIX_SUCCESS;
}

celix_status_t serviceReference_release(service_reference_pt ref, bool *out) {
    bool destroyed = false;
    celixThreadRwlock_writeLock(&ref->lock);
    assert(ref->refCount > 0);
    ref->refCount -= 1;
    if (ref->refCount == 0) {
        if (ref->registration != NULL) {
            serviceRegistration_release(ref->registration);
        }
        celixThreadRwlock_unlock(&ref->lock);
        serviceReference_destroy(ref);
        destroyed = true;
    } else {
        celixThreadRwlock_unlock(&ref->lock);
    }

    if (out) {
        *out = destroyed;
    }
    return CELIX_SUCCESS;
}

celix_status_t serviceReference_increaseUsage(service_reference_pt ref, size_t *out) {
    //fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "Destroying service reference %p\n", ref);
    size_t local = 0;
    celixThreadRwlock_writeLock(&ref->lock);
    ref->usageCount += 1;
    local = ref->usageCount;
    celixThreadRwlock_unlock(&ref->lock);
    if (out) {
        *out = local;
    }
    return CELIX_SUCCESS;
}

celix_status_t serviceReference_decreaseUsage(service_reference_pt ref, size_t *out) {
    celix_status_t status = CELIX_SUCCESS;
    size_t localCount = 0;
    celixThreadRwlock_writeLock(&ref->lock);
    if (ref->usageCount == 0) {
        serviceReference_logWarningUsageCountBelowZero(ref);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        ref->usageCount -= 1;
    }
    localCount = ref->usageCount;
    celixThreadRwlock_unlock(&ref->lock);

    if (out) {
        *out = localCount;
    }
    return status;
}

static void serviceReference_logWarningUsageCountBelowZero(service_reference_pt ref __attribute__((unused))) {
    fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot decrease service usage count below 0\n");
}


celix_status_t serviceReference_getUsageCount(service_reference_pt ref, size_t *count) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&ref->lock);
    *count = ref->usageCount;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_getReferenceCount(service_reference_pt ref, size_t *count) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&ref->lock);
    *count = ref->refCount;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_getService(service_reference_pt ref, void **service) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&ref->lock);
    /*NOTE the service argument should be 'const void**'
      To ensure backwards compatability a cast is made instead.
    */
    *service = (const void**) ref->service;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_setService(service_reference_pt ref, const void *service) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_writeLock(&ref->lock);
    ref->service = service;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

static void serviceReference_destroy(service_reference_pt ref) {
	assert(ref->refCount == 0);
    celixThreadRwlock_destroy(&ref->lock);
	ref->registration = NULL;
	free(ref);
}

celix_status_t serviceReference_getBundle(service_reference_pt ref, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&ref->lock);
    if (ref->registration != NULL) {
        *bundle = ref->registrationBundle;
    }
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_getOwner(service_reference_pt ref, bundle_pt *owner) { 
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&ref->lock);
    *owner = ref->referenceOwner;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_getServiceRegistration(service_reference_pt ref, service_registration_pt *out) {
    if (ref != NULL) {
        celixThreadRwlock_readLock(&ref->lock);
        *out = ref->registration;
        celixThreadRwlock_unlock(&ref->lock);
        return CELIX_SUCCESS;
    } else {
        return CELIX_ILLEGAL_ARGUMENT;
    }
}

FRAMEWORK_EXPORT celix_status_t
serviceReference_getPropertyWithDefault(service_reference_pt ref, const char *key, const char* def, const char **value) {
    celix_status_t status = CELIX_SUCCESS;
    properties_pt props = NULL;
    celixThreadRwlock_readLock(&ref->lock);
    if (ref->registration != NULL) {
        status = serviceRegistration_getProperties(ref->registration, &props);
        if (status == CELIX_SUCCESS) {
            *value = (char*) properties_getWithDefault(props, key, def);
        }
    } else {
        *value = NULL;
    }
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_getProperty(service_reference_pt ref, const char* key, const char** value) {
    return serviceReference_getPropertyWithDefault(ref, key, NULL, value);
}

FRAMEWORK_EXPORT celix_status_t serviceReference_getPropertyKeys(service_reference_pt ref, char **keys[], unsigned int *size) {
    celix_status_t status = CELIX_SUCCESS;
    properties_pt props = NULL;

    celixThreadRwlock_readLock(&ref->lock);
    serviceRegistration_getProperties(ref->registration, &props);
    hash_map_iterator_pt it;
    int i = 0;
    int vsize = hashMap_size(props);
    *size = (unsigned int)vsize;
    *keys = malloc(vsize * sizeof(**keys));
    it = hashMapIterator_create(props);
    while (hashMapIterator_hasNext(it)) {
        (*keys)[i] = hashMapIterator_nextKey(it);
        i++;
    }
    hashMapIterator_destroy(it);
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_invalidate(service_reference_pt ref) {
    assert(ref != NULL);
    celix_status_t status = CELIX_SUCCESS;
    service_registration_pt reg = NULL;
    celixThreadRwlock_writeLock(&ref->lock);
    reg = ref->registration;
    ref->registration = NULL;
    celixThreadRwlock_unlock(&ref->lock);

    if (reg != NULL) {
        serviceRegistration_release(reg);
    }
	return status;
}

celix_status_t serviceReference_isValid(service_reference_pt ref, bool *result) {
    celixThreadRwlock_readLock(&ref->lock);
    (*result) = ref->registration != NULL;
    celixThreadRwlock_unlock(&ref->lock);
    return CELIX_SUCCESS;
}

bool serviceReference_isAssignableTo(service_reference_pt reference __attribute__((unused)), bundle_pt requester __attribute__((unused)), const char* serviceName __attribute__((unused))) {
	bool allow = true;

	/*NOTE for now always true. It would be nice to be able to do somechecks if the services are really assignable.
	 */

	return allow;
}

celix_status_t serviceReference_equals(service_reference_pt reference, service_reference_pt compareTo, bool *equal) {
    celix_status_t status = CELIX_SUCCESS;
    if (reference != NULL && compareTo != NULL) {
        service_registration_pt reg1;
        service_registration_pt reg2;
        serviceReference_getServiceRegistration(reference, &reg1);
        serviceReference_getServiceRegistration(compareTo, &reg2);
        *equal = (reg1 == reg2);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
        *equal = false;
    }
	return status;
}

int serviceReference_equals2(const void* reference1, const void* reference2) {
	bool equal;
	serviceReference_equals((service_reference_pt)reference1, (service_reference_pt)reference2, &equal);
	return equal;
}

celix_status_t serviceReference_compareTo(service_reference_pt reference, service_reference_pt compareTo, int *compare) {
	celix_status_t status = CELIX_SUCCESS;

	long id, other_id;
	const char* id_str;
    const char* other_id_str;
	serviceReference_getProperty(reference, (char *) OSGI_FRAMEWORK_SERVICE_ID, &id_str);
	serviceReference_getProperty(compareTo, (char *) OSGI_FRAMEWORK_SERVICE_ID, &other_id_str);

	id = atol(id_str);
	other_id = atol(other_id_str);


	long rank, other_rank;
	const char *rank_str;
    const char* other_rank_str;
	serviceReference_getProperty(reference, OSGI_FRAMEWORK_SERVICE_RANKING, &rank_str);
	serviceReference_getProperty(compareTo, OSGI_FRAMEWORK_SERVICE_RANKING, &other_rank_str);

	rank = rank_str == NULL ? 0 : atol(rank_str);
	other_rank = other_rank_str == NULL ? 0 : atol(other_rank_str);

    *compare = utils_compareServiceIdsAndRanking(id, rank, other_id, other_rank);

	return status;
}

unsigned int serviceReference_hashCode(const void *referenceP) {
    service_reference_pt ref = (service_reference_pt)referenceP;
    bundle_pt bundle = NULL;
    service_registration_pt reg = NULL;

    if (ref != NULL) {
        celixThreadRwlock_readLock(&ref->lock);
        bundle = ref->registrationBundle;
        reg = ref->registration;
        celixThreadRwlock_unlock(&ref->lock);
    }


	int prime = 31;
	int result = 1;
	result = prime * result;

	if (bundle != NULL && reg != NULL) {
		intptr_t bundleA = (intptr_t) bundle;
		intptr_t registrationA = (intptr_t) reg;

		result += bundleA + registrationA;
	}
	return result;
}


celix_status_t serviceReference_getUsingBundles(service_reference_pt ref, array_list_pt *out) {
    celix_status_t status = CELIX_SUCCESS;
    service_registration_pt reg = NULL;
    registry_callback_t callback;

    callback.getUsingBundles = NULL;


    celixThreadRwlock_readLock(&ref->lock);
    reg = ref->registration;
    if (reg != NULL) {
        serviceRegistration_retain(reg);
        callback.handle = ref->callback.handle;
        callback.getUsingBundles = ref->callback.getUsingBundles;
    }
    celixThreadRwlock_unlock(&ref->lock);

    if (reg != NULL) {
        if (callback.getUsingBundles != NULL) {
            status = callback.getUsingBundles(callback.handle, reg, out);
        } else {
            fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "getUsingBundles callback not set");
            status = CELIX_BUNDLE_EXCEPTION;
        }
        serviceRegistration_release(reg);
    }

    return status;
}

