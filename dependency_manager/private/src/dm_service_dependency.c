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
#define DM_SERVICE_DEPENDENCY_DEFAULT_STRATEGY DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING

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
        (*dependency_ptr)->strategy = DM_SERVICE_DEPENDENCY_DEFAULT_STRATEGY;

        (*dependency_ptr)->set = NULL;
        (*dependency_ptr)->add = NULL;
        (*dependency_ptr)->change = NULL;
        (*dependency_ptr)->remove = NULL;
        (*dependency_ptr)->swap = NULL;

        (*dependency_ptr)->add_with_ref = NULL;
        (*dependency_ptr)->change_with_ref = NULL;
        (*dependency_ptr)->remove_with_ref = NULL;
        (*dependency_ptr)->swap_with_ref = NULL;

        (*dependency_ptr)->autoConfigure = NULL;

        (*dependency_ptr)->isStarted = false;

        (*dependency_ptr)->tracked_service = NULL;
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
        free((*dependency_ptr)->tracked_service);
        free((*dependency_ptr)->tracked_filter);
        free((*dependency_ptr)->tracked_filter_unmodified);
        free(*dependency_ptr);
        *dependency_ptr = NULL;
    }

    return status;
}

celix_status_t serviceDependency_lock(dm_service_dependency_pt dependency) {
    celixThreadMutex_lock(&dependency->lock);
    return CELIX_SUCCESS;
}

celix_status_t serviceDependency_unlock(dm_service_dependency_pt dependency) {
    celixThreadMutex_unlock(&dependency->lock);
    return CELIX_SUCCESS;
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

celix_status_t serviceDependency_setStrategy(dm_service_dependency_pt dependency, dm_service_dependency_strategy_t strategy) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        dependency->strategy = strategy;
    }

    return status;
}

celix_status_t serviceDependency_getStrategy(dm_service_dependency_pt dependency, dm_service_dependency_strategy_t* strategy) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *strategy = dependency->strategy;
    }

    return status;

}

celix_status_t serviceDependency_setService(dm_service_dependency_pt dependency, char *serviceName, char *serviceVersionRange, char *filter) {
    celix_status_t status = CELIX_SUCCESS;
    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        array_list_pt filterElements = NULL;
        arrayList_create(&filterElements);

        if (serviceName != NULL) {
            dependency->tracked_service = strdup(serviceName);
        }

        if (serviceVersionRange != NULL) {
            version_range_pt versionRange;

            if (versionRange_parse(serviceVersionRange, &versionRange) == CELIX_SUCCESS) {
                version_pt lowVersion = NULL;
                version_pt highVersion = NULL;

                if ((versionRange_getHighVersion(versionRange, &highVersion) == CELIX_SUCCESS) && (highVersion != NULL)) {
                    bool isHighInclusive;
                    char* highOperator;
                    char* highVersionStr;

                    versionRange_isHighInclusive(versionRange, &isHighInclusive);
                    version_toString(highVersion, &highVersionStr);

                    highOperator = isHighInclusive ? "<=" : "<";

                    size_t len = strlen(CELIX_FRAMEWORK_SERVICE_VERSION) + strlen(highVersionStr) + strlen(highOperator) + 3;
                    char serviceVersionFilter[len];
                    snprintf(serviceVersionFilter, len, "(%s%s%s)", CELIX_FRAMEWORK_SERVICE_VERSION, highOperator, highVersionStr);
                    arrayList_add(filterElements, strdup(serviceVersionFilter));
                }

                if ((versionRange_getLowVersion(versionRange, &lowVersion) == CELIX_SUCCESS) && (lowVersion != NULL)) {
                    bool isLowInclusive;
                    char* lowOperator;
                    char* lowVersionStr;

                    versionRange_isLowInclusive(versionRange, &isLowInclusive);
                    version_toString(lowVersion, &lowVersionStr);

                    lowOperator = isLowInclusive ? ">=" : ">";

                    size_t len = strlen(CELIX_FRAMEWORK_SERVICE_VERSION) + strlen(lowVersionStr) + strlen(lowOperator) + 3;
                    char serviceVersionFilter[len];
                    snprintf(serviceVersionFilter, len, "(%s%s%s)", CELIX_FRAMEWORK_SERVICE_VERSION, lowOperator, lowVersionStr);
                    arrayList_add(filterElements, strdup(serviceVersionFilter));
                }
            }
        }

        if (filter != NULL) {
            dependency->tracked_filter_unmodified = strdup(filter);
            arrayList_add(filterElements, strdup(filter));
        }

        if (arrayList_size(filterElements) > 0) {
            array_list_iterator_pt filterElementsIter = arrayListIterator_create(filterElements);

            size_t len = strlen(serviceName) + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + 4;
            dependency->tracked_filter = calloc(len, sizeof(*dependency->tracked_filter));
            snprintf(dependency->tracked_filter, len, "(%s=%s)", OSGI_FRAMEWORK_OBJECTCLASS, serviceName);

            while (arrayListIterator_hasNext(filterElementsIter) == true) {
                char* filterElement = (char*) arrayListIterator_next(filterElementsIter);
                size_t len = strlen(dependency->tracked_filter) + strlen(filterElement) + 4;
                char* newFilter = calloc(len, sizeof(*newFilter));

                snprintf(newFilter, len, "(&%s%s)", dependency->tracked_filter, filterElement);

                free(dependency->tracked_filter);
                free(filterElement);

                dependency->tracked_filter = newFilter;
            }

            arrayListIterator_destroy(filterElementsIter);
        }
        else {
            dependency->tracked_filter = NULL;
        }

        arrayList_destroy(filterElements);
    }

    return status;
}

