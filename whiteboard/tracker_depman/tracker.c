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
 *  Created on: Mar 7, 2011
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "celixbool.h"

#include "service.h"
#include "publisher.h"
#include "tracker.h"
#include "log_service.h"

void * dp_send(void * handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			PUBLISHER_SERVICE pub = (PUBLISHER_SERVICE) arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "Tracker message");
		}
		sleep(1);
	}
	pthread_exit(NULL);
	return NULL;
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
	pthread_join(data->sender, NULL);
}

void service_destroy(void * userData) {

}

void tracker_addedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Service Added\n");
}

void tracker_modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Service Changed\n");
}

void tracker_removedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Service Removed\n");
}

void tracker_addLog(void *handle, SERVICE_REFERENCE ref, void *service) {
    printf("Add log\n");
    ((log_service_t) service)->log(((log_service_t) service)->logger, LOG_DEBUG, "test");
}

void tracker_modifiedLog(void *handle, SERVICE_REFERENCE ref, void *service) {
    printf("Modify log\n");
    ((log_service_t) service)->log(((log_service_t) service)->logger, LOG_DEBUG, "test");
}

void tracker_removeLog(void *handle, SERVICE_REFERENCE ref, void *service) {
    printf("Remove log\n");
}
