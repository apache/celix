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
 * service_component.c
 *
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "dependency_manager.h"
#include "service_component.h"
#include "service_component_private.h"
#include "service.h"
#include "bundle_context.h"
#include "bundle.h"
#include "array_list.h"
#include "service_registration.h"

typedef enum state_state {
	STATE_INACTIVE = 1,
	STATE_WAITING_FOR_REQUIRED,
	STATE_TRACKING_OPTIONAL,
} STATE_STATE;

struct state {
	array_list_pt dependencies;
	STATE_STATE state;
};

struct executorEntry {
	service_pt service;
	void (*function)(void *, void*);
	void * argument;
};

struct executor {
	struct executorEntry * active;
	linked_list_pt workQueue;

	apr_thread_mutex_t *mutex;
};

service_pt serviceComponent_create(bundle_context_pt context, dependency_manager_pt manager) {
    service_pt service;
    apr_pool_t *pool;
	apr_pool_t *mypool;

	bundleContext_getMemoryPool(context, &pool);
	apr_pool_create(&mypool, pool);

	if (mypool) {
        service = (service_pt) apr_pcalloc(mypool, sizeof(*service));
        service->pool = mypool;
        service->impl = NULL;
        service->serviceName = NULL;
        service->serviceRegistration = NULL;
        service->dependencies = NULL;
        arrayList_create(&service->dependencies);

        service->init = service_init;
        service->start= service_start;
        service->stop = service_stop;
        service->destroy = service_destroy;

        service->context = context;
        service->manager = manager;
        service->state = state_create(arrayList_clone(service->dependencies), false);
        service->executor = executor_create(mypool);

		apr_thread_mutex_create(&service->mutex, APR_THREAD_MUTEX_UNNESTED, mypool);
	}

	return service;
}

void serviceComponent_calculateStateChanges(service_pt service, const state_pt old, const state_pt new) {
	if (state_isWaitingForRequired(old) && state_isTrackingOptional(new)) {
		executor_enqueue(service->executor, service, serviceComponent_activateService, new);
	}
	if (state_isTrackingOptional(old) && state_isWaitingForRequired(new)) {
		executor_enqueue(service->executor, service, serviceComponent_deactivateService, old);
	}
	if (state_isInactive(old) && state_isTrackingOptional(new)) {
		executor_enqueue(service->executor, service, serviceComponent_activateService, new);
	}
	if (state_isInactive(old) && state_isWaitingForRequired(new)) {
		executor_enqueue(service->executor, service, serviceComponent_startTrackingRequired, new);
	}
	if (state_isWaitingForRequired(old) && state_isInactive(new)) {
		executor_enqueue(service->executor, service, serviceComponent_stopTrackingRequired, old);
	}
	if (state_isTrackingOptional(old) && state_isInactive(new)) {
		executor_enqueue(service->executor, service, serviceComponent_deactivateService, old);
		executor_enqueue(service->executor, service, serviceComponent_stopTrackingRequired, old);
	}
	executor_execute(service->executor);
}

service_pt serviceComponent_addServiceDependency(service_pt service, service_dependency_pt dependency) {
	state_pt old, new;
	apr_thread_mutex_lock(service->mutex);
	old = service->state;
	arrayList_add(service->dependencies, dependency);
	apr_thread_mutex_unlock(service->mutex);

	if (state_isTrackingOptional(old) || ( state_isWaitingForRequired(old) && dependency->required)) {
		serviceDependency_start(dependency, service);
	}

	apr_thread_mutex_lock(service->mutex);
	new = state_create(arrayList_clone(service->dependencies), !state_isInactive(old));
	service->state = new;
	apr_thread_mutex_unlock(service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);

	return service;
}

service_pt serviceComponent_removeServiceDependency(service_pt service, service_dependency_pt dependency) {
	state_pt old, new;
	apr_thread_mutex_lock(service->mutex);
	old = service->state;
	arrayList_removeElement(service->dependencies, dependency);
	apr_thread_mutex_unlock(service->mutex);

	if (state_isTrackingOptional(old) || ( state_isWaitingForRequired(old) && dependency->required)) {
		serviceDependency_stop(dependency, service);
	}

	apr_thread_mutex_lock(service->mutex);
	new = state_create(arrayList_clone(service->dependencies), !state_isInactive(old));
	service->state = new;
	apr_thread_mutex_unlock(service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);

	return service;
}

