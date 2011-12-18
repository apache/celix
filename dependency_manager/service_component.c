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
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "headers.h"
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
	ARRAY_LIST dependencies;
	STATE_STATE state;
};

struct executorEntry {
	SERVICE service;
	void (*function)(void *, void*);
	void * argument;
};

struct executor {
	struct executorEntry * active;
	LINKED_LIST workQueue;

	pthread_mutex_t mutex;
};

SERVICE serviceComponent_create(BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
    SERVICE service;
    apr_pool_t *pool;

	bundleContext_getMemoryPool(context, &pool);

	apr_pool_t *mypool;
	apr_pool_create(&mypool, pool);

	if (mypool) {
        service = (SERVICE) apr_pcalloc(mypool, sizeof(*service));
        service->pool = mypool;
        service->impl = NULL;
        service->serviceName = NULL;
        service->serviceRegistration = NULL;
        service->dependencies = NULL;
        arrayList_create(mypool, &service->dependencies);

        service->init = service_init;
        service->start= service_start;
        service->stop = service_stop;
        service->destroy = service_destroy;

        service->context = context;
        service->manager = manager;
        service->state = state_create(arrayList_clone(mypool, service->dependencies), false);
        service->executor = executor_create(mypool);

        pthread_mutex_init(&service->mutex, NULL);
	}

	return service;
}

