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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "service_reference.h"

#include "service_registry.h"
#include "service_reference_private.h"
#include "service_registration_private.h"
#include "module.h"
#include "wire.h"
#include "bundle.h"
#include "celix_log.h"


celix_status_t serviceReference_create(bundle_pt bundle, service_registration_pt registration, service_reference_pt *reference) {
	celix_status_t status = CELIX_SUCCESS;

	*reference = malloc(sizeof(**reference));
	if (!*reference) {
		status = CELIX_ENOMEM;
	} else {

		(*reference)->bundle = bundle;
		(*reference)->registration = registration;
	}

	framework_logIfError(logger, status, NULL, "Cannot create service reference");

	return status;
}

celix_status_t serviceReference_destroy(service_reference_pt *reference) {
	(*reference)->bundle = NULL;
	(*reference)->registration = NULL;
	free(*reference);
	*reference = NULL;
	return CELIX_SUCCESS;
}

celix_status_t serviceReference_getBundle(service_reference_pt reference, bundle_pt *bundle) {
	*bundle = reference->bundle;
	return CELIX_SUCCESS;
}

celix_status_t serviceReference_getServiceRegistration(service_reference_pt reference, service_registration_pt *registration) {
	*registration = reference->registration;
	return CELIX_SUCCESS;
}

celix_status_t serviceReference_getProperty(service_reference_pt reference, char *key, char **value) {
    celix_status_t status = CELIX_SUCCESS;
    properties_pt props = NULL;

    serviceRegistration_getProperties(reference->registration, &props);
    *value = properties_get(props, key);

    return status;
}

FRAMEWORK_EXPORT celix_status_t serviceReference_getPropertyKeys(service_reference_pt reference, char **keys[], unsigned int *size) {
    celix_status_t status = CELIX_SUCCESS;
    properties_pt props = NULL;

    serviceRegistration_getProperties(reference->registration, &props);
    hash_map_iterator_pt it;
    int i = 0;
    int vsize = hashMap_size(props);
    *size = vsize;
    *keys = malloc(vsize * sizeof(*keys));
    it = hashMapIterator_create(props);
    while (hashMapIterator_hasNext(it)) {
        (*keys)[i] = hashMapIterator_nextKey(it);
        i++;
    }
    hashMapIterator_destroy(it);

    return status;
}

celix_status_t serviceReference_invalidate(service_reference_pt reference) {
	reference->registration = NULL;
	return CELIX_SUCCESS;
}

celix_status_t serviceRefernce_isValid(service_reference_pt reference, bool *result) {
	(*result) = reference->registration != NULL;
	return CELIX_SUCCESS;
}

bool serviceReference_isAssignableTo(service_reference_pt reference, bundle_pt requester, char * serviceName) {
	bool allow = true;

	bundle_pt provider = reference->bundle;
	if (requester == provider) {
		return allow;
	}
//	wire_pt providerWire = module_getWire(bundle_getCurrentModule(provider), serviceName);
//	wire_pt requesterWire = module_getWire(bundle_getCurrentModule(requester), serviceName);
//
//	if (providerWire == NULL && requesterWire != NULL) {
//		allow = (bundle_getCurrentModule(provider) == wire_getExporter(requesterWire));
//	} else if (providerWire != NULL && requesterWire != NULL) {
//		allow = (wire_getExporter(providerWire) == wire_getExporter(requesterWire));
//	} else if (providerWire != NULL && requesterWire == NULL) {
//		allow = (wire_getExporter(providerWire) == bundle_getCurrentModule(requester));
//	} else {
//		allow = false;
//	}

	return allow;
}

celix_status_t serviceReference_getUsingBundles(service_reference_pt reference, array_list_pt *bundles) {
	celix_status_t status = CELIX_SUCCESS;

	service_registry_pt registry = NULL;
	serviceRegistration_getRegistry(reference->registration, &registry);

	*bundles = serviceRegistry_getUsingBundles(registry, reference);

	return status;
}

celix_status_t serviceReference_equals(service_reference_pt reference, service_reference_pt compareTo, bool *equal) {
	*equal = (reference->registration == compareTo->registration);
	return CELIX_SUCCESS;
}

int serviceReference_equals2(void *reference1, void *reference2) {
	bool equal;
	serviceReference_equals(reference1, reference2, &equal);
	return equal;
}

unsigned int serviceReference_hashCode(void *referenceP) {
	service_reference_pt reference = referenceP;
	int prime = 31;
	int result = 1;
	result = prime * result;

	if (reference != NULL) {
		intptr_t bundleA = (intptr_t) reference->bundle;
		intptr_t registrationA = (intptr_t) reference->registration;

		result += bundleA + registrationA;
	}
	return result;
}

