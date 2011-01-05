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
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "dependency_activator_base.h"
#include "service_component_private.h"
#include "service.h"
#include "publisher.h"

struct data {
	SERVICE service;
	BUNDLE_CONTEXT context;
	ARRAY_LIST publishers;
	pthread_t sender;
	bool running;
};

void * dp_send(void * handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			PUBLISHER_SERVICE pub = (PUBLISHER_SERVICE) arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "test");
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

void service_init(void * userData) {

}

void service_start(void * userData) {
	struct data * data = (struct data *) userData;
	data->running = true;
	pthread_create(&data->sender, NULL, dp_send, data);
}

void service_stop(void * userData) {
	struct data * data = (struct data *) userData;
	data->running = false;
}

void service_destroy(void * userData) {

}

void addedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Service Added\n");
}

void modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Service Changed\n");
}

void removedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Service Removed\n");
}

void * dm_create() {
	struct data * data = malloc(sizeof(*data));
	data->publishers = arrayList_create();
	return data;
}

void dm_init(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
	struct data * data = (struct data *) userData;
	data->context = context;

	SERVICE service = dependencyActivatorBase_createService(manager);
	serviceComponent_setImplementation(service, data);
	SERVICE_DEPENDENCY dep = dependencyActivatorBase_createServiceDependency(manager);
	serviceDependency_setRequired(dep, false);
	serviceDependency_setService(dep, PUBLISHER_NAME, NULL);
	serviceDependency_setCallbacks(dep, addedServ, modifiedServ, removedServ);
	serviceComponent_addServiceDependency(service, dep);

	data->service = service;
	dependencyManager_add(manager, service);
}

void dm_destroy(void * userData, BUNDLE_CONTEXT context, DEPENDENCY_MANAGER manager) {
	struct data * data = (struct data *) userData;
	dependencyManager_remove(manager, data->service);
}