void serviceComponent_calculateStateChanges(SERVICE service, const STATE old, const STATE new) {
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

SERVICE serviceComponent_addServiceDependency(SERVICE service, SERVICE_DEPENDENCY dependency) {
	STATE old, new;
	pthread_mutex_lock(&service->mutex);
	old = service->state;
	arrayList_add(service->dependencies, dependency);
	pthread_mutex_unlock(&service->mutex);

	if (state_isTrackingOptional(old) || ( state_isWaitingForRequired(old) && dependency->required)) {
		serviceDependency_start(dependency, service);
	}

	pthread_mutex_lock(&service->mutex);
	new = state_create(arrayList_clone(service->pool, service->dependencies), !state_isInactive(old));
	service->state = new;
	pthread_mutex_unlock(&service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);

	return service;
}

SERVICE serviceComponent_removeServiceDependency(SERVICE service, SERVICE_DEPENDENCY dependency) {
	STATE old, new;
	pthread_mutex_lock(&service->mutex);
	old = service->state;
	arrayList_removeElement(service->dependencies, dependency);
	pthread_mutex_unlock(&service->mutex);

	if (state_isTrackingOptional(old) || ( state_isWaitingForRequired(old) && dependency->required)) {
		serviceDependency_stop(dependency, service);
	}

	pthread_mutex_lock(&service->mutex);
	new = state_create(arrayList_clone(service->pool, service->dependencies), !state_isInactive(old));
	service->state = new;
	pthread_mutex_unlock(&service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);

	return service;
}

void serviceComponent_dependencyAvailable(SERVICE service, SERVICE_DEPENDENCY dependency) {
	STATE old, new;
	pthread_mutex_lock(&service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->pool, service->dependencies), !state_isInactive(old));
	service->state = new;
	pthread_mutex_unlock(&service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
	if (state_isTrackingOptional(new)) {
		executor_enqueue(service->executor, service, serviceComponent_updateInstance, dependency);
		executor_execute(service->executor);
	}
}

void serviceComponent_dependencyChanged(SERVICE service, SERVICE_DEPENDENCY dependency) {
	STATE state;
	pthread_mutex_lock(&service->mutex);
	state = service->state;
	pthread_mutex_unlock(&service->mutex);
	if (state_isTrackingOptional(state)) {
		executor_enqueue(service->executor, service, serviceComponent_updateInstance, dependency);
		executor_execute(service->executor);
	}
}

void serviceComponent_dependencyUnavailable(SERVICE service, SERVICE_DEPENDENCY dependency) {
	STATE old, new;
	pthread_mutex_lock(&service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->pool, service->dependencies), !state_isInactive(old));
	service->state = new;
	pthread_mutex_unlock(&service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
	if (state_isTrackingOptional(new)) {
		executor_enqueue(service->executor, service, serviceComponent_updateInstance, dependency);
		executor_execute(service->executor);
	}
}

void serviceComponent_start(SERVICE service) {
	bundleContext_registerService(service->context, SERVICE_COMPONENT_NAME, service, NULL, &service->serviceRegistration);
	STATE old, new;
	pthread_mutex_lock(&service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->pool, service->dependencies), true);
	service->state = new;
	pthread_mutex_unlock(&service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
}

void serviceComponent_stop(SERVICE service) {
	STATE old, new;
	pthread_mutex_lock(&service->mutex);
	old = service->state;
	new = state_create(arrayList_clone(service->pool, service->dependencies), false);
	service->state = new;
	pthread_mutex_unlock(&service->mutex);
	serviceComponent_calculateStateChanges(service, old, new);
	state_destroy(old);
	serviceRegistration_unregister(service->serviceRegistration);
}

SERVICE serviceComponent_setInterface(SERVICE service, char * serviceName, PROPERTIES properties) {
	service->serviceName = serviceName;
	service->properties = properties;

	return service;
}

SERVICE serviceComponent_setImplementation(SERVICE service, void * implementation) {
	service->impl = implementation;
	return service;
}

void serviceComponent_activateService(SERVICE service, void * arg) {
	STATE state = (STATE) arg;
	serviceComponent_initService(service);
	service->init(service->impl);
	serviceComponent_configureService(service, state);
	service->start(service->impl);
	serviceComponent_startTrackingOptional(service, state);
	serviceComponent_registerService(service);
}

void serviceComponent_deactivateService(SERVICE service, void * arg) {
	STATE state = (STATE) arg;
	serviceComponent_unregisterService(service);
	serviceComponent_stopTrackingOptional(service, state);
	service->stop(service->impl);
	service->destroy(service->impl);
	serviceComponent_destroyService(service, state);
}

void serviceComponent_startTrackingOptional(SERVICE service, STATE state) {
    ARRAY_LIST deps = arrayList_clone(service->pool, state->dependencies);
	ARRAY_LIST_ITERATOR i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arrayListIterator_next(i);
		if (!dependency->required) {
			serviceDependency_start(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_stopTrackingOptional(SERVICE service, STATE state) {
    ARRAY_LIST deps = arrayList_clone(service->pool, state->dependencies);
	ARRAY_LIST_ITERATOR i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arrayListIterator_next(i);
		if (!dependency->required) {
			serviceDependency_stop(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_startTrackingRequired(SERVICE service, void * arg) {
	STATE state = (STATE) arg;
	ARRAY_LIST deps = arrayList_clone(service->pool, state->dependencies);
    ARRAY_LIST_ITERATOR i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arrayListIterator_next(i);
		if (dependency->required) {
			serviceDependency_start(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_stopTrackingRequired(SERVICE service, void * arg) {
	STATE state = (STATE) arg;
	ARRAY_LIST deps = arrayList_clone(service->pool, state->dependencies);
    ARRAY_LIST_ITERATOR i = arrayListIterator_create(deps);
	while (arrayListIterator_hasNext(i)) {
		SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arrayListIterator_next(i);
		if (dependency->required) {
			serviceDependency_stop(dependency, service);
		}
	}
	arrayListIterator_destroy(i);
	arrayList_destroy(deps);
}

void serviceComponent_initService(SERVICE service) {
}

void serviceComponent_configureService(SERVICE service, STATE state) {
	ARRAY_LIST_ITERATOR i = arrayListIterator_create(state->dependencies);
	while (arrayListIterator_hasNext(i)) {
		SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arrayListIterator_next(i);
		if (dependency->autoConfigureField != NULL) {
			*dependency->autoConfigureField = serviceDependency_getService(dependency);
		}
		if (dependency->required) {
			serviceDependency_invokeAdded(dependency);
		}
	}
	arrayListIterator_destroy(i);
}

void serviceComponent_destroyService(SERVICE service, STATE state) {
	ARRAY_LIST_ITERATOR i = arrayListIterator_create(state->dependencies);
	while (arrayListIterator_hasNext(i)) {
		SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arrayListIterator_next(i);
		if (dependency->required) {
			serviceDependency_invokeRemoved(dependency);
		}
	}

	arrayListIterator_destroy(i);

	arrayList_destroy(service->dependencies);
}

void serviceComponent_registerService(SERVICE service) {
	if (service->serviceName != NULL) {
		bundleContext_registerService(service->context, service->serviceName, service->impl, service->properties, &service->registration);
	}
}

void serviceComponent_unregisterService(SERVICE service) {
	if (service->serviceName != NULL) {
		serviceRegistration_unregister(service->registration);
	}
}

void serviceComponent_updateInstance(SERVICE service, void * arg) {
	SERVICE_DEPENDENCY dependency = (SERVICE_DEPENDENCY) arg;
	if (dependency->autoConfigureField != NULL) {
		*dependency->autoConfigureField = serviceDependency_getService(dependency);
	}
}

char * serviceComponent_getName(SERVICE service) {
	return service->serviceName;
}

STATE state_create(ARRAY_LIST dependencies, bool active) {
	STATE state = (STATE) malloc(sizeof(*state));
	state->dependencies = dependencies;
	if (active) {
		bool allReqAvail = true;
		int i;
		for (i = 0; i < arrayList_size(dependencies); i++) {
			SERVICE_DEPENDENCY dependency = arrayList_get(dependencies, i);
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

void state_destroy(STATE state) {
	if (state->dependencies != NULL) {
		arrayList_destroy(state->dependencies);
		state->dependencies = NULL;
	}
	state->state = STATE_INACTIVE;
	free(state);
	state = NULL;
}

bool state_isInactive(STATE state) {
	return state->state == STATE_INACTIVE;
}

bool state_isWaitingForRequired(STATE state) {
	return state->state == STATE_WAITING_FOR_REQUIRED;
}

bool state_isTrackingOptional(STATE state) {
	return state->state == STATE_TRACKING_OPTIONAL;
}

ARRAY_LIST state_getDependencies(STATE state) {
	return state->dependencies;
}

EXECUTOR executor_create(apr_pool_t *memory_pool) {
	EXECUTOR executor;

	executor = (EXECUTOR) apr_pcalloc(memory_pool, sizeof(*executor));
	if (executor) {
        linkedList_create(memory_pool, &executor->workQueue);
        executor->active = NULL;
        pthread_mutex_init(&executor->mutex, NULL);
	}

	return executor;
}

void executor_enqueue(EXECUTOR executor, SERVICE service, void (*function), void * argument) {
	pthread_mutex_lock(&executor->mutex);
	struct executorEntry * entry = malloc(sizeof(*entry));
	entry->service = service;
	entry->function = function;
	entry->argument = argument;
	linkedList_addLast(executor->workQueue, entry);
	pthread_mutex_unlock(&executor->mutex);
}

void executor_execute(EXECUTOR executor) {
	struct executorEntry * active;
	pthread_mutex_lock(&executor->mutex);
	active = executor->active;
	pthread_mutex_unlock(&executor->mutex);
	if (active == NULL) {
		executor_scheduleNext(executor);
	}
}

void executor_scheduleNext(EXECUTOR executor) {
	struct executorEntry * entry = NULL;
	pthread_mutex_lock(&executor->mutex);
	entry = linkedList_removeFirst(executor->workQueue);
	pthread_mutex_unlock(&executor->mutex);
	if (entry != NULL) {
		entry->function(entry->service, entry->argument);
		executor_scheduleNext(executor);
	}
}
