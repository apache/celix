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

#include <bundle_activator.h>
#include <service_tracker.h>
#include <constants.h>

#include <array_list.h>

#include "reader_service.h"
#include "writer_service.h"
#include "data.h"

struct activator {
    service_tracker_pt readerTracker;
    service_tracker_pt writerTracker;
    array_list_pt readerServices;
    array_list_pt writerServices;
    bool running;
    celix_thread_t worker;
};

celix_status_t readerServiceAdded(void *handle, service_reference_pt reference, void *service);
celix_status_t readerServiceRemoved(void *handle, service_reference_pt reference, void *service);

celix_status_t writerServiceAdded(void *handle, service_reference_pt reference, void *service);
celix_status_t writerServiceRemoved(void *handle, service_reference_pt reference, void *service);

void *retrieveData(void *handle);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    *userData = calloc(1, sizeof(struct activator));
    if (*userData) {
        ((struct activator *) *userData)->readerTracker = NULL;
        ((struct activator *) *userData)->writerTracker = NULL;
        ((struct activator *) *userData)->readerServices = NULL;
        ((struct activator *) *userData)->writerServices = NULL;
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
    arrayList_create(&activator->writerServices);

    // start the thread
    activator->running = true;
    status = celixThread_create(&activator->worker, NULL, retrieveData, activator);

    if (status == CELIX_SUCCESS) {
        service_tracker_customizer_pt readerCustomizer = NULL;
        service_tracker_customizer_pt writerCustomizer = NULL;
        serviceTrackerCustomizer_create(userData, NULL, readerServiceAdded, NULL, readerServiceRemoved, &readerCustomizer);
        serviceTrackerCustomizer_create(userData, NULL, writerServiceAdded, NULL, writerServiceRemoved, &writerCustomizer);

        serviceTracker_create(context, WRITER_SERVICE_NAME, writerCustomizer, &activator->writerTracker);
        serviceTracker_open(activator->writerTracker);

        serviceTracker_create(context, READER_SERVICE_NAME, readerCustomizer, &activator->readerTracker);
        serviceTracker_open(activator->readerTracker);
    }

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    // stop the thread
    activator->running = false;

    celixThread_join(activator->worker, NULL);

    serviceTracker_close(activator->readerTracker);
    serviceTracker_close(activator->writerTracker);

    serviceTracker_destroy(activator->readerTracker);
    serviceTracker_destroy(activator->writerTracker);

    // destroy the list of services
    arrayList_destroy(activator->readerServices);
    arrayList_destroy(activator->writerServices);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;

    free(activator);

    return status;
}

celix_status_t readerServiceAdded(void *handle, service_reference_pt reference, void *service) {
    struct activator *activator = handle;
    arrayList_add(activator->readerServices, service);
    printf("Consumer: Reader Service Added.\n");

    return CELIX_SUCCESS;
}

celix_status_t readerServiceRemoved(void *handle, service_reference_pt reference, void *service) {
    struct activator *activator = handle;
    arrayList_removeElement(activator->readerServices, service);
    printf("Consumer: Reader Service Removed.\n");

    return CELIX_SUCCESS;
}

celix_status_t writerServiceAdded(void *handle, service_reference_pt reference, void *service) {
    struct activator *activator = handle;
    arrayList_add(activator->writerServices, service);
    printf("Consumer: Writer Service Added.\n");

    return CELIX_SUCCESS;
}

celix_status_t writerServiceRemoved(void *handle, service_reference_pt reference, void *service) {
    struct activator *activator = handle;
    arrayList_removeElement(activator->writerServices, service);
    printf("Consumer: Writer Service Removed.\n");

    return CELIX_SUCCESS;
}

void *retrieveData(void *handle) {
    struct activator *activator = handle;

    while (activator->running) {
        int i;
        for (i = 0; i < arrayList_size(activator->readerServices); i++) {
            reader_service_pt service = arrayList_get(activator->readerServices, i);
            data_pt retrievedData = NULL;

            if (service->readerService_getNextData(service->handler, &retrievedData) == CELIX_SUCCESS) {
                printf(" Data #%d received.", retrievedData->id);

                writer_service_pt writerService = arrayList_get(activator->writerServices, 0);
                if (writerService && writerService->writerService_removeData(writerService->handler, &retrievedData) == CELIX_SUCCESS) {
                    printf(" and removed\n");
                }

            } else {
                printf(" No data available\n");
                sleep(5);
            }

        }

    }

    return NULL;
}
