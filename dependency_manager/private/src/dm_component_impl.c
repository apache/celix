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
 * dm_component_impl.c
 *
 *  \date       9 Oct 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "constants.h"
#include "filter.h"
#include "dm_component_impl.h"
#include "../../../framework/private/include/framework_private.h"

struct dm_executor {
    pthread_t runningThread;
    bool runningThreadSet;
    linked_list_pt workQueue;

    pthread_mutex_t mutex;
};

struct dm_executor_task {
    dm_component_pt component;
    void (*command)(void *command_ptr, void *data);
    void *data;
};

struct dm_handle_event_type {
	dm_service_dependency_pt dependency;
	dm_event_pt event;
	dm_event_pt newEvent;
};

typedef struct dm_handle_event_type *dm_handle_event_type_pt;

static celix_status_t executor_runTasks(dm_executor_pt executor, pthread_t  currentThread __attribute__((unused)));
static celix_status_t executor_execute(dm_executor_pt executor);
static celix_status_t executor_executeTask(dm_executor_pt executor, dm_component_pt component, void (*command), void *data);
static celix_status_t executor_schedule(dm_executor_pt executor, dm_component_pt component, void (*command), void *data);
static celix_status_t executor_create(dm_component_pt component __attribute__((unused)), dm_executor_pt *executor);
static celix_status_t executor_destroy(dm_executor_pt *executor);

static celix_status_t component_destroyComponent(dm_component_pt component);
static celix_status_t component_invokeRemoveRequiredDependencies(dm_component_pt component);
static celix_status_t component_invokeRemoveInstanceBoundDependencies(dm_component_pt component);
static celix_status_t component_invokeRemoveOptionalDependencies(dm_component_pt component);
static celix_status_t component_registerService(dm_component_pt component);
static celix_status_t component_unregisterService(dm_component_pt component);
static celix_status_t component_invokeAddOptionalDependencies(dm_component_pt component);
static celix_status_t component_invokeAddRequiredInstanceBoundDependencies(dm_component_pt component);
static celix_status_t component_invokeAddRequiredDependencies(dm_component_pt component);
static celix_status_t component_invokeAutoConfigInstanceBoundDependencies(dm_component_pt component);
static celix_status_t component_invokeAutoConfigDependencies(dm_component_pt component);
static celix_status_t component_configureImplementation(dm_component_pt component, dm_service_dependency_pt dependency);
static celix_status_t component_allInstanceBoundAvailable(dm_component_pt component, bool *available);
static celix_status_t component_allRequiredAvailable(dm_component_pt component, bool *available);
static celix_status_t component_performTransition(dm_component_pt component, dm_component_state_pt oldState, dm_component_state_pt newState, bool *transition);
static celix_status_t component_calculateNewState(dm_component_pt component, dm_component_state_pt currentState, dm_component_state_pt *newState);
static celix_status_t component_handleChange(dm_component_pt component);
static celix_status_t component_startDependencies(dm_component_pt component __attribute__((unused)), array_list_pt dependencies);
static celix_status_t component_getDependencyEvent(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt *event_pptr);
static celix_status_t component_updateInstance(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event, bool update, bool add);

static celix_status_t component_addTask(dm_component_pt component, array_list_pt dependencies);
static celix_status_t component_startTask(dm_component_pt component, void * data __attribute__((unused)));
static celix_status_t component_stopTask(dm_component_pt component, void * data __attribute__((unused)));
static celix_status_t component_removeTask(dm_component_pt component, dm_service_dependency_pt dependency);
static celix_status_t component_handleEventTask(dm_component_pt component, dm_handle_event_type_pt data);

static celix_status_t component_handleAdded(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event);
static celix_status_t component_handleChanged(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event);
static celix_status_t component_handleRemoved(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event);
static celix_status_t component_handleSwapped(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event, dm_event_pt newEvent);

