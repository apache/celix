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
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "constants.h"

#include "dm_service_dependency_impl.h"
#include "dm_component_impl.h"

#define DEFAULT_RANKING     0
#define DM_SERVICE_DEPENDENCY_DEFAULT_STRATEGY DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND

static celix_status_t serviceDependency_addedService(void *_ptr, service_reference_pt reference, void *service);
static celix_status_t serviceDependency_modifiedService(void *_ptr, service_reference_pt reference, void *service);
static celix_status_t serviceDependency_removedService(void *_ptr, service_reference_pt reference, void *service);
static void* serviceDependency_getCallbackHandle(celix_dm_service_dependency_t *dep);

celix_status_t serviceDependency_create(celix_dm_service_dependency_t **dependency_ptr) {
    celix_dm_service_dependency_t *dep = celix_dmServiceDependency_create();
    if (dep != NULL) {
        *dependency_ptr = dep;
        return CELIX_SUCCESS;
    } else {
        return CELIX_BUNDLE_EXCEPTION;
    }
}

celix_dm_service_dependency_t* celix_dmServiceDependency_create() {
	celix_dm_service_dependency_t *dep = calloc(1, sizeof(*dep));
	dep->strategy = DM_SERVICE_DEPENDENCY_DEFAULT_STRATEGY;
    return dep;
}

celix_status_t serviceDependency_destroy(celix_dm_service_dependency_t **dependency_ptr) {
	if (dependency_ptr != NULL) {
		celix_dmServiceDependency_destroy(*dependency_ptr);
	}
	return CELIX_SUCCESS;
}

void celix_dmServiceDependency_destroy(celix_dm_service_dependency_t *dep) {
	if (dep != NULL) {
		free(dep->tracked_service);
		free(dep->tracked_filter);
		free(dep->tracked_filter_unmodified);
		free(dep);
	}
}

celix_status_t serviceDependency_lock(celix_dm_service_dependency_t *dependency) {
	celixThreadMutex_lock(&dependency->lock);
	return CELIX_SUCCESS;
}

celix_status_t serviceDependency_unlock(celix_dm_service_dependency_t *dependency) {
	celixThreadMutex_unlock(&dependency->lock);
	return CELIX_SUCCESS;
}

celix_status_t serviceDependency_setRequired(celix_dm_service_dependency_t *dependency, bool required) {
	return celix_dmServiceDependency_setRequired(dependency, required);
}

celix_status_t celix_dmServiceDependency_setRequired(celix_dm_service_dependency_t *dependency, bool required) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->required = required;
	}

	return status;
}

celix_status_t serviceDependency_setAddCLanguageFilter(celix_dm_service_dependency_t *dependency, bool addCLangFilter) {
	return celix_dmServiceDependency_setAddCLanguageFilter(dependency, addCLangFilter);
}

celix_status_t celix_dmServiceDependency_setAddCLanguageFilter(celix_dm_service_dependency_t *dependency, bool addCLangFilter) {
    dependency->addCLanguageFilter = addCLangFilter;
    return CELIX_SUCCESS;
}

celix_status_t serviceDependency_setStrategy(celix_dm_service_dependency_t *dependency, dm_service_dependency_strategy_t strategy) {
	return celix_dmServiceDependency_setStrategy(dependency, strategy);
}

celix_status_t celix_dmServiceDependency_setStrategy(celix_dm_service_dependency_t *dependency, dm_service_dependency_strategy_t strategy) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		dependency->strategy = strategy;
	}

	return status;
}

celix_status_t serviceDependency_getStrategy(celix_dm_service_dependency_t *dependency, dm_service_dependency_strategy_t* strategy) {
	celix_dm_service_dependency_strategy_t str = celix_dmServiceDependency_getStrategy(dependency);
	if (strategy != NULL) {
		*strategy = str;
	}
	return CELIX_SUCCESS;
}

