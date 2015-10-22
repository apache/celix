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
 * tracker.c
 *
 *  \date       Mar 7, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "celixbool.h"

#include "publisher.h"
#include "tracker.h"

static void *dp_send(void *handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		printf("Running\n");
		int i;
		// lock
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			publisher_service_pt pub = (publisher_service_pt) arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "Tracker message");
			celixThreadMutex_lock(&data->logger_lock);
			printf("%p\n", data->logger);
			if (data->logger != NULL) {
				data->logger->log(data->logger->logger, OSGI_LOGSERVICE_INFO, "Sending message to publisher");
			}
			celixThreadMutex_unlock(&data->logger_lock);
		}
		// lock
		usleep(1000000);
	}
	pthread_exit(NULL);
	return NULL;
}

celix_status_t service_init(void * userData) {
	fprintf(stderr, "Service init");
	return CELIX_SUCCESS;
}

celix_status_t service_start(void * userData) {
	struct data * data = (struct data *) userData;
	fprintf(stderr, "Service started\n");
	data->running = true;
	pthread_create(&data->sender, NULL, dp_send, data);
	return CELIX_SUCCESS;
}

celix_status_t service_stop(void * userData) {
	struct data * data = (struct data *) userData;
	fprintf(stderr, "Service stopped\n");
	data->running = false;
	pthread_join(data->sender, NULL);
	return CELIX_SUCCESS;
}

celix_status_t service_deinit(void * userData) {
	fprintf(stderr, "Service deinit\n");
	return CELIX_SUCCESS;
}

celix_status_t tracker_setServ(void * handle, service_reference_pt ref, void * service) {
	printf("Service Set %p\n", service);
	return CELIX_SUCCESS;
}


celix_status_t tracker_addedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Service Added %p\n", service);
	return CELIX_SUCCESS;
}

celix_status_t tracker_modifiedServ(void * handle, service_reference_pt ref, void * service) {
    printf("Service Changed\n");
	return CELIX_SUCCESS;
}

celix_status_t tracker_removedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Service Removed\n");
	return CELIX_SUCCESS;
}

celix_status_t tracker_setLog(void *handle, service_reference_pt ref, void *service) {
	struct data * data = (struct data *) handle;

	printf("SET log %p\n", service);
	if(service) {
		data->logger = service;
		((log_service_pt) service)->log(((log_service_pt) service)->logger, OSGI_LOGSERVICE_DEBUG, "SET log");
	}
	fprintf(stderr, "SET end %p\n", service);
	return CELIX_SUCCESS;
}

celix_status_t tracker_addLog(void *handle, service_reference_pt ref, void *service) {
    struct data * data = (struct data *) handle;
    printf("Add log %p\n", service);
    data->logger = service;
    ((log_service_pt) service)->log(((log_service_pt) service)->logger, OSGI_LOGSERVICE_DEBUG, "test");
    return CELIX_SUCCESS;
}

celix_status_t tracker_modifiedLog(void *handle, service_reference_pt ref, void *service) {
    struct data * data = (struct data *) handle;
    printf("Modify log\n");
    data->logger = service;
    ((log_service_pt) service)->log(((log_service_pt) service)->logger, OSGI_LOGSERVICE_DEBUG, "test");
    return CELIX_SUCCESS;
}

celix_status_t tracker_removeLog(void *handle, service_reference_pt ref, void *service) {
    struct data * data = (struct data *) handle;
    data->logger = NULL;
    printf("Remove log\n");
    return CELIX_SUCCESS;
}