celix_status_t component_create(bundle_context_pt context, dm_dependency_manager_pt manager, dm_component_pt *component) {
    celix_status_t status = CELIX_SUCCESS;

    *component = malloc(sizeof(**component));
    if (!*component) {
        status = CELIX_ENOMEM;
    } else {
        (*component)->context = context;
        (*component)->manager = manager;

	arrayList_create(&((*component)->dm_interface));

        (*component)->implementation = NULL;

        (*component)->callbackInit = NULL;
        (*component)->callbackStart = NULL;
        (*component)->callbackStop = NULL;
        (*component)->callbackDeinit = NULL;


        arrayList_create(&(*component)->dependencies);
        pthread_mutex_init(&(*component)->mutex, NULL);

        (*component)->state = DM_CMP_STATE_INACTIVE;
        (*component)->isStarted = false;
        (*component)->active = false;

        (*component)->dependencyEvents = hashMap_create(NULL, NULL, NULL, NULL);

        (*component)->executor = NULL;
        executor_create(*component, &(*component)->executor);
    }

    return status;
}

celix_status_t component_destroy(dm_component_pt *component_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!*component_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		unsigned int i;

		for (i = 0; i < arrayList_size((*component_ptr)->dm_interface); i++) {
		    dm_interface *interface = arrayList_get((*component_ptr)->dm_interface, i);

		    free (interface->serviceName);
		}
		arrayList_destroy((*component_ptr)->dm_interface);

		// #TODO destroy dependencies?
		executor_destroy(&(*component_ptr)->executor);
		hashMap_destroy((*component_ptr)->dependencyEvents, false, false);
		pthread_mutex_destroy(&(*component_ptr)->mutex);
		arrayList_destroy((*component_ptr)->dependencies);

		free(*component_ptr);
		*component_ptr = NULL;
	}

	return status;
}

celix_status_t component_addServiceDependency(dm_component_pt component, ...) {
    celix_status_t status = CELIX_SUCCESS;

    array_list_pt dependenciesList = NULL;
    arrayList_create(&dependenciesList);

    va_list dependencies;
    va_start(dependencies, component);
    dm_service_dependency_pt dependency = va_arg(dependencies, dm_service_dependency_pt);
    while (dependency != NULL) {
        arrayList_add(dependenciesList, dependency);


        dependency = va_arg(dependencies, dm_service_dependency_pt);
    }

    va_end(dependencies);

	executor_executeTask(component->executor, component, component_addTask, dependenciesList);
//    component_addTask(component, dependenciesList);

    return status;
}


celix_status_t component_addTask(dm_component_pt component, array_list_pt dependencies) {
    celix_status_t status = CELIX_SUCCESS;

    array_list_pt bounds = NULL;
    arrayList_create(&bounds);
    for (unsigned int i = 0; i < arrayList_size(dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(dependencies, i);

        pthread_mutex_lock(&component->mutex);
        array_list_pt events = NULL;
        arrayList_createWithEquals(event_equals, &events);
        hashMap_put(component->dependencyEvents, dependency, events);

        arrayList_add(component->dependencies, dependency);
        pthread_mutex_unlock(&component->mutex);

        serviceDependency_setComponent(dependency, component);
        if (component->state != DM_CMP_STATE_INACTIVE) {
            serviceDependency_setInstanceBound(dependency, true);
            arrayList_add(bounds, dependency);
        }
        component_startDependencies(component, bounds);
        component_handleChange(component);
    }

    arrayList_destroy(bounds);
    arrayList_destroy(dependencies);

    return status;
}

celix_status_t component_removeServiceDependency(dm_component_pt component, dm_service_dependency_pt dependency) {
    celix_status_t status = CELIX_SUCCESS;

    executor_executeTask(component->executor, component, component_removeTask, dependency);
//    component_removeTask(component, dependency);

    return status;
}

celix_status_t component_removeTask(dm_component_pt component, dm_service_dependency_pt dependency) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    arrayList_removeElement(component->dependencies, dependency);
    pthread_mutex_unlock(&component->mutex);

    if (component->state != DM_CMP_STATE_INACTIVE) {
        serviceDependency_stop(dependency);
    }

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_remove(component->dependencyEvents, dependency);
    for (unsigned int i = arrayList_size(events); i > 0; i--) {
    	dm_event_pt event = arrayList_remove(events, i - 1);
    	event_destroy(&event);
    }
    arrayList_destroy(events);
    pthread_mutex_unlock(&component->mutex);

    component_handleChange(component);

    return status;
}