celix_dm_service_dependency_strategy_t celix_dmServiceDependency_getStrategy(celix_dm_service_dependency_t *dependency) {
	return dependency->strategy;
}

celix_status_t serviceDependency_setService(celix_dm_service_dependency_t *dependency, const char* serviceName, const char* serviceVersionRange, const char* filter) {
	return celix_dmServiceDependency_setService(dependency, serviceName, serviceVersionRange, filter);
}

celix_status_t celix_dmServiceDependency_setService(celix_dm_service_dependency_t *dependency, const char* serviceName, const char* serviceVersionRange, const char* filter) {
	celix_status_t status = CELIX_SUCCESS;
	if (!dependency || !serviceName) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		array_list_pt filterElements = NULL;
		arrayList_create(&filterElements);

		free(dependency->tracked_service);
		dependency->tracked_service = strdup(serviceName);

		if (serviceVersionRange != NULL) {
			version_range_pt versionRange = NULL;

			if (versionRange_parse(serviceVersionRange, &versionRange) == CELIX_SUCCESS) {
				version_pt lowVersion = NULL;
				version_pt highVersion = NULL;

				if ((versionRange_getHighVersion(versionRange, &highVersion) == CELIX_SUCCESS) && (highVersion != NULL)) {
					bool isHighInclusive;
					char* highOperator;
					char* highVersionStr = NULL;

					versionRange_isHighInclusive(versionRange, &isHighInclusive);
					version_toString(highVersion, &highVersionStr);

					highOperator = isHighInclusive ? "<=" : "<";

					if(highVersionStr != NULL){
						size_t len = strlen(CELIX_FRAMEWORK_SERVICE_VERSION) + strlen(highVersionStr) + strlen(highOperator) + 3;
						char serviceVersionFilter[len];
						snprintf(serviceVersionFilter, len, "(%s%s%s)", CELIX_FRAMEWORK_SERVICE_VERSION, highOperator, highVersionStr);
						arrayList_add(filterElements, strdup(serviceVersionFilter));
						free(highVersionStr);
					}
				}

				if ((versionRange_getLowVersion(versionRange, &lowVersion) == CELIX_SUCCESS) && (lowVersion != NULL)) {
					bool isLowInclusive;
					char* lowOperator;
					char* lowVersionStr = NULL;

					versionRange_isLowInclusive(versionRange, &isLowInclusive);
					version_toString(lowVersion, &lowVersionStr);

					lowOperator = isLowInclusive ? ">=" : ">";

					if(lowVersionStr != NULL){
						size_t len = strlen(CELIX_FRAMEWORK_SERVICE_VERSION) + strlen(lowVersionStr) + strlen(lowOperator) + 3;
						char serviceVersionFilter[len];
						snprintf(serviceVersionFilter, len, "(%s%s%s)", CELIX_FRAMEWORK_SERVICE_VERSION, lowOperator, lowVersionStr);
						arrayList_add(filterElements, strdup(serviceVersionFilter));
						free(lowVersionStr);
					}
				}
			}

			if(versionRange!=NULL){
				versionRange_destroy(versionRange);
			}
		}

		if (filter != NULL) {
			free(dependency->tracked_filter_unmodified);
			dependency->tracked_filter_unmodified = strdup(filter);
			arrayList_add(filterElements, strdup(filter));
		}



        bool needLangFilter = true;
		if (filter != NULL) {
            char needle[128];
            snprintf(needle, sizeof(needle), "(%s=", CELIX_FRAMEWORK_SERVICE_LANGUAGE);
            if (strstr(filter, needle) != NULL) {
                needLangFilter = false;
            }
        }

        if (needLangFilter && dependency->addCLanguageFilter) {
			char langFilter[128];
			snprintf(langFilter, sizeof(langFilter), "(%s=%s)", CELIX_FRAMEWORK_SERVICE_LANGUAGE, CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);
            arrayList_add(filterElements, strdup(langFilter));
		}

		if (arrayList_size(filterElements) > 0) {
			array_list_iterator_pt filterElementsIter = arrayListIterator_create(filterElements);

			size_t len = strlen(serviceName) + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + 4;
			free(dependency->tracked_filter);
			dependency->tracked_filter = calloc(len, sizeof(*dependency->tracked_filter));
			snprintf(dependency->tracked_filter, len, "(%s=%s)", OSGI_FRAMEWORK_OBJECTCLASS, serviceName);

			while (arrayListIterator_hasNext(filterElementsIter) == true) {
				char* filterElement = (char*) arrayListIterator_next(filterElementsIter);
				size_t len = strnlen(dependency->tracked_filter, 1024*1024) + strnlen(filterElement, 1024*1024) + 4;
				char* newFilter = calloc(len, sizeof(*newFilter));

				if (dependency->tracked_filter[0] == '(' && dependency->tracked_filter[1] == '&') {
					//already have an & (AND) can combine with additional filter -> easier to read
					size_t orgLen = strnlen(dependency->tracked_filter, 1024*1024);
					snprintf(newFilter, len, "%.*s%s)", (int)orgLen -1, dependency->tracked_filter, filterElement);
				} else {
					snprintf(newFilter, len, "(&%s%s)", dependency->tracked_filter, filterElement);
				}

				free(dependency->tracked_filter);
				free(filterElement);

				dependency->tracked_filter = newFilter;
			}

			arrayListIterator_destroy(filterElementsIter);
		}
		else {
			free(dependency->tracked_filter);
			dependency->tracked_filter = NULL;
		}

		arrayList_destroy(filterElements);
	}

	return status;
}
celix_status_t serviceDependency_getFilter(celix_dm_service_dependency_t *dependency, const char** filter) {
	const char *f =  celix_dmServiceDependency_getFilter(dependency);
	if (filter != NULL) {
		*filter = f;
	}
	return CELIX_SUCCESS;
}

