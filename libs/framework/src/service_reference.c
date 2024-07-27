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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <utils.h>
#include <assert.h>

#include "service_reference.h"
#include "celix_compiler.h"
#include "celix_constants.h"
#include "celix_build_assert.h"
#include "celix_log.h"
#include "service_reference_private.h"
#include "service_registration_private.h"

static bool serviceReference_doRelease(struct celix_ref *);
static void serviceReference_destroy(service_reference_pt);

celix_status_t serviceReference_create(registry_callback_t callback, bundle_pt referenceOwner, service_registration_pt registration,  service_reference_pt *out) {
	celix_status_t status = CELIX_SUCCESS;

	service_reference_pt ref = calloc(1, sizeof(*ref));
	if (!ref) {
		status = CELIX_ENOMEM;
	} else {
        celix_ref_init(&ref->refCount);
        ref->callback = callback;
		ref->referenceOwner = referenceOwner;
		ref->registration = registration;
        ref->serviceCache = NULL;
        serviceRegistration_getBundle(registration, &ref->registrationBundle);
		celixThreadRwlock_create(&ref->lock, NULL);
        ref->usageCount = 0;
        serviceRegistration_retain(ref->registration);
	}

	if (status == CELIX_SUCCESS) {
		*out = ref;
	}

    framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create service reference");

	return status;
}

celix_status_t serviceReference_retain(service_reference_pt ref) {
    celix_ref_get(&ref->refCount);
    return CELIX_SUCCESS;
}

celix_status_t serviceReference_release(service_reference_pt ref, bool *out) {
    bool destroyed = false;
    destroyed = celix_ref_put(&ref->refCount, serviceReference_doRelease);
    if (out) {
        *out = destroyed;
    }
    return CELIX_SUCCESS;
}

static bool serviceReference_doRelease(struct celix_ref *refCount) {
    service_reference_pt ref = (service_reference_pt)refCount;
    bool removed = true;
    CELIX_BUILD_ASSERT(offsetof(struct serviceReference, refCount) == 0);
    // now the reference is dying in the registry, to stop anyone from reviving it, we must remove it from the registry.
    // suppose another thread revives the reference and release it immediately, we may end up with two (or more) callers of tryRemoveServiceReference.
    // it's the registry's responsibility to guarantee that only one caller get the chance to actually perform cleanup
    removed = ref->callback.tryRemoveServiceReference(ref->callback.handle, ref);
    if(removed) {
        serviceReference_invalidateCache(ref);
        serviceRegistration_release(ref->registration);
        serviceReference_destroy(ref);
    }
    return removed;
}

celix_status_t serviceReference_getUsageCount(service_reference_pt ref, size_t *count) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_readLock(&ref->lock);
    *count = ref->usageCount;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_getReferenceCount(service_reference_pt ref, size_t *count) {
    *count = celix_ref_read(&ref->refCount);
    return CELIX_SUCCESS;
}

celix_status_t serviceReference_getService(service_reference_pt ref, const void **service)
{
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_writeLock(&ref->lock);
    if(ref->usageCount == 0) {
        assert(ref->serviceCache == NULL);
        status  = serviceRegistration_getService(ref->registration, ref->referenceOwner, &ref->serviceCache);
    }
    if(status == CELIX_SUCCESS) {
        ref->usageCount += 1;
    }
    *service = ref->serviceCache;
    celixThreadRwlock_unlock(&ref->lock);
    return status;
}

celix_status_t serviceReference_ungetService(service_reference_pt ref, bool *result)
{
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_writeLock(&ref->lock);
    if (ref->usageCount == 0) {
        assert(ref->serviceCache == NULL);
        fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_WARNING, "Cannot decrease service usage count below 0\n");
    } else {
        ref->usageCount -= 1;
    }
    if(ref->usageCount == 0 && ref->serviceCache != NULL) {
        status = serviceRegistration_ungetService(ref->registration, ref->referenceOwner, &ref->serviceCache);
    }
    celixThreadRwlock_unlock(&ref->lock);
    if(result != NULL) {
        *result = (status == CELIX_SUCCESS);
    }
    return status;
}

static void serviceReference_destroy(service_reference_pt ref) {
	assert(ref->refCount.count == 0);
    celixThreadRwlock_destroy(&ref->lock);
	ref->registration = NULL;
	free(ref);
}

celix_status_t serviceReference_getBundle(service_reference_pt ref, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;
    *bundle = ref->registrationBundle;
    return status;
}