void serviceComponent_dependencyAvailable(service_pt service, service_dependency_pt dependency) {
	state_pt old, new;
	apr_thread_mutex_lock(service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->dependencies), !state_isInactive(old));
	service->state = new;
	apr_thread_mutex_unlock(service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
	if (state_isTrackingOptional(new)) {
		executor_enqueue(service->executor, service, serviceComponent_updateInstance, dependency);
		executor_execute(service->executor);
	}
}

void serviceComponent_dependencyChanged(service_pt service, service_dependency_pt dependency) {
	state_pt state;
	apr_thread_mutex_lock(service->mutex);
	state = service->state;
	apr_thread_mutex_unlock(service->mutex);
	if (state_isTrackingOptional(state)) {
		executor_enqueue(service->executor, service, serviceComponent_updateInstance, dependency);
		executor_execute(service->executor);
	}
}

void serviceComponent_dependencyUnavailable(service_pt service, service_dependency_pt dependency) {
	state_pt old, new;
	apr_thread_mutex_lock(service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->dependencies), !state_isInactive(old));
	service->state = new;
	apr_thread_mutex_unlock(service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
	if (state_isTrackingOptional(new)) {
		executor_enqueue(service->executor, service, serviceComponent_updateInstance, dependency);
		executor_execute(service->executor);
	}
}

void serviceComponent_start(service_pt service) {
	state_pt old, new;
	bundleContext_registerService(service->context, DEPENDENCYMANAGER_SERVICE_COMPONENT_NAME, service, NULL, &service->serviceRegistration);
	apr_thread_mutex_lock(service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->dependencies), true);
	service->state = new;
	apr_thread_mutex_unlock(service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
}

void serviceComponent_stop(service_pt service) {
	state_pt old, new;
	apr_thread_mutex_lock(service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->dependencies), false);
	service->state = new;
	apr_thread_mutex_unlock(service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
	serviceRegistration_unregister(service->serviceRegistration);
}

service_pt serviceComponent_setInterface(service_pt service, char * serviceName, properties_pt properties) {
	service->serviceName = serviceName;
	service->properties = properties;

	return service;
}

service_pt serviceComponent_setImplementation(service_pt service, void * implementation) {
	service->impl = implementation;
	return service;
}

void serviceComponent_activateService(service_pt service, void * arg) {
	state_pt state = (state_pt) arg;
	serviceComponent_initService(service);
	service->init(service->impl);
	serviceComponent_configureService(service, state);
	service->start(service->impl);
	serviceComponent_startTrackingOptional(service, state);
	serviceComponent_registerService(service);
}

void serviceComponent_deactivateService(service_pt service, void * arg) {
	state_pt state = (state_pt) arg;
	serviceComponent_unregisterService(service);
	serviceComponent_stopTrackingOptional(service, state);
	service->stop(service->impl);
	service->destroy(service->impl);
	serviceComponent_destroyService(service, state);
}

