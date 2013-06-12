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

typedef struct state * state_pt;
typedef struct executor * executor_pt;

struct service {
	apr_pool_t *pool;

	array_list_pt dependencies;
	void (*init)(void * userData);
	void (*start)(void * userData);
	void (*stop)(void * userData);
	void (*destroy)(void * userData);

	char * serviceName;
	void * impl;
	properties_pt properties;

	bundle_context_pt context;
	dependency_manager_pt manager;

	service_registration_pt registration;
	service_registration_pt serviceRegistration;

	bool registered;

	state_pt state;
	executor_pt executor;

	apr_thread_mutex_t *mutex;
};

service_pt serviceComponent_create(bundle_context_pt context, dependency_manager_pt manager);
service_pt serviceComponent_addServiceDependency(service_pt service, service_dependency_pt dependency);
service_pt serviceComponent_removeServiceDependency(service_pt service, service_dependency_pt dependency);
void serviceComponent_dependencyAvailable(service_pt service, service_dependency_pt dependency);
void serviceComponent_dependencyChanged(service_pt service, service_dependency_pt dependency);
void serviceComponent_dependencyUnavailable(service_pt service, service_dependency_pt dependency);
void serviceComponent_start(service_pt service);
void serviceComponent_stop(service_pt service);
service_pt serviceComponent_setInterface(service_pt service, char * serviceName, properties_pt properties);
service_pt serviceComponent_setImplementation(service_pt service, void * implementation);
void serviceComponent_activateService(service_pt service, void * arg);
void serviceComponent_deactivateService(service_pt service, void * arg);
void serviceComponent_startTrackingOptional(service_pt service, state_pt state);
void serviceComponent_stopTrackingOptional(service_pt service, state_pt state);
void serviceComponent_startTrackingRequired(service_pt service, void * arg);
void serviceComponent_stopTrackingRequired(service_pt service, void * arg);
void serviceComponent_initService(service_pt service);
void serviceComponent_configureService(service_pt service, state_pt state);
void serviceComponent_destroyService(service_pt service, state_pt state);
void serviceComponent_registerService(service_pt service);
void serviceComponent_unregisterService(service_pt service);
void serviceComponent_updateInstance(service_pt service, void * arg);
char * serviceComponent_getName(service_pt service);


state_pt state_create(array_list_pt dependencies, bool active);
void state_destroy(state_pt state);
bool state_isInactive(state_pt state);
bool state_isWaitingForRequired(state_pt state);
bool state_isTrackingOptional(state_pt state);
array_list_pt state_getDependencies(state_pt state);

executor_pt executor_create(apr_pool_t *memory_pool);
void executor_enqueue(executor_pt executor, service_pt service, void (*function), void * argument);
void executor_execute(executor_pt executor);
void executor_scheduleNext(executor_pt executor);

#endif /* SERVICE_COMPONENT_PRIVATE_H_ */