const char* celix_dmServiceDependency_getFilter(celix_dm_service_dependency_t *dependency) {
	return (const char*)dependency->tracked_filter;
}

celix_status_t serviceDependency_setCallbacks(celix_dm_service_dependency_t *dependency, service_set_fpt set, service_add_fpt add, service_change_fpt change, service_remove_fpt remove, service_swap_fpt swap) {
	celix_status_t status = CELIX_SUCCESS;

    //printf("Setting callbacks set %p, add %p, change %p, remove %p and swap %p\n", set, add, change, remove, swap);

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->set = (celix_dm_service_update_fp)set;
		dependency->add = (celix_dm_service_update_fp)add;
		dependency->change = (celix_dm_service_update_fp)change;
		dependency->remove = (celix_dm_service_update_fp)remove;
		dependency->swap = (celix_dm_service_swap_fp)swap;
	}

	return status;
}

celix_status_t serviceDependency_setCallbacksWithServiceReference(celix_dm_service_dependency_t *dependency, service_set_with_ref_fpt set, service_add_with_ref_fpt add, service_change_with_ref_fpt change, service_remove_with_ref_fpt remove,
		service_swap_with_ref_fpt swap) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->set_with_ref = set;
		dependency->add_with_ref = add;
		dependency->change_with_ref = change;
		dependency->remove_with_ref = remove;
		dependency->swap_with_ref = swap;
	}

	return status;
}

celix_status_t celix_dmServiceDependency_setCallback(celix_dm_service_dependency_t *dependency, celix_dm_service_update_fp set) {
	dependency->set = set;
	return CELIX_SUCCESS;
}


celix_status_t celix_dmServiceDependency_setCallbackWithProperties(celix_dm_service_dependency_t *dependency, celix_dm_service_update_with_props_fp set) {
	dependency->set_with_props = set;
	return CELIX_SUCCESS;
}


