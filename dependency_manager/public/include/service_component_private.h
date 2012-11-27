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
 * service_component_private.h
 *
 *  \date       Aug 9, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SERVICE_COMPONENT_PRIVATE_H_
#define SERVICE_COMPONENT_PRIVATE_H_
#include <celixbool.h>

#include "service_component.h"
#include "service_dependency.h"
#include "properties.h"
#include "dependency_manager.h"

typedef struct state * STATE;
typedef struct executor * EXECUTOR;

struct service {
	apr_pool_t *pool;

	ARRAY_LIST dependencies;
	void (*init)(void * userData);
	void (*start)(void * userData);
	void (*stop)(void * userData);
	void (*destroy)(void * userData);

	char * serviceName;
	void * impl;
	PROPERTIES properties;

	bundle_context_t context;
	DEPENDENCY_MANAGER manager;

	SERVICE_REGISTRATION registration;
	SERVICE_REGISTRATION serviceRegistration;

	bool registered;

	STATE state;
	EXECUTOR executor;

	apr_thread_mutex_t *mutex;
};

SERVICE serviceComponent_create(bundle_context_t context, DEPENDENCY_MANAGER manager);
SERVICE serviceComponent_addServiceDependency(SERVICE service, SERVICE_DEPENDENCY dependency);
SERVICE serviceComponent_removeServiceDependency(SERVICE service, SERVICE_DEPENDENCY dependency);
void serviceComponent_dependencyAvailable(SERVICE service, SERVICE_DEPENDENCY dependency);
void serviceComponent_dependencyChanged(SERVICE service, SERVICE_DEPENDENCY dependency);
void serviceComponent_dependencyUnavailable(SERVICE service, SERVICE_DEPENDENCY dependency);
void serviceComponent_start(SERVICE service);
void serviceComponent_stop(SERVICE service);
SERVICE serviceComponent_setInterface(SERVICE service, char * serviceName, PROPERTIES properties);
SERVICE serviceComponent_setImplementation(SERVICE service, void * implementation);
void serviceComponent_activateService(SERVICE service, void * arg);
void serviceComponent_deactivateService(SERVICE service, void * arg);
void serviceComponent_startTrackingOptional(SERVICE service, STATE state);
void serviceComponent_stopTrackingOptional(SERVICE service, STATE state);
void serviceComponent_startTrackingRequired(SERVICE service, void * arg);
void serviceComponent_stopTrackingRequired(SERVICE service, void * arg);
void serviceComponent_initService(SERVICE service);
void serviceComponent_configureService(SERVICE service, STATE state);
void serviceComponent_destroyService(SERVICE service, STATE state);
void serviceComponent_registerService(SERVICE service);
void serviceComponent_unregisterService(SERVICE service);
void serviceComponent_updateInstance(SERVICE service, void * arg);
char * serviceComponent_getName(SERVICE service);


STATE state_create(ARRAY_LIST dependencies, bool active);
void state_destroy(STATE state);
bool state_isInactive(STATE state);
bool state_isWaitingForRequired(STATE state);
bool state_isTrackingOptional(STATE state);
ARRAY_LIST state_getDependencies(STATE state);

EXECUTOR executor_create(apr_pool_t *memory_pool);
void executor_enqueue(EXECUTOR executor, SERVICE service, void (*function), void * argument);
void executor_execute(EXECUTOR executor);
void executor_scheduleNext(EXECUTOR executor);

#endif /* SERVICE_COMPONENT_PRIVATE_H_ */