celix_status_t serviceReference_getOwner(service_reference_pt ref, bundle_pt *owner) { 
    celix_status_t status = CELIX_SUCCESS;
    *owner = ref->referenceOwner;
    return status;
}

celix_status_t serviceReference_getServiceRegistration(service_reference_pt ref, service_registration_pt *out) {
    if (ref != NULL) {
        *out = ref->registration;
        return CELIX_SUCCESS;
    } else {
        return CELIX_ILLEGAL_ARGUMENT;
    }
}

long serviceReference_getServiceId(service_reference_pt ref) {
    return serviceRegistration_getServiceId(ref->registration);
}



celix_status_t
serviceReference_getPropertyWithDefault(service_reference_pt ref, const char *key, const char* def, const char **value) {
    celix_status_t status = CELIX_SUCCESS;
    celix_properties_t* props = NULL;
    status = serviceRegistration_getProperties(ref->registration, &props);
    if (status == CELIX_SUCCESS) {
        *value = (char*) celix_properties_get(props, key, def);
    }
    return status;
}

celix_status_t serviceReference_getProperty(service_reference_pt ref, const char* key, const char** value) {
    return serviceReference_getPropertyWithDefault(ref, key, NULL, value);
}

celix_status_t serviceReference_getPropertyKeys(service_reference_pt ref, char **keys[], unsigned int *size) {
    if (!keys || !size) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_properties_t* props = NULL;
    celix_status_t status = serviceRegistration_getProperties(ref->registration, &props);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    *size = (unsigned int)celix_properties_size(props);
    *keys = malloc((*size) * sizeof(**keys));
    if (!*keys) {
        return ENOMEM;
    }

    int i = 0;
    CELIX_PROPERTIES_ITERATE(props, entry) {
        (*keys)[i++] = (char*)entry.key;
    }

    return CELIX_SUCCESS;
}

celix_status_t serviceReference_invalidateCache(service_reference_pt reference) {
    assert(reference != NULL);
    celix_status_t status = CELIX_SUCCESS;
    celixThreadRwlock_writeLock(&reference->lock);
    if(reference->serviceCache != NULL) {
        status = serviceRegistration_ungetService(reference->registration, reference->referenceOwner, &reference->serviceCache);
    }
    celixThreadRwlock_unlock(&reference->lock);
	return status;
}

bool serviceReference_isAssignableTo(service_reference_pt reference CELIX_UNUSED, bundle_pt requester CELIX_UNUSED, const char* serviceName CELIX_UNUSED) {
	bool allow = true;

	/*NOTE for now always true. It would be nice to be able to do some checks if the services are really assignable.
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
	serviceReference_getProperty(reference, (char *) CELIX_FRAMEWORK_SERVICE_ID, &id_str);
	serviceReference_getProperty(compareTo, (char *) CELIX_FRAMEWORK_SERVICE_ID, &other_id_str);

	id = atol(id_str);
	other_id = atol(other_id_str);


	long rank, other_rank;
	const char *rank_str;
    const char* other_rank_str;
	serviceReference_getProperty(reference, CELIX_FRAMEWORK_SERVICE_RANKING, &rank_str);
	serviceReference_getProperty(compareTo, CELIX_FRAMEWORK_SERVICE_RANKING, &other_rank_str);

	rank = rank_str == NULL ? 0 : atol(rank_str);
	other_rank = other_rank_str == NULL ? 0 : atol(other_rank_str);

    *compare = celix_utils_compareServiceIdsAndRanking(id, rank, other_id, other_rank);

	return status;
}

unsigned int serviceReference_hashCode(const void *referenceP) {
    service_reference_pt ref = (service_reference_pt)referenceP;
    bundle_pt bundle = NULL;
    service_registration_pt reg = NULL;

    if (ref != NULL) {
        bundle = ref->registrationBundle;
        reg = ref->registration;
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


celix_status_t serviceReference_getUsingBundles(service_reference_pt ref, celix_array_list_t** out) {
    celix_status_t status = CELIX_SUCCESS;
    registry_callback_t callback;

    callback.getUsingBundles = NULL;

    callback.handle = ref->callback.handle;
    callback.getUsingBundles = ref->callback.getUsingBundles;

    if (callback.getUsingBundles != NULL) {
        status = callback.getUsingBundles(callback.handle, ref->registration, out);
    } else {
        fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "getUsingBundles callback not set");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