celix_status_t celix_dmServiceDependency_setCallbacksWithOptions(celix_dm_service_dependency_t *dependency, const celix_dm_service_dependency_callback_options_t *opts) {
	dependency->set = opts->set;
	dependency->add = opts->add;
	dependency->remove = opts->remove;
	dependency->swap = opts->swap;

	dependency->set_with_props = opts->setWithProps;
	dependency->add_with_props = opts->addWithProps;
	dependency->rem_with_props = opts->removeWithProps;
	dependency->swap_with_props = opts->swapWithProps;
	return CELIX_SUCCESS;
}

celix_status_t serviceDependency_setAutoConfigure(celix_dm_service_dependency_t *dependency, celix_thread_mutex_t *service_lock, const void **field) {
	celix_status_t status = CELIX_SUCCESS;

	celix_thread_mutex_t lock;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->autoConfigure = field;
		celixThreadMutex_create(&lock, NULL);
		*service_lock = lock;
		dependency->lock = lock;
	}

	return status;
}

celix_status_t serviceDependency_setComponent(celix_dm_service_dependency_t *dependency, celix_dm_component_t *component) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->component = component;
	}

	return status;
}

celix_status_t serviceDependency_start(celix_dm_service_dependency_t *dependency) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;

	if (!dependency || !dependency->component || (!dependency->tracked_service && !dependency->tracked_filter)) {
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
		status = serviceTrackerCustomizer_create(dependency, NULL, serviceDependency_addedService, serviceDependency_modifiedService, serviceDependency_removedService, &dependency->tracker_customizer);
	}
	if (status == CELIX_SUCCESS) {
		if (dependency->tracked_filter) {
			status = serviceTracker_createWithFilter(context, dependency->tracked_filter, dependency->tracker_customizer, &dependency->tracker);
		} else if (dependency->tracked_service) {
			status = serviceTracker_create(context, dependency->tracked_service, dependency->tracker_customizer, &dependency->tracker);
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

celix_status_t serviceDependency_stop(celix_dm_service_dependency_t *dependency) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->isStarted = false;
	}

	if (status == CELIX_SUCCESS && dependency->tracker) {
		status = serviceTracker_close(dependency->tracker);
		if (status == CELIX_SUCCESS) {
			serviceTracker_destroy(dependency->tracker);
			dependency->tracker = NULL;
		}
	}

	return status;
}

celix_status_t serviceDependency_setInstanceBound(celix_dm_service_dependency_t *dependency, bool instanceBound) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->instanceBound = instanceBound;
	}

	return status;
}

celix_status_t serviceDependency_setAvailable(celix_dm_service_dependency_t *dependency, bool available) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		dependency->available = available;
	}

	return status;
}

celix_status_t serviceDependency_invokeSet(celix_dm_service_dependency_t *dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	assert(dependency->isStarted == true);
	array_list_pt serviceReferences = NULL;
	int i;
	int curRanking = INT_MIN;
	service_reference_pt curServRef = NULL;
	void *service = NULL;

	serviceReferences = serviceTracker_getServiceReferences(dependency->tracker);

	/* Find the service with the higest ranking */
	for (i = 0; i < arrayList_size(serviceReferences); i++) {
		service_reference_pt serviceReference = arrayList_get(serviceReferences, i);
		const char* ranking_value;
		int ranking = 0;

		status = serviceReference_getProperty(serviceReference, ((char *) OSGI_FRAMEWORK_SERVICE_RANKING), &ranking_value);

		if (status == CELIX_SUCCESS) {
			if (ranking_value == NULL) {
				ranking = DEFAULT_RANKING;
			} else {
				char *end;
				ranking = strtol(ranking_value, &end, 10);
				if (end == ranking_value) {
					ranking = DEFAULT_RANKING;
				}
			}

			if (ranking > curRanking) {
				curRanking = ranking;
				curServRef = serviceReference;
			}
		} else {
			break;
		}

	}

	arrayList_destroy(serviceReferences);

	if (status == CELIX_SUCCESS) {
		if (curServRef) {
			status = bundleContext_getService(event->context, curServRef, &service);
		} else {
			service = NULL;
		}

		if (dependency->set) {
			dependency->set(serviceDependency_getCallbackHandle(dependency), service);
		}
		if (dependency->set_with_ref) {
			dependency->set_with_ref(serviceDependency_getCallbackHandle(dependency), curServRef, service);
		}
		if (dependency->set_with_props) {
			service_registration_pt reg = NULL;
			celix_properties_t *props = NULL;
			serviceReference_getServiceRegistration(curServRef, &reg);
			if (reg != NULL) {
				serviceRegistration_getProperties(reg, &props);
			}
			dependency->set_with_props(serviceDependency_getCallbackHandle(dependency), service, props);
		}

		if (curServRef) {
			bundleContext_ungetService(event->context, curServRef, NULL);
		}
	}

	return status;
}

