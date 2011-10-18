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
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "celixbool.h"

#include "bundle_activator.h"
#include "publisher.h"
#include "service_tracker.h"
#include "bundle_context.h"

struct data {
	BUNDLE_CONTEXT context;
	SERVICE_TRACKER tracker;
	ARRAY_LIST publishers;
	pthread_t sender;
	bool running;
};

void * trk_send(void * handle) {
	struct data * data = (struct data *) handle;
	while (data->running) {
		int i;
		for (i = 0; i < arrayList_size(data->publishers); i++) {
			PUBLISHER_SERVICE pub = arrayList_get(data->publishers, i);
			pub->invoke(pub->publisher, "test");
		}
		sleep(1);
	}
	pthread_exit(NULL);
	return NULL;
}

celix_status_t addingServ(void * handle, SERVICE_REFERENCE ref, void **service) {
    struct data * data = (struct data *) handle;

    printf("Adding\n");
	bundleContext_getService(data->context, ref, service);

	return CELIX_SUCCESS;
}

celix_status_t addedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_add(data->publishers, service);
	printf("Added %p\n", service);
	return CELIX_SUCCESS;
}

celix_status_t modifiedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Modified\n");
	return CELIX_SUCCESS;
}

celix_status_t removedServ(void * handle, SERVICE_REFERENCE ref, void * service) {
	struct data * data = (struct data *) handle;
	arrayList_removeElement(data->publishers, service);
	printf("Removed %p\n", service);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
    apr_pool_t *pool;
    celix_status_t status = bundleContext_getMemoryPool(context, &pool);
    if (status == CELIX_SUCCESS) {
        *userData = apr_palloc(pool, sizeof(struct data));
        ((struct data *) (*userData))->publishers = arrayList_create();
    } else {
        status = CELIX_START_ERROR;
    }
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;
    apr_pool_t *pool;
    status = bundleContext_getMemoryPool(context, &pool);
    if (status == CELIX_SUCCESS) {
        struct data * data = (struct data *) userData;
        data->context = context;

        SERVICE_TRACKER_CUSTOMIZER cust = (SERVICE_TRACKER_CUSTOMIZER) apr_palloc(pool, sizeof(*cust));
        cust->handle = data;
        cust->addedService = addedServ;
        cust->addingService = addingServ;
        cust->modifiedService = modifiedServ;
        cust->removedService = removedServ;
        SERVICE_TRACKER tracker = NULL;
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

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;
    printf("Stop\n");

    struct data * data = (struct data *) userData;
    serviceTracker_close(data->tracker);
    data->running = false;
    pthread_join(data->sender, NULL);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
    celix_status_t status = CELIX_SUCCESS;
    return status;
}
