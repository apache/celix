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
/**
 * dm_component_impl.c
 *
 *  \date       9 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "celix_constants.h"
#include "filter.h"
#include "dm_component_impl.h"


typedef struct dm_executor_struct * dm_executor_pt;

struct celix_dm_component_struct {
    char id[DM_COMPONENT_MAX_ID_LENGTH];
    char name[DM_COMPONENT_MAX_NAME_LENGTH];
    bundle_context_pt context;
    array_list_pt dm_interfaces;  //protected by mutex

    void* implementation;

    celix_dm_cmp_lifecycle_fpt callbackInit;
    celix_dm_cmp_lifecycle_fpt callbackStart;
    celix_dm_cmp_lifecycle_fpt callbackStop;
    celix_dm_cmp_lifecycle_fpt callbackDeinit;

    array_list_pt dependencies; //protected by mutex
    pthread_mutex_t mutex;

    celix_dm_component_state_t state;
    bool isStarted;
    bool active;

    bool setCLanguageProperty;

    hash_map_pt dependencyEvents; //protected by mutex

    dm_executor_pt executor;
};

typedef struct dm_interface_struct {
    char* serviceName;
    const void* service;
    properties_pt properties;
    service_registration_pt registration;
} dm_interface_t;

struct dm_executor_struct {
    pthread_t runningThread;
    bool runningThreadSet;
    celix_array_list_t *workQueue;
    pthread_mutex_t mutex;
};

typedef struct dm_executor_task_struct {
    celix_dm_component_t *component;
    void (*command)(void *command_ptr, void *data);
    void *data;
} dm_executor_task_t;

typedef struct dm_handle_event_type_struct {
	celix_dm_service_dependency_t *dependency;
	dm_event_pt event;
	dm_event_pt newEvent;
} *dm_handle_event_type_pt;

static celix_status_t executor_runTasks(dm_executor_pt executor, pthread_t  currentThread __attribute__((unused)));
static celix_status_t executor_execute(dm_executor_pt executor);
static celix_status_t executor_executeTask(dm_executor_pt executor, celix_dm_component_t *component, void (*command), void *data);
static celix_status_t executor_schedule(dm_executor_pt executor, celix_dm_component_t *component, void (*command), void *data);
static celix_status_t executor_create(celix_dm_component_t *component __attribute__((unused)), dm_executor_pt *executor);
static void executor_destroy(dm_executor_pt executor);

static celix_status_t component_invokeRemoveRequiredDependencies(celix_dm_component_t *component);
static celix_status_t component_invokeRemoveInstanceBoundDependencies(celix_dm_component_t *component);
static celix_status_t component_invokeRemoveOptionalDependencies(celix_dm_component_t *component);
static celix_status_t component_registerServices(celix_dm_component_t *component);
static celix_status_t component_unregisterServices(celix_dm_component_t *component);
static celix_status_t component_invokeAddOptionalDependencies(celix_dm_component_t *component);
static celix_status_t component_invokeAddRequiredInstanceBoundDependencies(celix_dm_component_t *component);
static celix_status_t component_invokeAddRequiredDependencies(celix_dm_component_t *component);
static celix_status_t component_invokeAutoConfigInstanceBoundDependencies(celix_dm_component_t *component);
static celix_status_t component_invokeAutoConfigDependencies(celix_dm_component_t *component);
static celix_status_t component_configureImplementation(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);
static celix_status_t component_allInstanceBoundAvailable(celix_dm_component_t *component, bool *available);
static celix_status_t component_allRequiredAvailable(celix_dm_component_t *component, bool *available);
static celix_status_t component_performTransition(celix_dm_component_t *component, celix_dm_component_state_t oldState, celix_dm_component_state_t newState, bool *transition);
static celix_status_t component_calculateNewState(celix_dm_component_t *component, celix_dm_component_state_t currentState, celix_dm_component_state_t *newState);
static celix_status_t component_handleChange(celix_dm_component_t *component);
static celix_status_t component_startDependencies(celix_dm_component_t *component __attribute__((unused)), array_list_pt dependencies);
static celix_status_t component_getDependencyEvent(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt *event_pptr);
static celix_status_t component_updateInstance(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event, bool update, bool add);

static celix_status_t component_addTask(celix_dm_component_t *component, celix_dm_service_dependency_t *dep);
static celix_status_t component_startTask(celix_dm_component_t *component, void * data __attribute__((unused)));
static celix_status_t component_stopTask(celix_dm_component_t *component, void * data __attribute__((unused)));
static celix_status_t component_removeTask(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);
static celix_status_t component_handleEventTask(celix_dm_component_t *component, dm_handle_event_type_pt data);

static celix_status_t component_handleAdded(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event);
static celix_status_t component_handleChanged(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event);
static celix_status_t component_handleRemoved(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event);
static celix_status_t component_handleSwapped(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event, dm_event_pt newEvent);

static celix_status_t component_suspend(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);
static celix_status_t component_resume(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency);

celix_dm_component_t* celix_dmComponent_create(bundle_context_t *context, const char* name) {
    celix_dm_component_t *component = calloc(1, sizeof(*component));
    snprintf(component->id, DM_COMPONENT_MAX_ID_LENGTH, "%p", component);
    snprintf(component->name, DM_COMPONENT_MAX_NAME_LENGTH, "%s", name == NULL ? "n/a" : name);

    component->context = context;

    arrayList_create(&component->dm_interfaces);
    arrayList_create(&(component)->dependencies);
    pthread_mutex_init(&(component)->mutex, NULL);

    component->implementation = NULL;

    component->callbackInit = NULL;
    component->callbackStart = NULL;
    component->callbackStop = NULL;
    component->callbackDeinit = NULL;

    component->state = DM_CMP_STATE_INACTIVE;
    component->isStarted = false;
    component->active = false;

    component->setCLanguageProperty = false;

    component->dependencyEvents = hashMap_create(NULL, NULL, NULL, NULL);

    component->executor = NULL;
    executor_create(component, &component->executor);
    return component;
}

celix_status_t component_create(bundle_context_pt context, const char *name, celix_dm_component_t **out) {
    celix_status_t status = CELIX_SUCCESS;
    celix_dm_component_t *cmp = celix_dmComponent_create(context, name);
    if (cmp == NULL) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        *out = cmp;
    }
    return status;
}

void component_destroy(celix_dm_component_t *component) {
    celix_dmComponent_destroy(component);
}

void celix_dmComponent_destroy(celix_dm_component_t *component) {
	if (component) {
		unsigned int i;

		for (i = 0; i < arrayList_size(component->dm_interfaces); i++) {
		    dm_interface_t *interface = arrayList_get(component->dm_interfaces, i);

			if(interface->properties!=NULL){
				properties_destroy(interface->properties);
			}
		    free (interface->serviceName);
            free (interface);
		}
		arrayList_destroy(component->dm_interfaces);

		executor_destroy(component->executor);

		hash_map_iterator_pt iter = hashMapIterator_create(component->dependencyEvents);
		while(hashMapIterator_hasNext(iter)){
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			celix_dm_service_dependency_t *sdep = (celix_dm_service_dependency_t*)hashMapEntry_getKey(entry);
			array_list_pt eventList = (array_list_pt)hashMapEntry_getValue(entry);
			serviceDependency_destroy(&sdep);
			arrayList_destroy(eventList);
		}
		hashMapIterator_destroy(iter);

		hashMap_destroy(component->dependencyEvents, false, false);

		arrayList_destroy(component->dependencies);
		pthread_mutex_destroy(&component->mutex);

		free(component);
	}
}

celix_status_t component_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) {
    return celix_dmComponent_addServiceDependency(component, dep);
}

celix_status_t celix_dmComponent_addServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) {

    celix_status_t status = CELIX_SUCCESS;

	executor_executeTask(component->executor, component, component_addTask, dep);

    return status;
}


static celix_status_t component_addTask(celix_dm_component_t *component, celix_dm_service_dependency_t *dep) {
    celix_status_t status = CELIX_SUCCESS;

    array_list_pt bounds = NULL;
    arrayList_create(&bounds);

    array_list_pt events = NULL;
    arrayList_createWithEquals(event_equals, &events);

    pthread_mutex_lock(&component->mutex);
    hashMap_put(component->dependencyEvents, dep, events);
    arrayList_add(component->dependencies, dep);
    pthread_mutex_unlock(&component->mutex);

    serviceDependency_setComponent(dep, component);

    if (component->state != DM_CMP_STATE_INACTIVE) {
        serviceDependency_setInstanceBound(dep, true);
        arrayList_add(bounds, dep);
    }
    component_startDependencies(component, bounds);
    component_handleChange(component);

    arrayList_destroy(bounds);

    return status;
}

celix_dm_component_state_t component_currentState(celix_dm_component_t *cmp) {
    return celix_dmComponent_currentState(cmp);
}

celix_dm_component_state_t celix_dmComponent_currentState(celix_dm_component_t *cmp) {
    return cmp->state;
}

void* component_getImplementation(celix_dm_component_t *cmp) {
    return celix_dmComponent_getImplementation(cmp);
}

void* celix_dmComponent_getImplementation(celix_dm_component_t *cmp) {
    return cmp->implementation;
}

const char* component_getName(celix_dm_component_t *cmp) {
    return celix_dmComponent_getName(cmp);
}

const char * celix_dmComponent_getName(celix_dm_component_t *cmp) {
    return cmp->name;
}

celix_status_t component_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
    return celix_dmComponent_removeServiceDependency(component, dependency);
}

celix_status_t celix_dmComponent_removeServiceDependency(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
    celix_status_t status = CELIX_SUCCESS;

    executor_executeTask(component->executor, component, component_removeTask, dependency);

    return status;
}

static celix_status_t component_removeTask(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    arrayList_removeElement(component->dependencies, dependency);
    pthread_mutex_unlock(&component->mutex);

    if (component->state != DM_CMP_STATE_INACTIVE) {
        serviceDependency_stop(dependency);
    }

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_remove(component->dependencyEvents, dependency);
    pthread_mutex_unlock(&component->mutex);

	serviceDependency_destroy(&dependency);

    while (!arrayList_isEmpty(events)) {
    	dm_event_pt event = arrayList_remove(events, 0);
    	event_destroy(&event);
    }
    arrayList_destroy(events);

    component_handleChange(component);

    return status;
}

celix_status_t celix_private_dmComponent_start(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    component->active = true;
    executor_executeTask(component->executor, component, component_startTask, NULL);

    return status;
}

celix_status_t component_startTask(celix_dm_component_t *component, void  *data __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;

    component->isStarted = true;
    component_handleChange(component);

    return status;
}

celix_status_t celix_private_dmComponent_stop(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    component->active = false;
    executor_executeTask(component->executor, component, component_stopTask, NULL);

    return status;
}

static celix_status_t component_stopTask(celix_dm_component_t *component, void *data __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;

    component->isStarted = false;
    component_handleChange(component);
    component->active = false;

    return status;
}

celix_status_t component_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp) {
    return celix_dmComponent_setCLanguageProperty(component, setCLangProp);
}

celix_status_t celix_dmComponent_setCLanguageProperty(celix_dm_component_t *component, bool setCLangProp) {
    component->setCLanguageProperty = setCLangProp;
    return CELIX_SUCCESS;
}

celix_status_t component_addInterface(celix_dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, properties_pt properties) {
    return celix_dmComponent_addInterface(component, serviceName, serviceVersion, service, properties);
}

celix_status_t celix_dmComponent_addInterface(celix_dm_component_t *component, const char* serviceName, const char* serviceVersion, const void* service, properties_pt properties) {
    celix_status_t status = CELIX_SUCCESS;

    dm_interface_t *interface = (dm_interface_t *) calloc(1, sizeof(*interface));
    char *name = strdup(serviceName);

    if (properties == NULL) {
        properties = properties_create();
    }

    if ((properties_get(properties, CELIX_FRAMEWORK_SERVICE_VERSION) == NULL) && (serviceVersion != NULL)) {
        properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, serviceVersion);
    }

    if (component->setCLanguageProperty && properties_get(properties, CELIX_FRAMEWORK_SERVICE_LANGUAGE) == NULL) { //always set default lang to C
        properties_set(properties, CELIX_FRAMEWORK_SERVICE_LANGUAGE, CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);
    }

    if (interface && name) {
        interface->serviceName = name;
        interface->service = service;
        interface->properties = properties;
        interface->registration = NULL;
        celixThreadMutex_lock(&component->mutex);
        arrayList_add(component->dm_interfaces, interface);
        celixThreadMutex_unlock(&component->mutex);
        if (component->state == DM_CMP_STATE_TRACKING_OPTIONAL) {
            component_registerServices(component);
        }
    } else {
        free(interface);
        free(name);
        status = CELIX_ENOMEM;
    }

    return status;
}

celix_status_t component_removeInterface(celix_dm_component_t *component, const void* service) {
    return celix_dmComponent_removeInterface(component, service);
}

celix_status_t celix_dmComponent_removeInterface(celix_dm_component_t *component, const void* service) {
    celix_status_t status = CELIX_ILLEGAL_ARGUMENT;

    celixThreadMutex_lock(&component->mutex);
    int nof_interfaces = arrayList_size(component->dm_interfaces);
    for (unsigned int i = 0; i < nof_interfaces; ++i) {
        dm_interface_t *interface = (dm_interface_t *) arrayList_get(component->dm_interfaces, i);
        if (interface->service == service) {
            arrayList_remove(component->dm_interfaces, i);
            if (component->state == DM_CMP_STATE_TRACKING_OPTIONAL) {
                celixThreadMutex_unlock(&component->mutex);
                component_unregisterServices(component);
                component_registerServices(component);
                celixThreadMutex_lock(&component->mutex);
            }
            status = CELIX_SUCCESS;
            break;
        }
    }
    celixThreadMutex_unlock(&component->mutex);

    return status;
}

celix_status_t component_getInterfaces(celix_dm_component_t *component, array_list_pt *out) {
    return celix_dmComponent_getInterfaces(component, out);
}

celix_status_t celix_dmComponent_getInterfaces(celix_dm_component_t *component, array_list_pt *out) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt names = NULL;
    arrayList_create(&names);
    celixThreadMutex_lock(&component->mutex);
    int size = arrayList_size(component->dm_interfaces);
    int i;
    for (i = 0; i < size; i += 1) {
        dm_interface_info_pt info = calloc(1, sizeof(*info));
        dm_interface_t *interface = arrayList_get(component->dm_interfaces, i);
        info->name = strdup(interface->serviceName);
        properties_copy(interface->properties, &info->properties);
        arrayList_add(names, info);
    }
    celixThreadMutex_unlock(&component->mutex);

    if (status == CELIX_SUCCESS) {
        *out = names;
    }

    return status;
}

celix_status_t celix_private_dmComponent_handleEvent(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	dm_handle_event_type_pt data = calloc(1, sizeof(*data));
	data->dependency = dependency;
	data->event = event;
	data->newEvent = NULL;

	status = executor_executeTask(component->executor, component, component_handleEventTask, data);
//	component_handleEventTask(component, data);

	return status;
}

static celix_status_t component_handleEventTask(celix_dm_component_t *component, dm_handle_event_type_pt data) {
	celix_status_t status = CELIX_SUCCESS;

	switch (data->event->event_type) {
		case DM_EVENT_ADDED:
			component_handleAdded(component,data->dependency, data->event);
			break;
		case DM_EVENT_CHANGED:
			component_handleChanged(component,data->dependency, data->event);
			break;
		case DM_EVENT_REMOVED:
			component_handleRemoved(component,data->dependency, data->event);
			break;
		case DM_EVENT_SWAPPED:
			component_handleSwapped(component,data->dependency, data->event, data->newEvent);
			break;
		default:
			break;
	}

	free(data);

	return status;
}

static celix_status_t component_suspend(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
	celix_status_t status = CELIX_SUCCESS;

	dm_service_dependency_strategy_t strategy;
	serviceDependency_getStrategy(dependency, &strategy);
	if (strategy == DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND &&  component->callbackStop != NULL) {
		status = component->callbackStop(component->implementation);
	}

	return status;
}

static celix_status_t component_resume(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
	celix_status_t status = CELIX_SUCCESS;

	dm_service_dependency_strategy_t strategy;
	serviceDependency_getStrategy(dependency, &strategy);
	if (strategy == DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND &&  component->callbackStop != NULL) {
		status = component->callbackStart(component->implementation);
	}

	return status;
}

static celix_status_t component_handleAdded(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    arrayList_add(events, event);
    pthread_mutex_unlock(&component->mutex);

    serviceDependency_setAvailable(dependency, true);

    switch (component->state) {
        case DM_CMP_STATE_WAITING_FOR_REQUIRED: {
            serviceDependency_invokeSet(dependency, event);
            bool required = false;
            serviceDependency_isRequired(dependency, &required);
            if (required) {
                component_handleChange(component);
            }
            break;
        }
        case DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED: {
            bool instanceBound = false;
            serviceDependency_isInstanceBound(dependency, &instanceBound);
            bool required = false;
            serviceDependency_isRequired(dependency, &required);
            if (!instanceBound) {
                if (required) {
                    serviceDependency_invokeSet(dependency, event);
                    serviceDependency_invokeAdd(dependency, event);
                }
                dm_event_pt event = NULL;
                component_getDependencyEvent(component, dependency, &event);
                component_updateInstance(component, dependency, event, false, true);
            }

            if (required) {
                component_handleChange(component);
            }
            break;
        }
        case DM_CMP_STATE_TRACKING_OPTIONAL:
		    component_suspend(component,dependency);
            serviceDependency_invokeSet(dependency, event);
            serviceDependency_invokeAdd(dependency, event);
		    component_resume(component,dependency);
            dm_event_pt event = NULL;
            component_getDependencyEvent(component, dependency, &event);
            component_updateInstance(component, dependency, event, false, true);
            break;
        default:
            break;
    }

    return status;
}

static celix_status_t component_handleChanged(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    int index = arrayList_indexOf(events, event);
    if (index < 0) {
	pthread_mutex_unlock(&component->mutex);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        dm_event_pt old = arrayList_remove(events, (unsigned int) index);
        arrayList_add(events, event);
        pthread_mutex_unlock(&component->mutex);

        serviceDependency_invokeSet(dependency, event);
        switch (component->state) {
            case DM_CMP_STATE_TRACKING_OPTIONAL:
			    component_suspend(component,dependency);
                serviceDependency_invokeChange(dependency, event);
			    component_resume(component,dependency);
                dm_event_pt hevent = NULL;
                component_getDependencyEvent(component, dependency, &hevent);
                component_updateInstance(component, dependency, hevent, true, false);
                break;
            case DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED: {
                bool instanceBound = false;
                serviceDependency_isInstanceBound(dependency, &instanceBound);
                if (!instanceBound) {
                    serviceDependency_invokeChange(dependency, event);
                    dm_event_pt hevent = NULL;
                    component_getDependencyEvent(component, dependency, &hevent);
                    component_updateInstance(component, dependency, hevent, true, false);
                }
                break;
            }
            default:
                break;
        }

        event_destroy(&old);
    }

    return status;
}

static celix_status_t component_handleRemoved(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    int size = arrayList_size(events);
    if (arrayList_contains(events, event)) {
        size--;
    }
    pthread_mutex_unlock(&component->mutex);
    serviceDependency_setAvailable(dependency, size > 0);
    component_handleChange(component);

    pthread_mutex_lock(&component->mutex);
    int index = arrayList_indexOf(events, event);
    if (index < 0) {
	pthread_mutex_unlock(&component->mutex);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        dm_event_pt old = arrayList_remove(events, (unsigned int) index);
        pthread_mutex_unlock(&component->mutex);


        switch (component->state) {
            case DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED: {
                serviceDependency_invokeSet(dependency, event);
                bool instanceBound = false;
                serviceDependency_isInstanceBound(dependency, &instanceBound);
                if (!instanceBound) {
                    bool required = false;
                    serviceDependency_isRequired(dependency, &required);
                    if (required) {
                        serviceDependency_invokeRemove(dependency, event);
                    }
                    dm_event_pt hevent = NULL;
                    component_getDependencyEvent(component, dependency, &hevent);
                    component_updateInstance(component, dependency, hevent, false, false);
                }
                break;
            }
            case DM_CMP_STATE_TRACKING_OPTIONAL:
			    component_suspend(component,dependency);
                serviceDependency_invokeSet(dependency, event);
                serviceDependency_invokeRemove(dependency, event);
			    component_resume(component,dependency);
                dm_event_pt hevent = NULL;
                component_getDependencyEvent(component, dependency, &hevent);
                component_updateInstance(component, dependency, hevent, false, false);
                break;
            default:
                break;
        }

        event_destroy(&event);
        if (old) {
            event_destroy(&old);
        }
    }

    return status;
}

static celix_status_t component_handleSwapped(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event, dm_event_pt newEvent) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    int index = arrayList_indexOf(events, event);
    if (index < 0) {
	pthread_mutex_unlock(&component->mutex);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        dm_event_pt old = arrayList_remove(events, (unsigned int) index);
        arrayList_add(events, newEvent);
        pthread_mutex_unlock(&component->mutex);

        serviceDependency_invokeSet(dependency, event);

        switch (component->state) {
            case DM_CMP_STATE_WAITING_FOR_REQUIRED:
                break;
            case DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED: {
                bool instanceBound = false;
                serviceDependency_isInstanceBound(dependency, &instanceBound);
                if (!instanceBound) {
                    bool required = false;
                    serviceDependency_isRequired(dependency, &required);
                    if (required) {
                        serviceDependency_invokeSwap(dependency, event, newEvent);
                    }
                }
                break;
            }
            case DM_CMP_STATE_TRACKING_OPTIONAL:
			    component_suspend(component,dependency);
                serviceDependency_invokeSwap(dependency, event, newEvent);
			    component_resume(component,dependency);
                break;
            default:
                break;
        }

        event_destroy(&event);
        if (old) {
            event_destroy(&old);
        }
    }

    return status;
}

static celix_status_t component_updateInstance(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt event, bool update, bool add) {
    celix_status_t status = CELIX_SUCCESS;

    bool autoConfig = false;

    serviceDependency_isAutoConfig(dependency, &autoConfig);

    if (autoConfig) {
        const void *service = NULL;
        const void **field = NULL;

        if (event != NULL) {
            event_getService(event, &service);
        }
        serviceDependency_getAutoConfig(dependency, &field);
        serviceDependency_lock(dependency);
        *field = service;
        serviceDependency_unlock(dependency);
    }

    return status;
}

static celix_status_t component_startDependencies(celix_dm_component_t *component __attribute__((unused)), array_list_pt dependencies) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt required_dependencies = NULL;
    arrayList_create(&required_dependencies);

    for (unsigned int i = 0; i < arrayList_size(dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(dependencies, i);
        bool required = false;
        serviceDependency_isRequired(dependency, &required);
        if (required) {
            arrayList_add(required_dependencies, dependency);
            continue;
        }

        serviceDependency_start(dependency);
    }

    for (unsigned int i = 0; i < arrayList_size(required_dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(required_dependencies, i);
        serviceDependency_start(dependency);
    }

    arrayList_destroy(required_dependencies);

    return status;
}

static celix_status_t component_stopDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);
        pthread_mutex_unlock(&component->mutex);
        serviceDependency_stop(dependency);
        pthread_mutex_lock(&component->mutex);
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_handleChange(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    celix_dm_component_state_t oldState;
    celix_dm_component_state_t newState;

    bool transition = false;
    do {
        oldState = component->state;
        status = component_calculateNewState(component, oldState, &newState);
        if (status == CELIX_SUCCESS) {
            component->state = newState;
            status = component_performTransition(component, oldState, newState, &transition);
        }

        if (status != CELIX_SUCCESS) {
            break;
        }
    } while (transition);

    return status;
}

static celix_status_t component_calculateNewState(celix_dm_component_t *component, celix_dm_component_state_t currentState, celix_dm_component_state_t *newState) {
    celix_status_t status = CELIX_SUCCESS;

    if (currentState == DM_CMP_STATE_INACTIVE) {
        if (component->isStarted) {
            *newState = DM_CMP_STATE_WAITING_FOR_REQUIRED;
        } else {
            *newState = currentState;
        }
    } else if (currentState == DM_CMP_STATE_WAITING_FOR_REQUIRED) {
        if (!component->isStarted) {
            *newState = DM_CMP_STATE_INACTIVE;
        } else {
            bool available = false;
            component_allRequiredAvailable(component, &available);

            if (available) {
                *newState = DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED;
            } else {
                *newState = currentState;
            }
        }
    } else if (currentState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED) {
        if (!component->isStarted) {
            *newState = DM_CMP_STATE_WAITING_FOR_REQUIRED;
        } else {
            bool available = false;
            component_allRequiredAvailable(component, &available);

            if (available) {
                bool instanceBoundAvailable = false;
                component_allInstanceBoundAvailable(component, &instanceBoundAvailable);

                if (instanceBoundAvailable) {
                    *newState = DM_CMP_STATE_TRACKING_OPTIONAL;
                } else {
                    *newState = currentState;
                }
            } else {
                *newState = currentState;
            }
        }
    } else if (currentState == DM_CMP_STATE_TRACKING_OPTIONAL) {
        bool instanceBoundAvailable = false;
        bool available = false;

        component_allInstanceBoundAvailable(component, &instanceBoundAvailable);
        component_allRequiredAvailable(component, &available);

        if (component->isStarted && available && instanceBoundAvailable) {
            *newState = currentState;
        } else {
            *newState = DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED;
        }
    } else {
        //should not reach
        *newState = DM_CMP_STATE_INACTIVE;
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

static celix_status_t component_performTransition(celix_dm_component_t *component, celix_dm_component_state_t oldState, celix_dm_component_state_t newState, bool *transition) {
    celix_status_t status = CELIX_SUCCESS;
    //printf("performing transition for %s in thread %i from %i to %i\n", component->name, (int) pthread_self(), oldState, newState);

    if (oldState == newState) {
        *transition = false;
    } else if (oldState == DM_CMP_STATE_INACTIVE && newState == DM_CMP_STATE_WAITING_FOR_REQUIRED) {
        component_startDependencies(component, component->dependencies);
        *transition = true;
    } else if (oldState == DM_CMP_STATE_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED) {
        component_invokeAddRequiredDependencies(component);
        component_invokeAutoConfigDependencies(component);
        if (component->callbackInit) {
        	status = component->callbackInit(component->implementation);
        }
        *transition = true;
    } else if (oldState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_TRACKING_OPTIONAL) {
        component_invokeAddRequiredInstanceBoundDependencies(component);
        component_invokeAutoConfigInstanceBoundDependencies(component);
		component_invokeAddOptionalDependencies(component);
        if (component->callbackStart) {
        	status = component->callbackStart(component->implementation);
        }
        component_registerServices(component);
        *transition = true;
    } else if (oldState == DM_CMP_STATE_TRACKING_OPTIONAL && newState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED) {
        component_unregisterServices(component);
        if (component->callbackStop) {
        	status = component->callbackStop(component->implementation);
        }
		component_invokeRemoveOptionalDependencies(component);
        component_invokeRemoveInstanceBoundDependencies(component);
        *transition = true;
    } else if (oldState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_WAITING_FOR_REQUIRED) {
    	if (component->callbackDeinit) {
    		status = component->callbackDeinit(component->implementation);
    	}
        component_invokeRemoveRequiredDependencies(component);
        *transition = true;
    } else if (oldState == DM_CMP_STATE_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_INACTIVE) {
        component_stopDependencies(component);
        *transition = true;
    }

    return status;
}

static celix_status_t component_allRequiredAvailable(celix_dm_component_t *component, bool *available) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    *available = true;
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);
        bool required = false;
        bool instanceBound = false;

        serviceDependency_isRequired(dependency, &required);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (required && !instanceBound) {
            bool isAvailable = false;
            serviceDependency_isAvailable(dependency, &isAvailable);
            if (!isAvailable) {
                *available = false;
                break;
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_allInstanceBoundAvailable(celix_dm_component_t *component, bool *available) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    *available = true;
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);
        bool required = false;
        bool instanceBound = false;

        serviceDependency_isRequired(dependency, &required);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (required && instanceBound) {
            bool isAvailable = false;
            serviceDependency_isAvailable(dependency, &isAvailable);
            if (!isAvailable) {
                *available = false;
                break;
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeAddRequiredDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool required = false;
        bool instanceBound = false;

        serviceDependency_isRequired(dependency, &required);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (required && !instanceBound) {
            array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
            if (events) {
				for (unsigned int j = 0; j < arrayList_size(events); j++) {
					dm_event_pt event = arrayList_get(events, j);
					serviceDependency_invokeAdd(dependency, event);
				}
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeAutoConfigDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool autoConfig = false;
        bool instanceBound = false;

        serviceDependency_isAutoConfig(dependency, &autoConfig);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (autoConfig && !instanceBound) {
            component_configureImplementation(component, dependency);
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeAutoConfigInstanceBoundDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool autoConfig = false;
        bool instanceBound = false;

        serviceDependency_isAutoConfig(dependency, &autoConfig);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (autoConfig && instanceBound) {
            component_configureImplementation(component, dependency);
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeAddRequiredInstanceBoundDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool required = false;
        bool instanceBound = false;

        serviceDependency_isRequired(dependency, &required);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (instanceBound && required) {
            array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
            if (events) {
				for (unsigned int j = 0; j < arrayList_size(events); j++) {
					dm_event_pt event = arrayList_get(events, j);
					serviceDependency_invokeAdd(dependency, event);
				}
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeAddOptionalDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool required = false;

        serviceDependency_isRequired(dependency, &required);

        if (!required) {
            array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
            if (events) {
				for (unsigned int j = 0; j < arrayList_size(events); j++) {
					dm_event_pt event = arrayList_get(events, j);
					serviceDependency_invokeAdd(dependency, event);
				}
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeRemoveOptionalDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool required = false;

        serviceDependency_isRequired(dependency, &required);

        if (!required) {
            array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
            if (events) {
				for (unsigned int j = 0; j < arrayList_size(events); j++) {
					dm_event_pt event = arrayList_get(events, j);
					serviceDependency_invokeRemove(dependency, event);
				}
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeRemoveInstanceBoundDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool instanceBound = false;

        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (instanceBound) {
            array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
            if (events) {
				for (unsigned int j = 0; j < arrayList_size(events); j++) {
					dm_event_pt event = arrayList_get(events, j);
					serviceDependency_invokeRemove(dependency, event);
				}
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_invokeRemoveRequiredDependencies(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        celix_dm_service_dependency_t *dependency = arrayList_get(component->dependencies, i);

        bool required = false;
        bool instanceBound = false;

        serviceDependency_isRequired(dependency, &required);
        serviceDependency_isInstanceBound(dependency, &instanceBound);

        if (!instanceBound && required) {
            array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
            if (events) {
				for (unsigned int j = 0; j < arrayList_size(events); j++) {
					dm_event_pt event = arrayList_get(events, j);
					serviceDependency_invokeRemove(dependency, event);
				}
            }
        }
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

static celix_status_t component_getDependencyEvent(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency, dm_event_pt *event_pptr) {
    celix_status_t status = CELIX_SUCCESS;

    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    *event_pptr = NULL;

    if (events) {
        for (unsigned int j = 0; j < arrayList_size(events); j++) {
            dm_event_pt event_ptr = arrayList_get(events, j);
            if (*event_pptr != NULL) {
                int compare = 0;
                event_compareTo(event_ptr, *event_pptr, &compare);
                if (compare > 0) {
                    *event_pptr = event_ptr;
                }
            } else {
                *event_pptr = event_ptr;
            }
        }
    }

    return status;
}

static celix_status_t component_configureImplementation(celix_dm_component_t *component, celix_dm_service_dependency_t *dependency) {
    celix_status_t status = CELIX_SUCCESS;

    const void **field = NULL;

    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    if (events) {
        const void *service = NULL;
        dm_event_pt event = NULL;
        component_getDependencyEvent(component, dependency, &event);
        if (event != NULL) {
            event_getService(event, &service);
            serviceDependency_getAutoConfig(dependency, &field);
            serviceDependency_lock(dependency);
            *field = service;
            serviceDependency_unlock(dependency);
        }
    }

    return status;
}

static celix_status_t component_registerServices(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    if (component->context != NULL) {
	    unsigned int i;
        celixThreadMutex_lock(&component->mutex);
        for (i = 0; i < arrayList_size(component->dm_interfaces); i++) {
            dm_interface_t *interface = arrayList_get(component->dm_interfaces, i);
            if (interface->registration == NULL) {
                properties_pt regProps = NULL;
                properties_copy(interface->properties, &regProps);
                bundleContext_registerService(component->context, interface->serviceName, interface->service, regProps,
                                              &interface->registration);
            }
        }
        celixThreadMutex_unlock(&component->mutex);
    }

    return status;
}

static celix_status_t component_unregisterServices(celix_dm_component_t *component) {
    celix_status_t status = CELIX_SUCCESS;

    unsigned int i;

    celixThreadMutex_lock(&component->mutex);
    for (i = 0; i < arrayList_size(component->dm_interfaces); i++) {
	    dm_interface_t *interface = arrayList_get(component->dm_interfaces, i);

	    serviceRegistration_unregister(interface->registration);
	    interface->registration = NULL;
    }
    celixThreadMutex_unlock(&component->mutex);

    return status;
}

celix_status_t component_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit) {
    return celix_dmComponent_setCallbacks(component, init, start, stop, deinit);
}

celix_status_t celix_dmComponent_setCallbacks(celix_dm_component_t *component, celix_dm_cmp_lifecycle_fpt init, celix_dm_cmp_lifecycle_fpt start, celix_dm_cmp_lifecycle_fpt stop, celix_dm_cmp_lifecycle_fpt deinit) {
	if (component->active) {
		return CELIX_ILLEGAL_STATE;
	}
	component->callbackInit = init;
	component->callbackStart = start;
	component->callbackStop = stop;
	component->callbackDeinit = deinit;
	return CELIX_SUCCESS;
}


/*
celix_status_t component_isAvailable(celix_dm_component_t *component, bool *available) {
    *available = component->state == DM_CMP_STATE_TRACKING_OPTIONAL;
    return CELIX_SUCCESS;
}*/