celix_status_t serviceDependency_invokeAdd(celix_dm_service_dependency_t *dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if (dependency->add) {
			dependency->add(serviceDependency_getCallbackHandle(dependency), (void*)event->service);
		}
		if (dependency->add_with_ref) {
			dependency->add_with_ref(serviceDependency_getCallbackHandle(dependency), event->reference, (void*)event->service);
		}
		if (dependency->add_with_props) {
			service_registration_pt reg = NULL;
			celix_properties_t *props = NULL;
			serviceReference_getServiceRegistration(event->reference, &reg);
			if (reg != NULL) {
				serviceRegistration_getProperties(reg, &props);
			}
			dependency->add_with_props(serviceDependency_getCallbackHandle(dependency), (void*)event->service, props);
		}
	}

	return status;
}

celix_status_t serviceDependency_invokeChange(celix_dm_service_dependency_t *dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if (dependency->change) {
			dependency->change(serviceDependency_getCallbackHandle(dependency), (void*)event->service);
		}
		if (dependency->change_with_ref) {
			dependency->change_with_ref(serviceDependency_getCallbackHandle(dependency), event->reference, event->service);
		}
	}

	return status;
}

celix_status_t serviceDependency_invokeRemove(celix_dm_service_dependency_t *dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if (dependency->remove) {
			dependency->remove(serviceDependency_getCallbackHandle(dependency), (void*)event->service);
		}
		if (dependency->remove_with_ref) {
			dependency->remove_with_ref(serviceDependency_getCallbackHandle(dependency), event->reference, event->service);
		}
		if (dependency->rem_with_props) {
			service_registration_pt reg = NULL;
			celix_properties_t *props = NULL;
			serviceReference_getServiceRegistration(event->reference, &reg);
			if (reg != NULL) {
				serviceRegistration_getProperties(reg, &props);
			}
			dependency->rem_with_props(serviceDependency_getCallbackHandle(dependency), (void*)event->service, props);
		}
	}

	return status;
}

celix_status_t serviceDependency_invokeSwap(celix_dm_service_dependency_t *dependency, dm_event_pt event, dm_event_pt newEvent) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if (dependency->swap) {
			dependency->swap(serviceDependency_getCallbackHandle(dependency), (void*)event->service, (void*)newEvent->service);
		}
		if (dependency->swap_with_ref) {
			dependency->swap_with_ref(serviceDependency_getCallbackHandle(dependency), event->reference, event->service, newEvent->reference, newEvent->service);
		}
		if (dependency->swap_with_props) {
			service_registration_pt reg = NULL;
			celix_properties_t *props = NULL;
			serviceReference_getServiceRegistration(newEvent->reference, &reg);
			if (reg != NULL) {
				serviceRegistration_getProperties(reg, &props);
			}
			dependency->swap_with_props(serviceDependency_getCallbackHandle(dependency),(void*) event->service, (void*)newEvent->service, props);
		}
	}

	return status;
}

celix_status_t serviceDependency_isAvailable(celix_dm_service_dependency_t *dependency, bool *available) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*available = dependency->available;
	}

	return status;
}

