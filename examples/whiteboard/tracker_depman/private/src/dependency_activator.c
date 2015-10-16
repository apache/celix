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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include "celixbool.h"

#include "dm_activator_base.h"
#include "publisher.h"
#include "tracker.h"

celix_status_t dm_create(bundle_context_pt context, void **userData) {
	printf("Create\n");
	struct data * data = malloc(sizeof(*data));
	data->publishers = NULL;
	arrayList_create(&data->publishers);
	data->context = NULL;
	data->running = false;
	data->sender = 0;
	data->service = NULL;
	data->logger = NULL;
	*userData = data;
	return CELIX_SUCCESS;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	printf("Init\n");
	struct data * data = (struct data *) userData;
	dm_component_pt service = NULL;
	dm_service_dependency_pt dep1 = NULL;
	dm_service_dependency_pt dep2 = NULL;

	data->context = context;

	component_create(context, manager, &service);
	component_setImplementation(service, data);
	component_setCallbacks(service, service_init, service_start, service_stop, service_deinit);

	serviceDependency_create(&dep1);
	serviceDependency_setRequired(dep1, true);
	serviceDependency_setService(dep1, PUBLISHER_NAME, "(|(id=A)(id=B))");
	serviceDependency_setCallbacksWithServiceReference(dep1, NULL /*tracker_setServ*/, tracker_addedServ, tracker_modifiedServ, tracker_removedServ, NULL);
	component_addServiceDependency(service, dep1, NULL);

	serviceDependency_create(&dep2);
    serviceDependency_setRequired(dep2, false);
    serviceDependency_setService(dep2, (char *) OSGI_LOGSERVICE_NAME, NULL);
    serviceDependency_setCallbacksWithServiceReference(dep2, NULL  /*tracker_setLog*/, tracker_addLog, tracker_modifiedLog, tracker_removeLog, NULL);
	serviceDependency_setAutoConfigure(dep2, &data->logger_lock, (void **) &data->logger);
    component_addServiceDependency(service, dep2, NULL);

	data->service = service;
	data->dep = dep1;
	data->dep2 = dep2;

	dependencyManager_add(manager, service);

	return CELIX_SUCCESS;
}

celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
	struct data * data = (struct data *) userData;
	dependencyManager_remove(manager, data->service);

	component_removeServiceDependency(data->service, data->dep);
	serviceDependency_destroy(&data->dep);

	component_removeServiceDependency(data->service, data->dep2);
	serviceDependency_destroy(&data->dep2);

	component_destroy(&data->service);

	arrayList_destroy(data->publishers);
	data->publishers = NULL;
	free(data);
	data = NULL;
	return CELIX_SUCCESS;
}

//int count;
//
//void lock(int state) {
//	pthread_mutex_t lock;
//	if (state == 1) {
//		if (count > 0) {
//			count++;
//		} else {
//			pthread_mutex_lock(&lock);
//		}
//	} else {
//		pthread_mutex_lock(&lock);
//	}
//}
//
//void unlcok(int state) {
//	if (state == 1) {
//		if (count-- == 0) {
//			pthread_mutex_unlock(&lock);
//		}
//	} else {
//		pthread_mutex_unlock(&lock);
//	}
//}
