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
 * dm_service_dependency.c
 *
 *  \date       17 Oct 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include "constants.h"

#include "dm_service_dependency_impl.h"
#include "dm_component_impl.h"
#include "dm_event.h"

static celix_status_t serviceDependency_addedService(void *_ptr, service_reference_pt reference, void *service);
static celix_status_t serviceDependency_modifiedService(void *_ptr, service_reference_pt reference, void *service);
static celix_status_t serviceDependency_removedService(void *_ptr, service_reference_pt reference, void *service);

celix_status_t serviceDependency_create(dm_service_dependency_pt *dependency_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	*dependency_ptr = calloc(1, sizeof(**dependency_ptr));
	if (!*dependency_ptr) {
		status = CELIX_ENOMEM;
	} else {
		(*dependency_ptr)->component = NULL;
		(*dependency_ptr)->available = false;
		(*dependency_ptr)->instanceBound = false;
		(*dependency_ptr)->required = false;

		(*dependency_ptr)->add = NULL;
		(*dependency_ptr)->change = NULL;
		(*dependency_ptr)->remove = NULL;

		(*dependency_ptr)->autoConfigure = false;

		(*dependency_ptr)->isStarted = false;

		(*dependency_ptr)->tracked_service_name = NULL;
		(*dependency_ptr)->tracked_filter_unmodified = NULL;
		(*dependency_ptr)->tracked_filter = NULL;

		(*dependency_ptr)->tracker = NULL;
		(*dependency_ptr)->tracker_customizer = NULL;
	}

	return status;
}

celix_status_t serviceDependency_destroy(dm_service_dependency_pt *dependency_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!*dependency_ptr) {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		free(*dependency_ptr);
		*dependency_ptr = NULL;
	}

	return status;
}

celix_status_t serviceDependency_setRequired(dm_service_dependency_pt dependency, bool required) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->required = required;
	}

	return status;
}

celix_status_t serviceDependency_setService(dm_service_dependency_pt dependency, char *serviceName, char *filter) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if (serviceName != NULL) {
			dependency->tracked_service_name = strdup(serviceName);
		}
		if (filter != NULL) {
			dependency->tracked_filter_unmodified = strdup(filter);
			if (serviceName == NULL) {
				dependency->tracked_filter = strdup(filter);
			} else {
				int len = strlen(serviceName) + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + strlen(filter) + 7;
				char nfilter[len];
				snprintf(nfilter, len, "(&(%s=%s)%s)", OSGI_FRAMEWORK_OBJECTCLASS, serviceName, filter);
				dependency->tracked_filter = strdup(nfilter);
			}
		} else {
			dependency->tracked_filter_unmodified = NULL;
			dependency->tracked_filter = NULL;
		}
	}

	return status;
}

celix_status_t serviceDependency_setCallbacks(dm_service_dependency_pt dependency, service_add_fpt add, service_change_fpt change, service_remove_fpt remove, service_swap_fpt swap) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->add = add;
		dependency->change = change;
		dependency->remove = remove;
		dependency->swap = swap;
	}

	return status;
}

celix_status_t serviceDependency_setAutoConfigure(dm_service_dependency_pt dependency, void **field) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->autoConfigure = field;
	}

	return status;
}

celix_status_t serviceDependency_setComponent(dm_service_dependency_pt dependency, dm_component_pt component) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->component = component;
	}

	return status;
}

celix_status_t serviceDependency_start(dm_service_dependency_pt dependency) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context;

	if (!dependency || !dependency->component || (!dependency->tracked_service_name && !dependency->tracked_filter)) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = component_getBundleContext(dependency->component, &context);
		if (!context) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		dependency->tracker_customizer = NULL;
		status = serviceTrackerCustomizer_create(dependency, NULL, serviceDependency_addedService, serviceDependency_modifiedService,
				serviceDependency_removedService, &dependency->tracker_customizer);
	}

	if (status == CELIX_SUCCESS) {
		if (dependency->tracked_filter) {
			status = serviceTracker_createWithFilter(context, dependency->tracked_filter, dependency->tracker_customizer, &dependency->tracker);
		} else if (dependency->tracked_service_name) {
			status = serviceTracker_create(context, dependency->tracked_service_name, dependency->tracker_customizer, &dependency->tracker);
		}
	}

	if (status == CELIX_SUCCESS) {
		status = serviceTracker_open(dependency->tracker);
	}

	if (status == CELIX_SUCCESS) {
		dependency->isStarted = true;
	}

	return status;
}