celix_status_t serviceDependency_isRequired(celix_dm_service_dependency_t *dependency, bool *required) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*required = dependency->required;
	}

	return status;
}

celix_status_t serviceDependency_isInstanceBound(celix_dm_service_dependency_t *dependency, bool *instanceBound) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*instanceBound = dependency->instanceBound;
	}

	return status;
}

celix_status_t serviceDependency_isAutoConfig(celix_dm_service_dependency_t *dependency, bool *autoConfig) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*autoConfig = dependency->autoConfigure != NULL;
	}

	return status;
}

celix_status_t serviceDependency_getAutoConfig(celix_dm_service_dependency_t *dependency, const void*** autoConfigure) {
	celix_status_t status = CELIX_SUCCESS;

	if (!dependency) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*autoConfigure = dependency->autoConfigure;
	}

	return status;
}

static celix_status_t serviceDependency_addedService(void *_ptr, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	dm_event_pt event = NULL;
	celix_dm_service_dependency_t *dependency = _ptr;

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
		celix_private_dmComponent_handleEvent(dependency->component, dependency, event);
	}

	return status;
}

static celix_status_t serviceDependency_modifiedService(void *_ptr, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	dm_event_pt event = NULL;
	celix_dm_service_dependency_t *dependency = _ptr;

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
		celix_private_dmComponent_handleEvent(dependency->component, dependency, event);
	}

	return status;
}

static celix_status_t serviceDependency_removedService(void *_ptr, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;
	bundle_pt bundle = NULL;
	dm_event_pt event = NULL;
	celix_dm_service_dependency_t *dependency = _ptr;

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
		celix_private_dmComponent_handleEvent(dependency->component, dependency, event);
	}

	return status;
}

celix_status_t serviceDependency_getServiceDependencyInfo(celix_dm_service_dependency_t *dep, dm_service_dependency_info_t **out) {
	if (out != NULL) {
		*out = celix_dmServiceDependency_createInfo(dep);
	}
	return CELIX_SUCCESS;
}

dm_service_dependency_info_t* celix_dmServiceDependency_createInfo(celix_dm_service_dependency_t* dep) {
	celix_dm_service_dependency_info_t *info = calloc(1, sizeof(*info));
	if (info != NULL) {
		celixThreadMutex_lock(&dep->lock);
		info->available = dep->available;
		info->filter = dep->tracked_filter != NULL ? strdup(dep->tracked_filter) : NULL;
		if (info->filter == NULL) {
			info->filter = dep->tracked_service != NULL ? strdup(dep->tracked_service) : NULL;
		}
		info->required = dep->required;

		array_list_pt refs = serviceTracker_getServiceReferences(dep->tracker);
		if (refs != NULL) {
			info->count = arrayList_size(refs);
		}
		arrayList_destroy(refs);

		celixThreadMutex_unlock(&dep->lock);
	}
	return info;
}


void dependency_destroyDependencyInfo(dm_service_dependency_info_pt info) {
	celix_dmServiceDependency_destroyInfo(NULL, info);
}

void celix_dmServiceDependency_destroyInfo(celix_dm_service_dependency_t *dep __attribute__((unused)), dm_service_dependency_info_t *info) {
	if (info != NULL) {
		free(info->filter);
	}
	free(info);
}

celix_status_t serviceDependency_setCallbackHandle(celix_dm_service_dependency_t *dependency, void* handle) {
	return celix_dmServiceDependency_setCallbackHandle(dependency, handle);
}

celix_status_t celix_dmServiceDependency_setCallbackHandle(celix_dm_service_dependency_t *dependency, void* handle) {
	dependency->callbackHandle = handle;
    return CELIX_SUCCESS;
}

static void* serviceDependency_getCallbackHandle(celix_dm_service_dependency_t *dependency) {
    return dependency->callbackHandle == NULL ? component_getImplementation(dependency->component) : dependency->callbackHandle;
}