celix_status_t component_start(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    component->active = true;
    executor_executeTask(component->executor, component, component_startTask, NULL);
//    component_startTask(component, NULL);

    return status;
}

celix_status_t component_startTask(dm_component_pt component, void  *data __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;

    component->isStarted = true;
    component_handleChange(component);

    return status;
}

celix_status_t component_stop(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    component->active = false;
    executor_executeTask(component->executor, component, component_stopTask, NULL);
//    component_stopTask(component, NULL);

    return status;
}

celix_status_t component_stopTask(dm_component_pt component, void *data __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;

    component->isStarted = false;
    component_handleChange(component);
    component->active = false;

    return status;
}

celix_status_t component_addInterface(dm_component_pt component, char *serviceName, void *service, properties_pt properties) {
    celix_status_t status = CELIX_SUCCESS;

    if (component->active) {
        return CELIX_ILLEGAL_STATE;
    } else {
	dm_interface *interface = (dm_interface *) malloc (sizeof (dm_interface));
	char *name = strdup (serviceName);

	if (interface && name) {
            interface->serviceName = name;
            interface->service = service;
            interface->properties = properties;
            interface->registration = NULL;
	    arrayList_add(component->dm_interface, interface);
	}
	else {
	   free (interface);
	   free (name);
	   status = CELIX_ENOMEM;
	}
    }

    return status;
}

celix_status_t component_handleEvent(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	dm_handle_event_type_pt data = calloc(1, sizeof(*data));
	data->dependency = dependency;
	data->event = event;
	data->newEvent = NULL;

	status = executor_executeTask(component->executor, component, component_handleEventTask, data);
//	component_handleEventTask(component, data);

	return status;
}

