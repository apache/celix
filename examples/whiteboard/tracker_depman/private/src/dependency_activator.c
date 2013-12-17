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
 * dependency_activator.c
 *
 *  \date       Aug 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include "celixbool.h"

#include "dependency_activator_base.h"
#include "service_component_private.h"
#include "publisher.h"
#include "tracker.h"
#include "log_service.h"
#include "bundle_context.h"

void * dm_create(bundle_context_pt context) {
	struct data * data = malloc(sizeof(*data));
	data->publishers = NULL;
	arrayList_create(&data->publishers);
	data->context = NULL;
	data->running = false;
	data->sender = 0;
	data->service = NULL;
	data->logger = NULL;
	return data;
}

void dm_init(void * userData, bundle_context_pt context, dependency_manager_pt manager) {
	struct data * data = (struct data *) userData;
	service_pt service = NULL;
	service_dependency_pt dep = NULL;
	service_dependency_pt dep2 = NULL;

	data->context = context;

	service = dependencyActivatorBase_createService(manager);
	serviceComponent_setImplementation(service, data);

	dep = dependencyActivatorBase_createServiceDependency(manager);
	serviceDependency_setRequired(dep, false);
	serviceDependency_setService(dep, PUBLISHER_NAME, "(|(id=A)(id=B))");
	serviceDependency_setCallbacks(dep, tracker_addedServ, tracker_modifiedServ, tracker_removedServ);
	serviceComponent_addServiceDependency(service, dep);

	dep2 = dependencyActivatorBase_createServiceDependency(manager);
    serviceDependency_setRequired(dep2, false);
    serviceDependency_setService(dep2, (char *) OSGI_LOGSERVICE_NAME, NULL);
    serviceDependency_setCallbacks(dep2, tracker_addLog, tracker_modifiedLog, tracker_removeLog);
    serviceComponent_addServiceDependency(service, dep2);

	data->service = service;
	dependencyManager_add(manager, service);
}

void dm_destroy(void * userData, bundle_context_pt context, dependency_manager_pt manager) {
	struct data * data = (struct data *) userData;
	dependencyManager_remove(manager, data->service);
	arrayList_destroy(data->publishers);
	data->publishers = NULL;
	free(data);
	data = NULL;
}

