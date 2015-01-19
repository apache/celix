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
 * activator.c
 *
 *  \date       Aug 23, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <unistd.h>

#include "celixbool.h"

#include "bundle_activator.h"
#include "publisher.h"
#include "service_tracker.h"
#include "bundle_context.h"

struct data {
	bundle_context_pt context;
	service_tracker_pt tracker;
	array_list_pt publishers;
	pthread_t sender;
	bool running;
};

static void *trk_send(void *handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			publisher_service_pt pub = arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "test");
		}
		usleep(1000000);
	}
	pthread_exit(NULL);
	return NULL;
}

celix_status_t addingServ(void * handle, service_reference_pt ref, void **service) {
    struct data * data = (struct data *) handle;

    printf("Adding\n");
	bundleContext_getService(data->context, ref, service);

	return CELIX_SUCCESS;
}

celix_status_t addedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Added %p\n", service);
	return CELIX_SUCCESS;
}

celix_status_t modifiedServ(void * handle, service_reference_pt ref, void * service) {
	printf("Modified\n");
	return CELIX_SUCCESS;
}

celix_status_t removedServ(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Removed %p\n", service);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    if (status == CELIX_SUCCESS) {
        *userData = calloc(1, sizeof(struct data));
        ((struct data *) (*userData))->publishers = NULL;
        arrayList_create(&((struct data *) (*userData))->publishers);
    } else {
        status = CELIX_START_ERROR;
    }
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    if (status == CELIX_SUCCESS) {
        struct data * data = (struct data *) userData;
		service_tracker_customizer_pt cust = NULL;
		service_tracker_pt tracker = NULL;
        
		data->context = context;

        serviceTrackerCustomizer_create(data, addingServ, addedServ, modifiedServ, removedServ, &cust);
        serviceTracker_create(context, (char *) PUBLISHER_NAME, cust, &tracker);

        data->tracker = tracker;

        serviceTracker_open(tracker);

        data->running = true;
        pthread_create(&data->sender, NULL, trk_send, data);
    } else {
        status = CELIX_START_ERROR;
    }
    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct data * data = (struct data *) userData;

	printf("Stop\n");
    serviceTracker_close(data->tracker);
    data->running = false;
    pthread_join(data->sender, NULL);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct data * data = (struct data *) userData;

    arrayList_destroy(data->publishers);

    return status;
}
