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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include "celixbool.h"

#include "service.h"
#include "publisher.h"
#include "tracker.h"
#include "log_service.h"

static void *APR_THREAD_FUNC dp_send(apr_thread_t *thd, void *handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			publisher_service_pt pub = (publisher_service_pt) arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "Tracker message");
			if (data->logger != NULL) {
				data->logger->log(data->logger->logger, OSGI_LOGSERVICE_INFO, "Sending message to publisher");
			}
		}
		apr_sleep(1000000);
	}
	apr_thread_exit(thd, APR_SUCCESS);
	return NULL;
}

void service_init(void * userData) {

}

void service_start(void * userData) {
	struct data * data = (struct data *) userData;
	apr_pool_t *pool = NULL;

	data->running = true;
	bundleContext_getMemoryPool(data->context, &pool);
	apr_thread_create(&data->sender, NULL, dp_send, data, pool);
}

void service_stop(void * userData) {
	apr_status_t stat;
	struct data * data = (struct data *) userData;
	data->running = false;
	apr_thread_join(&stat, data->sender);
}

void service_destroy(void * userData) {

}

void tracker_addedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Service Added\n");
}

void tracker_modifiedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Service Changed\n");
}

void tracker_removedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Service Removed\n");
}

void tracker_addLog(void *handle, service_reference_pt ref, void *service) {
    struct data * data = (struct data *) handle;
    printf("Add log\n");
    data->logger = service;
    ((log_service_pt) service)->log(((log_service_pt) service)->logger, OSGI_LOGSERVICE_DEBUG, "test");
}

void tracker_modifiedLog(void *handle, service_reference_pt ref, void *service) {
    struct data * data = (struct data *) handle;
    printf("Modify log\n");
    data->logger = service;
    ((log_service_pt) service)->log(((log_service_pt) service)->logger, OSGI_LOGSERVICE_DEBUG, "test");
}

void tracker_removeLog(void *handle, service_reference_pt ref, void *service) {
    struct data * data = (struct data *) handle;
    data->logger = NULL;
    printf("Remove log\n");
}