celix_status_t serviceDependency_getFilter(dm_service_dependency_pt dependency, char **filter) {
    *filter = dependency->tracked_filter;
    return CELIX_SUCCESS;
}

celix_status_t serviceDependency_setCallbacks(dm_service_dependency_pt dependency, service_set_fpt set, service_add_fpt add, service_change_fpt change, service_remove_fpt remove, service_swap_fpt swap) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        dependency->set = set;
        dependency->add = add;
        dependency->change = change;
        dependency->remove = remove;
        dependency->swap = swap;
    }

    return status;
}

celix_status_t serviceDependency_setCallbacksWithServiceReference(dm_service_dependency_pt dependency, service_set_with_ref_fpt set, service_add_with_ref_fpt add, service_change_with_ref_fpt change, service_remove_with_ref_fpt remove,
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

celix_status_t serviceDependency_setAutoConfigure(dm_service_dependency_pt dependency, celix_thread_mutex_t *service_lock, void **field) {
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

celix_status_t serviceDependency_stop(dm_service_dependency_pt dependency) {
    celix_status_t status = CELIX_SUCCESS;
    celix_status_t tmp_status;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        dependency->isStarted = false;
    }

    if (status == CELIX_SUCCESS) {
        if (dependency->tracker) {
            tmp_status = serviceTracker_close(dependency->tracker);
            if (tmp_status != CELIX_SUCCESS) {
                status = tmp_status;
            }
            tmp_status = serviceTracker_destroy(dependency->tracker);
            if (tmp_status != CELIX_SUCCESS && status == CELIX_SUCCESS) {
                status = tmp_status;
            }
        }
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

celix_status_t serviceDependency_invokeSet(dm_service_dependency_pt dependency, dm_event_pt event) {
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
        char *ranking_value;
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
            return status;
        }
    }

    arrayList_destroy(serviceReferences);

    if (curServRef) {
        status = bundleContext_getService(event->context, curServRef, &service);
    } else {
        service = NULL;
    }

    if (dependency->set) {
        dependency->set(component_getImplementation(dependency->component), service);
    }
    if (dependency->set_with_ref) {
        dependency->set_with_ref(component_getImplementation(dependency->component), curServRef, service);
    }

    return status;
}

celix_status_t serviceDependency_invokeAdd(dm_service_dependency_pt dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        if (dependency->add) {
            dependency->add(component_getImplementation(dependency->component), event->service);
        }
        if (dependency->add_with_ref) {
            dependency->add_with_ref(component_getImplementation(dependency->component), event->reference, event->service);
        }
    }

    return status;
}

celix_status_t serviceDependency_invokeChange(dm_service_dependency_pt dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        if (dependency->change) {
            dependency->change(component_getImplementation(dependency->component), event->service);
        }
        if (dependency->change_with_ref) {
            dependency->change_with_ref(component_getImplementation(dependency->component), event->reference, event->service);
        }
    }

    return status;
}

celix_status_t serviceDependency_invokeRemove(dm_service_dependency_pt dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        if (dependency->remove) {
            dependency->remove(component_getImplementation(dependency->component), event->service);
        }
        if (dependency->remove_with_ref) {
            dependency->remove_with_ref(component_getImplementation(dependency->component), event->reference, event->service);
        }
    }

    return status;
}

celix_status_t serviceDependency_invokeSwap(dm_service_dependency_pt dependency, dm_event_pt event, dm_event_pt newEvent) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        if (dependency->swap) {
            dependency->swap(component_getImplementation(dependency->component), event->service, newEvent->service);
        }
        if (dependency->swap_with_ref) {
            dependency->swap_with_ref(component_getImplementation(dependency->component), event->reference, event->service, newEvent->reference, newEvent->service);
        }
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

celix_status_t serviceDependency_isAutoConfig(dm_service_dependency_pt dependency, bool *autoConfig) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        *autoConfig = dependency->autoConfigure != NULL;
    }

    return status;
}

celix_status_t serviceDependency_getAutoConfig(dm_service_dependency_pt dependency, void ***autoConfigure) {
    celix_status_t status = CELIX_SUCCESS;

    if (!dependency) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        *autoConfigure = dependency->autoConfigure;
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

celix_status_t serviceDependency_getServiceDependencyInfo(dm_service_dependency_pt dep, dm_service_dependency_info_pt *out) {
    celix_status_t status = CELIX_SUCCESS;
    dm_service_dependency_info_pt info = calloc(1, sizeof(*info));
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
    } else {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        *out = info;
    }

    return status;
}

void dependency_destroyDependencyInfo(dm_service_dependency_info_pt info) {
    if (info != NULL) {
        free(info->filter);
    }
    free(info);
}