void serviceComponent_startTrackingOptional(service_pt service, state_pt state) {
    array_list_pt deps = arrayList_clone(state->dependencies);
	array_list_iterator_pt i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		service_dependency_pt dependency = (service_dependency_pt) arrayListIterator_next(i);
		if (!dependency->required) {
			serviceDependency_start(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_stopTrackingOptional(service_pt service, state_pt state) {
    array_list_pt deps = arrayList_clone(state->dependencies);
	array_list_iterator_pt i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		service_dependency_pt dependency = (service_dependency_pt) arrayListIterator_next(i);
		if (!dependency->required) {
			serviceDependency_stop(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_startTrackingRequired(service_pt service, void * arg) {
	state_pt state = (state_pt) arg;
	array_list_pt deps = arrayList_clone(state->dependencies);
    array_list_iterator_pt i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		service_dependency_pt dependency = (service_dependency_pt) arrayListIterator_next(i);
		if (dependency->required) {
			serviceDependency_start(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_stopTrackingRequired(service_pt service, void * arg) {
	state_pt state = (state_pt) arg;
	array_list_pt deps = arrayList_clone(state->dependencies);
    array_list_iterator_pt i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		service_dependency_pt dependency = (service_dependency_pt) arrayListIterator_next(i);
		if (dependency->required) {
			serviceDependency_stop(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_initService(service_pt service) {
}

void serviceComponent_configureService(service_pt service, state_pt state) {
	array_list_iterator_pt i = arrayListIterator_create(state->dependencies);
	while (arrayListIterator_hasNext(i)) {
		service_dependency_pt dependency = (service_dependency_pt) arrayListIterator_next(i);
		if (dependency->autoConfigureField != NULL) {
			*dependency->autoConfigureField = serviceDependency_getService(dependency);
		}
		if (dependency->required) {
			serviceDependency_invokeAdded(dependency);
		}
	}
	arrayListIterator_destroy(i);
}

void serviceComponent_destroyService(service_pt service, state_pt state) {
	array_list_iterator_pt i = arrayListIterator_create(state->dependencies);
	while (arrayListIterator_hasNext(i)) {
		service_dependency_pt dependency = (service_dependency_pt) arrayListIterator_next(i);
		if (dependency->required) {
			serviceDependency_invokeRemoved(dependency);
		}
	}

	arrayListIterator_destroy(i);

	arrayList_destroy(service->dependencies);
}

void serviceComponent_registerService(service_pt service) {
	if (service->serviceName != NULL) {
		bundleContext_registerService(service->context, service->serviceName, service->impl, service->properties, &service->registration);
	}
}

void serviceComponent_unregisterService(service_pt service) {
	if (service->serviceName != NULL) {
		serviceRegistration_unregister(service->registration);
	}
}

void serviceComponent_updateInstance(service_pt service, void * arg) {
	service_dependency_pt dependency = (service_dependency_pt) arg;
	if (dependency->autoConfigureField != NULL) {
		*dependency->autoConfigureField = serviceDependency_getService(dependency);
	}
}

char * serviceComponent_getName(service_pt service) {
	return service->serviceName;
}

state_pt state_create(array_list_pt dependencies, bool active) {
	state_pt state = (state_pt) malloc(sizeof(*state));
	state->dependencies = dependencies;
	if (active) {
		bool allReqAvail = true;
		unsigned int i;
		for (i = 0; i < arrayList_size(dependencies); i++) {
			service_dependency_pt dependency = arrayList_get(dependencies, i);
			if (dependency->required) {
				if (!dependency->available) {
					allReqAvail = false;
					break;
				}
			}
		}
		if (allReqAvail) {
			state->state = STATE_TRACKING_OPTIONAL;
		} else {
			state->state = STATE_WAITING_FOR_REQUIRED;
		}
	} else {
		state->state = STATE_INACTIVE;
	}

	return state;
}

void state_destroy(state_pt state) {
	if (state->dependencies != NULL) {
		arrayList_destroy(state->dependencies);
		state->dependencies = NULL;
	}
	state->state = STATE_INACTIVE;
	free(state);
	state = NULL;
}

bool state_isInactive(state_pt state) {
	return state->state == STATE_INACTIVE;
}

bool state_isWaitingForRequired(state_pt state) {
	return state->state == STATE_WAITING_FOR_REQUIRED;
}

bool state_isTrackingOptional(state_pt state) {
	return state->state == STATE_TRACKING_OPTIONAL;
}

array_list_pt state_getDependencies(state_pt state) {
	return state->dependencies;
}

executor_pt executor_create(apr_pool_t *memory_pool) {
	executor_pt executor;

	executor = (executor_pt) apr_pcalloc(memory_pool, sizeof(*executor));
	if (executor) {
        linkedList_create(memory_pool, &executor->workQueue);
        executor->active = NULL;
		apr_thread_mutex_create(&executor->mutex, APR_THREAD_MUTEX_UNNESTED, memory_pool);
	}

	return executor;
}

void executor_enqueue(executor_pt executor, service_pt service, void (*function), void * argument) {
	struct executorEntry * entry = NULL;
	apr_thread_mutex_lock(executor->mutex);
	entry = (struct executorEntry *) malloc(sizeof(*entry));
	entry->service = service;
	entry->function = function;
	entry->argument = argument;
	linkedList_addLast(executor->workQueue, entry);
	apr_thread_mutex_unlock(executor->mutex);
}

void executor_execute(executor_pt executor) {
	struct executorEntry * active;
	apr_thread_mutex_lock(executor->mutex);
	active = executor->active;
	apr_thread_mutex_unlock(executor->mutex);
	if (active == NULL) {
		executor_scheduleNext(executor);
	}
}

void executor_scheduleNext(executor_pt executor) {
	struct executorEntry * entry = NULL;
	apr_thread_mutex_lock(executor->mutex);
	entry = linkedList_removeFirst(executor->workQueue);
	apr_thread_mutex_unlock(executor->mutex);
	if (entry != NULL) {
		entry->function(entry->service, entry->argument);
		executor_scheduleNext(executor);
	}
}