celix_status_t component_setImplementation(celix_dm_component_t *component, void *implementation) {
    return celix_dmComponent_setImplementation(component, implementation);
}

celix_status_t celix_dmComponent_setImplementation(celix_dm_component_t *component, void *implementation) {
    component->implementation = implementation;
    return CELIX_SUCCESS;
}

celix_status_t component_getBundleContext(celix_dm_component_t *component, bundle_context_pt *context) {
    *context = celix_dmComponent_getBundleContext(component);
    return *context == NULL ? CELIX_BUNDLE_EXCEPTION : CELIX_SUCCESS;
}

celix_bundle_context_t* celix_dmComponent_getBundleContext(celix_dm_component_t *component) {
	celix_bundle_context_t *result = NULL;
	if (component) {
		result = component->context;
	}
    return result;
}


static celix_status_t executor_create(celix_dm_component_t *component __attribute__((unused)), dm_executor_pt *executor) {
    celix_status_t status = CELIX_SUCCESS;

    *executor = malloc(sizeof(**executor));
    if (!*executor) {
        status = CELIX_ENOMEM;
    } else {
        (*executor)->workQueue = celix_arrayList_create();
        pthread_mutex_init(&(*executor)->mutex, NULL);
        (*executor)->runningThreadSet = false;
    }

    return status;
}

