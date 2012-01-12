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
 *  Created on: Jul 20, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>

#include "service_registry.h"
#include "service_reference.h"
#include "service_registration.h"
#include "module.h"
#include "wire.h"
#include "bundle.h"

struct serviceReference {
	BUNDLE bundle;
	struct serviceRegistration * registration;
};

apr_status_t serviceReference_destroy(void *referenceP);

celix_status_t serviceReference_create(apr_pool_t *pool, BUNDLE bundle, SERVICE_REGISTRATION registration, SERVICE_REFERENCE *reference) {
	celix_status_t status = CELIX_SUCCESS;

	*reference = apr_palloc(pool, sizeof(**reference));
	if (!*reference) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *reference, serviceReference_destroy);

		(*reference)->bundle = bundle;
		(*reference)->registration = registration;
	}

	return status;
}

apr_status_t serviceReference_destroy(void *referenceP) {
	SERVICE_REFERENCE reference = referenceP;
	reference->bundle = NULL;
	reference->registration = NULL;
	return APR_SUCCESS;
}

celix_status_t serviceReference_getBundle(SERVICE_REFERENCE reference, BUNDLE *bundle) {
	*bundle = reference->bundle;
	return CELIX_SUCCESS;
}

celix_status_t serviceReference_getServiceRegistration(SERVICE_REFERENCE reference, SERVICE_REGISTRATION *registration) {
	*registration = reference->registration;
	return CELIX_SUCCESS;
}

celix_status_t serviceReference_invalidate(SERVICE_REFERENCE reference) {
	reference->registration = NULL;
	return CELIX_SUCCESS;
}

bool serviceReference_isAssignableTo(SERVICE_REFERENCE reference, BUNDLE requester, char * serviceName) {
	bool allow = true;

	BUNDLE provider = reference->bundle;
	if (requester == provider) {
		return allow;
	}
//	WIRE providerWire = module_getWire(bundle_getCurrentModule(provider), serviceName);
//	WIRE requesterWire = module_getWire(bundle_getCurrentModule(requester), serviceName);
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

celix_status_t serviceReference_getUsingBundles(SERVICE_REFERENCE reference, apr_pool_t *pool, ARRAY_LIST *bundles) {
	celix_status_t status = CELIX_SUCCESS;

	SERVICE_REGISTRY registry = NULL;
	serviceRegistration_getRegistry(reference->registration, &registry);

	*bundles = serviceRegistry_getUsingBundles(registry, pool, reference);

	return status;
}

celix_status_t serviceReference_equals(SERVICE_REFERENCE reference, SERVICE_REFERENCE compareTo, bool *equal) {
	*equal = (reference->registration == compareTo->registration);
	return CELIX_SUCCESS;
}

int serviceReference_equals2(void *reference1, void *reference2) {
	bool equal;
	serviceReference_equals(reference1, reference2, &equal);
	return equal;
}

unsigned int serviceReference_hashCode(void *referenceP) {
	SERVICE_REFERENCE reference = referenceP;
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