celix_status_t serviceDependency_stop(dm_service_dependency_pt dependency) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if (dependency->tracker) {
			status = serviceTracker_close(dependency->tracker);
			status = serviceTracker_destroy(dependency->tracker);
		}
	}

	if (status == CELIX_SUCCESS) {
		dependency->isStarted = false;
	}

	return status;
}

celix_status_t serviceDependency_setInstanceBound(dm_service_dependency_pt dependency, bool instanceBound) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->instanceBound = instanceBound;
	}

	return status;
}

celix_status_t serviceDependency_setAvailable(dm_service_dependency_pt dependency, bool available) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->available = available;
	}

	return status;
}

celix_status_t serviceDependency_invokeAdd(dm_service_dependency_pt dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->add(dependency->component->implementation, event->reference, event->service);
	}

	return status;
}

celix_status_t serviceDependency_invokeChange(dm_service_dependency_pt dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->change(dependency->component->implementation, event->reference, event->service);
	}

	return status;
}

celix_status_t serviceDependency_invokeRemove(dm_service_dependency_pt dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->remove(dependency->component->implementation, event->reference, event->service);
	}

	return status;
}

celix_status_t serviceDependency_invokeSwap(dm_service_dependency_pt dependency, dm_event_pt event, dm_event_pt newEvent) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->swap(dependency->component->implementation, event->reference, event->service, newEvent->reference, newEvent->service);
	}

	return status;
}

celix_status_t serviceDependency_isAvailable(dm_service_dependency_pt dependency, bool *available) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*available = dependency->available;
	}

	return status;
}

celix_status_t serviceDependency_isRequired(dm_service_dependency_pt dependency, bool *required) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*required = dependency->required;
	}

	return status;
}

celix_status_t serviceDependency_isInstanceBound(dm_service_dependency_pt dependency, bool *instanceBound) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*instanceBound = dependency->instanceBound;
	}

	return status;
}

celix_status_t serviceDependency_addedService(void *_ptr, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	dm_event_pt event = NULL;
	dm_service_dependency_pt dependency = _ptr;

	if (!dependency || !reference || !service) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = component_getBundleContext(dependency->component, &context);
		if (!context) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_getBundle(context, &bundle);
		if (!bundle) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = event_create(DM_EVENT_ADDED, bundle, context, reference, service, &event);
	}

	if (status == CELIX_SUCCESS) {
		component_handleEvent(dependency->component, dependency, event);
	}

	return status;
}

celix_status_t serviceDependency_modifiedService(void *_ptr, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	dm_event_pt event = NULL;
	dm_service_dependency_pt dependency = _ptr;

	if (!dependency || !reference || !service) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = component_getBundleContext(dependency->component, &context);
		if (!context) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_getBundle(context, &bundle);
		if (!bundle) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = event_create(DM_EVENT_CHANGED, bundle, context, reference, service, &event);
	}

	if (status == CELIX_SUCCESS) {
		component_handleEvent(dependency->component, dependency, event);
	}

	return status;
}

celix_status_t serviceDependency_removedService(void *_ptr, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	dm_event_pt event = NULL;
	dm_service_dependency_pt dependency = _ptr;

	if (!dependency || !reference || !service) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = component_getBundleContext(dependency->component, &context);
		if (!context) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = bundleContext_getBundle(context, &bundle);
		if (!bundle) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = event_create(DM_EVENT_REMOVED, bundle, context, reference, service, &event);
	}

	if (status == CELIX_SUCCESS) {
		component_handleEvent(dependency->component, dependency, event);
	}

	return status;
}
