/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * activator.c
 *
 *  \date       16 Feb 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>

#include <unistd.h>

#include <sys/time.h>
#include <stdlib.h>
#include <bundle_activator.h>
#include <service_tracker.h>
#include <constants.h>

#include <array_list.h>

#include "writer_service.h"
#include "data.h"

struct activator {
    service_tracker_pt tracker;
    array_list_pt readerServices;
    bool running;
    celix_thread_t worker;
};

celix_status_t writerServiceAdded(void *handle, service_reference_pt reference, void *service);
celix_status_t writerServiceRemoved(void *handle, service_reference_pt reference, void *service);

int globalDataId = 0;

void *produceData(void *handle);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    *userData = calloc(1, sizeof(struct activator));
    if (*userData) {
        ((struct activator *) *userData)->tracker = NULL;
        ((struct activator *) *userData)->readerServices = NULL;
        ((struct activator *) *userData)->running = false;
        ((struct activator *) *userData)->worker = celix_thread_default;
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    // create list for services
    arrayList_create(&activator->readerServices);

    // start the thread
    activator->running = true;
    status = celixThread_create(&activator->worker, NULL, produceData, activator);

    if (status == CELIX_SUCCESS) {
        service_tracker_customizer_pt customizer = NULL;
        serviceTrackerCustomizer_create(userData, NULL, writerServiceAdded, NULL, writerServiceRemoved, &customizer);

        serviceTracker_create(context, WRITER_SERVICE_NAME, customizer, &activator->tracker);
        serviceTracker_open(activator->tracker);
    }

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    // stop the thread
    activator->running = false;
    celixThread_join(activator->worker, NULL);

    serviceTracker_close(activator->tracker);
    serviceTracker_destroy(activator->tracker);

    // destroy the list of services
    arrayList_destroy(activator->readerServices);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    free(activator);

    return status;
}

celix_status_t writerServiceAdded(void *handle, service_reference_pt reference, void *service) {
    struct activator *activator = handle;
    arrayList_add(activator->readerServices, service);
    printf("Producer: Writer Service Added.\n");

    return CELIX_SUCCESS;
}

celix_status_t writerServiceRemoved(void *handle, service_reference_pt reference, void *service) {
    struct activator *activator = handle;
    arrayList_removeElement(activator->readerServices, service);
    printf("Producer: Writer Service Removed.\n");

    return CELIX_SUCCESS;
}

void *produceData(void *handle) {
    struct activator *activator = handle;

    while (activator->running) {
        int i;
        for (i = 0; i < arrayList_size(activator->readerServices); i++) {
            writer_service_pt service = arrayList_get(activator->readerServices, i);
            data_pt newData = calloc(1, sizeof(struct data));

            newData->id = globalDataId++;
            snprintf(newData->description, 100, "%d : Some Description", newData->id);

            if (service->writerService_storeData(service->handler, newData) == CELIX_SUCCESS) {
                printf(" Data #%d stored.\n", newData->id);
            } else {
                printf(" Could not store data. \n");
            }

            srand(time(NULL));
            int r = rand() % 100;

            if (r > 70)
                sleep(10);
        }
    }

    return NULL;
}
