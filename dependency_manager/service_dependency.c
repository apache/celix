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
 *  Created on: Aug 9, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "service_component_private.h"
#include "service_dependency.h"
#include "service_tracker.h"
#include "bundle_context.h"
#include "constants.h"

void * serviceDependency_addingService(void * handle, SERVICE_REFERENCE reference);
void serviceDependency_addedService(void * handle, SERVICE_REFERENCE reference, void * service);
void serviceDependency_modifiedService(void * handle, SERVICE_REFERENCE reference, void * service);
void serviceDependency_removedService(void * handle, SERVICE_REFERENCE reference, void * service);

SERVICE_DEPENDENCY serviceDependency_create(BUNDLE_CONTEXT context) {
	SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) malloc(sizeof(*dependency));
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

void * serviceDependency_getService(SERVICE_DEPENDENCY dependency) {
	void * service = NULL;
	if (dependency->started) {
		service = tracker_getService(dependency->tracker);
	}
	return service;
}

void serviceDependency_start(SERVICE_DEPENDENCY dependency, SERVICE service) {
	dependency->service = service;

	SERVICE_TRACKER_CUSTOMIZER cust = (SERVICE_TRACKER_CUSTOMIZER) malloc(sizeof(*cust));
	cust->handle = dependency;
	cust->addingService = serviceDependency_addingService;
	cust->addedService = serviceDependency_addedService;
	cust->modifiedService = serviceDependency_modifiedService;
	cust->removedService = serviceDependency_removedService;

	dependency->tracker = NULL;

	if (dependency->trackedServiceFilter != NULL) {
		tracker_createWithFilter(dependency->context, dependency->trackedServiceFilter, cust, &dependency->tracker);
	} else if (dependency->trackedServiceName != NULL) {
		tracker_create(dependency->context, dependency->trackedServiceName, cust, &dependency->tracker);
	} else {

	}
	dependency->started = true;
	tracker_open(dependency->tracker);
}

void serviceDependency_stop(SERVICE_DEPENDENCY dependency, SERVICE service) {
	dependency->started = true;
	tracker_close(dependency->tracker);
}

void * serviceDependency_addingService(void * handle, SERVICE_REFERENCE reference) {
	SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) handle;
	void * service = NULL;
	bundleContext_getService(dependency->context, reference, &service);
	dependency->reference = reference;
	dependency->serviceInstance = service;
	return service;
}

void serviceDependency_addedService(void * handle, SERVICE_REFERENCE reference, void * service) {
	SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) handle;
	if (!dependency->available) {
		dependency->available = true;
		serviceComponent_dependencyAvailable(dependency->service, dependency);
	} else {
		serviceComponent_dependencyChanged(dependency->service, dependency);
	}
	if (!dependency->required && dependency->added != NULL) {
		dependency->added(dependency->service->impl, reference, service);
	}
}

void serviceDependency_invokeAdded(SERVICE_DEPENDENCY dependency) {
	if (dependency->added != NULL) {
		dependency->added(dependency->service->impl, dependency->reference, dependency->serviceInstance);
	}
}

void serviceDependency_modifiedService(void * handle, SERVICE_REFERENCE reference, void * service) {
	SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) handle;
	dependency->reference = reference;
	dependency->serviceInstance = service;
	serviceComponent_dependencyChanged(dependency->service, dependency);

	if (dependency->service->registered && dependency->changed != NULL) {
		dependency->changed(dependency->service->impl, reference, service);
	}
}

void serviceDependency_removedService(void * handle, SERVICE_REFERENCE reference, void * service) {
	SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) handle;
	if (dependency->available && tracker_getService(dependency->tracker) == NULL) {
		dependency->available = false;
		serviceComponent_dependencyUnavailable(dependency->service, dependency);
	}

	if (!dependency->required && dependency->removed != NULL) {
		dependency->removed(dependency->service->impl, reference, service);
	}

	bool result;
	bundleContext_ungetService(dependency->context, reference, &result);
}

void serviceDependency_invokeRemoved(SERVICE_DEPENDENCY dependency) {
	if (dependency->removed != NULL) {
		dependency->removed(dependency->service->impl, dependency->reference, dependency->serviceInstance);
	}
}

SERVICE_DEPENDENCY serviceDependency_setService(SERVICE_DEPENDENCY dependency, char * serviceName, char * filter) {
	dependency->trackedServiceName = serviceName;

	if (filter != NULL) {
		if (serviceName != NULL) {
			int len = strlen(serviceName) + strlen(OBJECTCLASS) + strlen(filter) + 7;
			char *nfilter = malloc(sizeof(char) * len);
			strcpy(nfilter, "(&(");
			strcat(nfilter, OBJECTCLASS);
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

SERVICE_DEPENDENCY serviceDependency_setRequired(SERVICE_DEPENDENCY dependency, bool required) {
	dependency->required = required;
	return dependency;
}

SERVICE_DEPENDENCY serviceDependency_setAutoConfigure(SERVICE_DEPENDENCY dependency, void ** field) {
	dependency->autoConfigureField = field;

	return dependency;
}

SERVICE_DEPENDENCY serviceDependency_setCallbacks(SERVICE_DEPENDENCY dependency, void (*added)(void * handle, SERVICE_REFERENCE reference, void *),
		void (*changed)(void * handle, SERVICE_REFERENCE reference, void *),
		void (*removed)(void * handle, SERVICE_REFERENCE reference, void *)) {
	dependency->added = added;
	dependency->changed = changed;
	dependency->removed = removed;
	return dependency;
}