static void executor_destroy(dm_executor_pt executor) {

	if (executor) {
		pthread_mutex_destroy(&executor->mutex);
		celix_arrayList_destroy(executor->workQueue);

		free(executor);
	}
}

static celix_status_t executor_schedule(dm_executor_pt executor, celix_dm_component_t *component, void (*command), void *data) {
    celix_status_t status = CELIX_SUCCESS;

    dm_executor_task_t *task = NULL;
    task = malloc(sizeof(*task));
    if (!task) {
        status = CELIX_ENOMEM;
    } else {
        task->component = component;
        task->command = command;
        task->data = data;

        pthread_mutex_lock(&executor->mutex);
        celix_arrayList_add(executor->workQueue, task);
        pthread_mutex_unlock(&executor->mutex);
    }

    return status;
}

static celix_status_t executor_executeTask(dm_executor_pt executor, celix_dm_component_t *component, void (*command), void *data) {
    celix_status_t status = CELIX_SUCCESS;

    // Check thread and executor thread, if the same, execute immediately.
//    bool execute = false;
//    pthread_mutex_lock(&executor->mutex);
//    pthread_t currentThread = pthread_self();
//    if (pthread_equal(executor->runningThread, currentThread)) {
//        execute = true;
//    }
//    pthread_mutex_unlock(&executor->mutex);

    // For now, just schedule.
    executor_schedule(executor, component, command, data);
    executor_execute(executor);

    return status;
}

