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
 * service_dependency.c
 *
 *  \date       Aug 9, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "service_component_private.h"
#include "service_dependency.h"
#include "service_tracker.h"
#include "bundle_context.h"
#include "constants.h"

celix_status_t serviceDependency_addingService(void * handle, service_reference_pt reference, void **service);
celix_status_t serviceDependency_addedService(void * handle, service_reference_pt reference, void * service);
celix_status_t serviceDependency_modifiedService(void * handle, service_reference_pt reference, void * service);
celix_status_t serviceDependency_removedService(void * handle, service_reference_pt reference, void * service);

service_dependency_pt serviceDependency_create(bundle_context_pt context) {
	service_dependency_pt dependency = (service_dependency_pt) malloc(sizeof(*dependency));
	dependency->context = context;

	dependency->autoConfigureField = NULL;
	dependency->tracker = NULL;
	dependency->trackedServiceName = NULL;
	dependency->trackedServiceFilter = NULL;
	dependency->service = NULL;
	dependency->serviceInstance = NULL;
	dependency->reference = NULL;
	dependency->interface = NULL;

	dependency->available = false;
	dependency->started = false;
	dependency->required = false;

	dependency->added = NULL;
	dependency->changed = NULL;
	dependency->removed = NULL;

	return dependency;
}

void * serviceDependency_getService(service_dependency_pt dependency) {
	void * service = NULL;
	if (dependency->started) {
		service = serviceTracker_getService(dependency->tracker);
	}
	return service;
}

void serviceDependency_start(service_dependency_pt dependency, service_pt service) {
	apr_pool_t *pool;
	service_tracker_customizer_pt cust = NULL;

	dependency->service = service;
	dependency->tracker = NULL;

	bundleContext_getMemoryPool(dependency->context, &pool);

	serviceTrackerCustomizer_create(pool, dependency, serviceDependency_addingService,
			serviceDependency_addedService, serviceDependency_modifiedService,
			serviceDependency_removedService, &cust);

	if (dependency->trackedServiceFilter != NULL) {
		serviceTracker_createWithFilter(pool, dependency->context, dependency->trackedServiceFilter, cust, &dependency->tracker);
	} else if (dependency->trackedServiceName != NULL) {
		serviceTracker_create(pool, dependency->context, dependency->trackedServiceName, cust, &dependency->tracker);
	} else {

	}
	dependency->started = true;
	serviceTracker_open(dependency->tracker);
}

void serviceDependency_stop(service_dependency_pt dependency, service_pt service) {
	dependency->started = true;
	serviceTracker_close(dependency->tracker);
}

celix_status_t serviceDependency_addingService(void * handle, service_reference_pt reference, void **service) {
	service_dependency_pt dependency = (service_dependency_pt) handle;
	bundleContext_getService(dependency->context, reference, service);
	dependency->reference = reference;
	dependency->serviceInstance = *service;
	return CELIX_SUCCESS;
}

celix_status_t serviceDependency_addedService(void * handle, service_reference_pt reference, void * service) {
	service_dependency_pt dependency = (service_dependency_pt) handle;
	if (!dependency->available) {
		dependency->available = true;
		serviceComponent_dependencyAvailable(dependency->service, dependency);
	} else {
		serviceComponent_dependencyChanged(dependency->service, dependency);
	}
	if (!dependency->required && dependency->added != NULL) {
		dependency->added(dependency->service->impl, reference, service);
	}
	return CELIX_SUCCESS;
}

void serviceDependency_invokeAdded(service_dependency_pt dependency) {
	if (dependency->added != NULL) {
		dependency->added(dependency->service->impl, dependency->reference, dependency->serviceInstance);
	}
}

celix_status_t serviceDependency_modifiedService(void * handle, service_reference_pt reference, void * service) {
	service_dependency_pt dependency = (service_dependency_pt) handle;
	dependency->reference = reference;
	dependency->serviceInstance = service;
	serviceComponent_dependencyChanged(dependency->service, dependency);

	if (dependency->service->registered && dependency->changed != NULL) {
		dependency->changed(dependency->service->impl, reference, service);
	}
	return CELIX_SUCCESS;
}

celix_status_t serviceDependency_removedService(void * handle, service_reference_pt reference, void * service) {
	bool result;
	service_dependency_pt dependency = (service_dependency_pt) handle;
	if (dependency->available && serviceTracker_getService(dependency->tracker) == NULL) {
		dependency->available = false;
		serviceComponent_dependencyUnavailable(dependency->service, dependency);
	}

	if (!dependency->required && dependency->removed != NULL) {
		dependency->removed(dependency->service->impl, reference, service);
	}

	bundleContext_ungetService(dependency->context, reference, &result);
	return CELIX_SUCCESS;
}

void serviceDependency_invokeRemoved(service_dependency_pt dependency) {
	if (dependency->removed != NULL) {
		dependency->removed(dependency->service->impl, dependency->reference, dependency->serviceInstance);
	}
}

service_dependency_pt serviceDependency_setService(service_dependency_pt dependency, char * serviceName, char * filter) {
	dependency->trackedServiceName = serviceName;

	if (filter != NULL) {
		if (serviceName != NULL) {
			int len = strlen(serviceName) + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + strlen(filter) + 7;
			char *nfilter = malloc(sizeof(char) * len);
			strcpy(nfilter, "(&(");
			strcat(nfilter, OSGI_FRAMEWORK_OBJECTCLASS);
			strcat(nfilter, "=");
			strcat(nfilter, serviceName);
			strcat(nfilter, ")");
			strcat(nfilter, filter);
			strcat(nfilter, ")\0");
			dependency->trackedServiceFilter = nfilter;
		} else {
			dependency->trackedServiceFilter = filter;
		}
	}

	return dependency;
}

service_dependency_pt serviceDependency_setRequired(service_dependency_pt dependency, bool required) {
	dependency->required = required;
	return dependency;
}

service_dependency_pt serviceDependency_setAutoConfigure(service_dependency_pt dependency, void ** field) {
	dependency->autoConfigureField = field;

	return dependency;
}

service_dependency_pt serviceDependency_setCallbacks(service_dependency_pt dependency, void (*added)(void * handle, service_reference_pt reference, void *),
		void (*changed)(void * handle, service_reference_pt reference, void *),
		void (*removed)(void * handle, service_reference_pt reference, void *)) {
	dependency->added = added;
	dependency->changed = changed;
	dependency->removed = removed;
	return dependency;
}