celix_status_t component_handleEventTask(dm_component_pt component, dm_handle_event_type_pt data) {
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

celix_status_t component_handleAdded(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    arrayList_add(events, event);
    pthread_mutex_unlock(&component->mutex);

    serviceDependency_setAvailable(dependency, true);

    serviceDependency_invokeSet(dependency, event);
    switch (component->state) {
        case DM_CMP_STATE_WAITING_FOR_REQUIRED: {
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
            if (!instanceBound) {
                bool required = false;
                serviceDependency_isRequired(dependency, &required);
                if (required) {
                    serviceDependency_invokeAdd(dependency, event);
                }
                dm_event_pt event = NULL;
                component_getDependencyEvent(component, dependency, &event);
                component_updateInstance(component, dependency, event, false, true);
            } else {
                bool required = false;
                serviceDependency_isRequired(dependency, &required);
                if (required) {
                    component_handleChange(component);
                }
            }
            break;
        }
        case DM_CMP_STATE_TRACKING_OPTIONAL:
            serviceDependency_invokeAdd(dependency, event);
            dm_event_pt event = NULL;
            component_getDependencyEvent(component, dependency, &event);
            component_updateInstance(component, dependency, event, false, true);
            break;
        default:
            break;
    }

    return status;
}

celix_status_t component_handleChanged(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    int index = arrayList_indexOf(events, event);
    if (index < 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        dm_event_pt old = arrayList_remove(events, (unsigned int) index);
        arrayList_add(events, event);
        pthread_mutex_unlock(&component->mutex);

        serviceDependency_invokeSet(dependency, event);
        switch (component->state) {
            case DM_CMP_STATE_TRACKING_OPTIONAL:
                serviceDependency_invokeChange(dependency, event);
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

celix_status_t component_handleRemoved(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event) {
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
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        dm_event_pt old = arrayList_remove(events, (unsigned int) index);
        pthread_mutex_unlock(&component->mutex);

        serviceDependency_invokeSet(dependency, event);

        switch (component->state) {
            case DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED: {
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
                serviceDependency_invokeRemove(dependency, event);
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

celix_status_t component_handleSwapped(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event, dm_event_pt newEvent) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    int index = arrayList_indexOf(events, event);
    if (index < 0) {
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
                serviceDependency_invokeSwap(dependency, event, newEvent);
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

celix_status_t component_updateInstance(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt event, bool update, bool add) {
    celix_status_t status = CELIX_SUCCESS;

    bool autoConfig = false;

    serviceDependency_isAutoConfig(dependency, &autoConfig);

    if (autoConfig) {
        void *service = NULL;
        void **field = NULL;

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

celix_status_t component_startDependencies(dm_component_pt component __attribute__((unused)), array_list_pt dependencies) {
    celix_status_t status = CELIX_SUCCESS;
    array_list_pt required_dependencies = NULL;
    arrayList_create(&required_dependencies);

    for (unsigned int i = 0; i < arrayList_size(dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(dependencies, i);
        bool required = false;
        serviceDependency_isRequired(dependency, &required);
        if (required) {
            arrayList_add(required_dependencies, dependency);
            continue;
        }

        serviceDependency_start(dependency);
    }

    for (unsigned int i = 0; i < arrayList_size(required_dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(required_dependencies, i);
        serviceDependency_start(dependency);
    }

    arrayList_destroy(required_dependencies);

    return status;
}

celix_status_t component_stopDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);
        pthread_mutex_unlock(&component->mutex);
        serviceDependency_stop(dependency);
        pthread_mutex_lock(&component->mutex);
    }
    pthread_mutex_unlock(&component->mutex);

    return status;
}

celix_status_t component_handleChange(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    dm_component_state_pt oldState;
    dm_component_state_pt newState;

    bool cont = false;
    do {
        oldState = component->state;
        component_calculateNewState(component, oldState, &newState);
        component->state = newState;
        component_performTransition(component, oldState, newState, &cont);
    } while (cont);

    return status;
}

celix_status_t component_calculateNewState(dm_component_pt component, dm_component_state_pt currentState, dm_component_state_pt *newState) {
    celix_status_t status = CELIX_SUCCESS;

    if (currentState == DM_CMP_STATE_INACTIVE) {
        if (component->isStarted) {
            *newState = DM_CMP_STATE_WAITING_FOR_REQUIRED;
            return status;
        }
    }
    if (currentState == DM_CMP_STATE_WAITING_FOR_REQUIRED) {
        if (!component->isStarted) {
            *newState = DM_CMP_STATE_INACTIVE;
            return status;
        }

        bool available = false;
        component_allRequiredAvailable(component, &available);

        if (available) {
            *newState = DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED;
            return status;
        }
    }
    if (currentState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED) {
        bool available = false;
        component_allRequiredAvailable(component, &available);

        if (component->isStarted && available) {
            bool instanceBoundAvailable = false;
            component_allInstanceBoundAvailable(component, &instanceBoundAvailable);

            if (instanceBoundAvailable) {
                *newState = DM_CMP_STATE_TRACKING_OPTIONAL;
                return status;
            }

            *newState = currentState;
            return status;
        }
        *newState = DM_CMP_STATE_WAITING_FOR_REQUIRED;
        return status;
    }
    if (currentState == DM_CMP_STATE_TRACKING_OPTIONAL) {
        bool instanceBoundAvailable = false;
        bool available = false;

        component_allInstanceBoundAvailable(component, &instanceBoundAvailable);
        component_allRequiredAvailable(component, &available);

        if (component->isStarted  && available && instanceBoundAvailable) {
            *newState = currentState;
            return status;
        }

        *newState = DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED;
        return status;
    }

    *newState = currentState;

    return status;
}

celix_status_t component_performTransition(dm_component_pt component, dm_component_state_pt oldState, dm_component_state_pt newState, bool *transition) {
    celix_status_t status = CELIX_SUCCESS;

    if (oldState == DM_CMP_STATE_INACTIVE && newState == DM_CMP_STATE_WAITING_FOR_REQUIRED) {
        component_startDependencies(component, component->dependencies);
//        #TODO Add listener support
//        notifyListeners(newState);
        *transition = true;
        return status;
    }

    if (oldState == DM_CMP_STATE_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED) {
        // #TODO Remove
//        component_instantiateComponent(component);
        component_invokeAddRequiredDependencies(component);
        component_invokeAutoConfigDependencies(component);
        dm_component_state_pt stateBeforeCallingInit = component->state;
        if (component->callbackInit) {
        	component->callbackInit(component->implementation);
        }
        if (stateBeforeCallingInit == component->state) {
//            #TODO Add listener support
//            notifyListeners(newState); // init did not change current state, we can notify about this new state
        }
        *transition = true;
        return status;
    }

    if (oldState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_TRACKING_OPTIONAL) {
        component_invokeAddRequiredInstanceBoundDependencies(component);
        component_invokeAutoConfigInstanceBoundDependencies(component);
        if (component->callbackStart) {
        	component->callbackStart(component->implementation);
        }
        component_invokeAddOptionalDependencies(component);
        component_registerService(component);
//            #TODO Add listener support
//        notifyListeners(newState);
        return true;
    }

    if (oldState == DM_CMP_STATE_TRACKING_OPTIONAL && newState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED) {
        component_unregisterService(component);
        component_invokeRemoveOptionalDependencies(component);
        if (component->callbackStop) {
        	component->callbackStop(component->implementation);
        }
        component_invokeRemoveInstanceBoundDependencies(component);
//            #TODO Add listener support
//        notifyListeners(newState);
        *transition = true;
        return status;
    }

    if (oldState == DM_CMP_STATE_INSTANTIATED_AND_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_WAITING_FOR_REQUIRED) {
    	if (component->callbackDeinit) {
    		component->callbackDeinit(component->implementation);
    	}
        component_invokeRemoveRequiredDependencies(component);
//            #TODO Add listener support
//        notifyListeners(newState);
//        bool needInstance = false;
//        component_someDependenciesNeedInstance(component, &needInstance);
//        if (!needInstance) {
            component_destroyComponent(component);
//        }
        *transition = true;
        return status;
    }

    if (oldState == DM_CMP_STATE_WAITING_FOR_REQUIRED && newState == DM_CMP_STATE_INACTIVE) {
        component_stopDependencies(component);
        component_destroyComponent(component);
//            #TODO Add listener support
//        notifyListeners(newState);
        *transition = true;
        return status;
    }

    *transition = false;
    return status;
}

celix_status_t component_allRequiredAvailable(dm_component_pt component, bool *available) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    *available = true;
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);
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

celix_status_t component_allInstanceBoundAvailable(dm_component_pt component, bool *available) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    *available = true;
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);
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

celix_status_t component_invokeAddRequiredDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeAutoConfigDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeAutoConfigInstanceBoundDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeAddRequiredInstanceBoundDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeAddOptionalDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeRemoveOptionalDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeRemoveInstanceBoundDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_invokeRemoveRequiredDependencies(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    pthread_mutex_lock(&component->mutex);
    for (unsigned int i = 0; i < arrayList_size(component->dependencies); i++) {
        dm_service_dependency_pt dependency = arrayList_get(component->dependencies, i);

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

celix_status_t component_getDependencyEvent(dm_component_pt component, dm_service_dependency_pt dependency, dm_event_pt *event_pptr) {
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

celix_status_t component_configureImplementation(dm_component_pt component, dm_service_dependency_pt dependency) {
    celix_status_t status = CELIX_SUCCESS;

    void **field = NULL;

    array_list_pt events = hashMap_get(component->dependencyEvents, dependency);
    if (events) {
        void *service = NULL;
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

celix_status_t component_destroyComponent(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

//    component->implementation = NULL;

    return status;
}

celix_status_t component_registerService(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    if (component->context) {
	unsigned int i;

	for (i = 0; i < arrayList_size(component->dm_interface); i++) {
	    dm_interface *interface = arrayList_get(component->dm_interface, i);

            bundleContext_registerService(component->context, interface->serviceName, interface->service, interface->properties, &interface->registration);
	}
    }

    return status;
}

celix_status_t component_unregisterService(dm_component_pt component) {
    celix_status_t status = CELIX_SUCCESS;

    unsigned int i;

    for (i = 0; i < arrayList_size(component->dm_interface); i++) {
	dm_interface *interface = arrayList_get(component->dm_interface, i);

	serviceRegistration_unregister(interface->registration);
	interface->registration = NULL;
    }

    return status;
}

celix_status_t component_setCallbacks(dm_component_pt component, init_fpt init, start_fpt start, stop_fpt stop, deinit_fpt deinit) {
	if (component->active) {
		return CELIX_ILLEGAL_STATE;
	}
	component->callbackInit = init;
	component->callbackStart = start;
	component->callbackStop = stop;
	component->callbackDeinit = deinit;
	return CELIX_SUCCESS;
}

celix_status_t component_isAvailable(dm_component_pt component, bool *available) {
    *available = component->state == DM_CMP_STATE_TRACKING_OPTIONAL;
    return CELIX_SUCCESS;
}

celix_status_t component_setImplementation(dm_component_pt component, void *implementation) {
    component->implementation = implementation;
    return CELIX_SUCCESS;
}

celix_status_t component_getBundleContext(dm_component_pt component, bundle_context_pt *context) {
	celix_status_t status = CELIX_SUCCESS;

	if (!component) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*context = component->context;
	}

	return status;
}


celix_status_t executor_create(dm_component_pt component __attribute__((unused)), dm_executor_pt *executor) {
    celix_status_t status = CELIX_SUCCESS;

    *executor = malloc(sizeof(**executor));
    if (!*executor) {
        status = CELIX_ENOMEM;
    } else {
        linkedList_create(&(*executor)->workQueue);
        pthread_mutex_init(&(*executor)->mutex, NULL);
        (*executor)->runningThreadSet = false;
    }

    return status;
}

celix_status_t executor_destroy(dm_executor_pt *executor) {
	celix_status_t status = CELIX_SUCCESS;

	if (!*executor) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		pthread_mutex_destroy(&(*executor)->mutex);
		linkedList_destroy((*executor)->workQueue);

		free(*executor);
		*executor = NULL;
	}

	return status;
}

celix_status_t executor_schedule(dm_executor_pt executor, dm_component_pt component, void (*command), void *data) {
    celix_status_t status = CELIX_SUCCESS;

    struct dm_executor_task *task = NULL;
    task = malloc(sizeof(*task));
    if (!task) {
        status = CELIX_ENOMEM;
    } else {
        task->component = component;
        task->command = command;
        task->data = data;

        pthread_mutex_lock(&executor->mutex);
        linkedList_addLast(executor->workQueue, task);
        pthread_mutex_unlock(&executor->mutex);
    }

    return status;
}

celix_status_t executor_executeTask(dm_executor_pt executor, dm_component_pt component, void (*command), void *data) {
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

celix_status_t executor_execute(dm_executor_pt executor) {
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

celix_status_t executor_runTasks(dm_executor_pt executor, pthread_t currentThread __attribute__((unused))) {
    celix_status_t status = CELIX_SUCCESS;
//    bool execute = false;

    do {
        struct dm_executor_task *entry = NULL;
        pthread_mutex_lock(&executor->mutex);
        while ((entry = linkedList_removeFirst(executor->workQueue)) != NULL) {
            pthread_mutex_unlock(&executor->mutex);

            entry->command(entry->component, entry->data);

            pthread_mutex_lock(&executor->mutex);

            free(entry);
        }
        executor->runningThreadSet = false;
        pthread_mutex_unlock(&executor->mutex);

//        pthread_mutex_lock(&executor->mutex);
//        if (executor->runningThread == NULL) {
//            executor->runningThread = currentThread;
//            execute = true;
//        }
//        pthread_mutex_unlock(&executor->mutex);
    } while (!linkedList_isEmpty(executor->workQueue)); // && execute

    return status;
}