static celix_status_t executor_execute(dm_executor_pt executor) {
    celix_status_t status = CELIX_SUCCESS;
    pthread_t currentThread = pthread_self();

    pthread_mutex_lock(&executor->mutex);
    bool execute = false;
    if (!executor->runningThreadSet) {
        executor->runningThread = currentThread;
        executor->runningThreadSet = true;
        execute = true;
    }
    pthread_mutex_unlock(&executor->mutex);
    if (execute) {
        executor_runTasks(executor, currentThread);
    }

    return status;
}

static celix_status_t executor_runTasks(dm_executor_pt executor, pthread_t currentThread __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;
//    bool execute = false;

    dm_executor_task_t *entry = NULL;

    pthread_mutex_lock(&executor->mutex);
    if (celix_arrayList_size(executor->workQueue) > 0) {
	    entry = celix_arrayList_get(executor->workQueue, 0);
	    celix_arrayList_removeAt(executor->workQueue, 0);
    }
    pthread_mutex_unlock(&executor->mutex);

    while (entry != NULL) {
	    entry->command(entry->component, entry->data);
	    free(entry);

	    pthread_mutex_lock(&executor->mutex);
	    if (celix_arrayList_size(executor->workQueue) > 0) {
		    entry = celix_arrayList_get(executor->workQueue, 0);
		    celix_arrayList_removeAt(executor->workQueue, 0);
	    } else {
		    entry = NULL;
	    }
	    pthread_mutex_unlock(&executor->mutex);
    }


    pthread_mutex_lock(&executor->mutex);
    executor->runningThreadSet = false;
    pthread_mutex_unlock(&executor->mutex);

    return status;
}

celix_status_t component_getComponentInfo(celix_dm_component_t *component, dm_component_info_pt *out) {
    return celix_dmComponent_getComponentInfo(component, out);
}

celix_status_t celix_dmComponent_getComponentInfo(celix_dm_component_t *component, dm_component_info_pt *out) {
    celix_status_t status = CELIX_SUCCESS;
    int i;
    int size;
    dm_component_info_pt info = NULL;
    info = calloc(1, sizeof(*info));

    if (info == NULL) {
        return CELIX_ENOMEM;
    }

    arrayList_create(&info->dependency_list);
    component_getInterfaces(component, &info->interfaces);
    info->active = false;
    memcpy(info->id, component->id, DM_COMPONENT_MAX_ID_LENGTH);
    memcpy(info->name, component->name, DM_COMPONENT_MAX_NAME_LENGTH);

    switch (component->state) {
        case DM_CMP_STATE_INACTIVE :
            info->state = strdup("INACTIVE");
            break;
        case DM_CMP_STATE_WAITING_FOR_REQUIRED :
            info->state = strdup("WAITING_FOR_REQUIRED");
            break;
        case DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED :
            info->state = strdup("INSTANTIATED_AND_WAITING_FOR_REQUIRED");
            break;
        case DM_CMP_STATE_TRACKING_OPTIONAL :
            info->state = strdup("TRACKING_OPTIONAL");
            info->active = true;
            break;
        default :
            info->state = strdup("UNKNOWN");
            break;
    }

    celixThreadMutex_lock(&component->mutex);
    size = arrayList_size(component->dependencies);
    for (i = 0; i < size; i += 1) {
        celix_dm_service_dependency_t *dep = arrayList_get(component->dependencies, i);
        dm_service_dependency_info_pt depInfo = NULL;
        status = serviceDependency_getServiceDependencyInfo(dep, &depInfo);
        if (status == CELIX_SUCCESS) {
            arrayList_add(info->dependency_list, depInfo);
        } else {
            break;
        }
    }
    celixThreadMutex_unlock(&component->mutex);

    if (status == CELIX_SUCCESS) {
        *out = info;
    } else if (info != NULL) {
        component_destroyComponentInfo(info);
    }

    return status;
}
void component_destroyComponentInfo(dm_component_info_pt info) {
    return celix_dmComponent_destroyComponentInfo(info);
}

void celix_dmComponent_destroyComponentInfo(dm_component_info_pt info) {
    int i;
    int size;
    if (info != NULL) {
        free(info->state);

        if (info->interfaces != NULL) {
            size = arrayList_size(info->interfaces);
            for (i = 0; i < size; i += 1) {
                dm_interface_info_pt intfInfo = arrayList_get(info->interfaces, i);
                free(intfInfo->name);
                properties_destroy(intfInfo->properties);
                free(intfInfo);
            }
            arrayList_destroy(info->interfaces);
        }
        if (info->dependency_list != NULL) {
            size = arrayList_size(info->dependency_list);
            for (i = 0; i < size; i += 1) {
                dm_service_dependency_info_pt depInfo = arrayList_get(info->dependency_list, i);
                dependency_destroyDependencyInfo(depInfo);
            }
            arrayList_destroy(info->dependency_list);
        }
    }
    free(info);
}

bool celix_dmComponent_isActive(celix_dm_component_t *component) {
    pthread_mutex_lock(&component->mutex);
    bool active = component->state == DM_CMP_STATE_TRACKING_OPTIONAL;
    pthread_mutex_unlock(&component->mutex);
    return active;
}
