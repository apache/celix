/*
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
/**
 * activator.c
 *
 *  \date      Jul 9, 2013
 *  \author    <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright Apache License, Version 2.0
 */

#include <stdlib.h>

#include "event_admin_impl.h"
#include "log_helper.h"
#include "log_service.h"

struct activator {

    event_admin_service_pt event_admin_service;
    event_admin_pt event_admin;
    service_registration_pt registration;
    service_tracker_pt tracker;
    bundle_context_pt context;
    log_helper_t *loghelper;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
    celix_status_t status = CELIX_SUCCESS;

    struct activator *activator;
    activator = calloc(1, sizeof(*activator));

    if(!activator) {
        status = CELIX_BUNDLE_EXCEPTION;
    }else {
        activator->registration = NULL;
        logHelper_create(context, &activator->loghelper);

        *userData = activator;
        event_admin_pt event_admin = NULL;
        event_admin_service_pt event_admin_service = NULL;
        status = eventAdmin_create(context, &event_admin);
        if(status == CELIX_SUCCESS){
            activator->event_admin = event_admin;
            event_admin_service = calloc(1, sizeof(event_admin_service));
            if(!event_admin_service){
                status = CELIX_ENOMEM;
            } else {
                event_admin->context = context;
                event_admin->loghelper = &activator->loghelper;
                event_admin_service->eventAdmin = event_admin;
                event_admin_service->postEvent = eventAdmin_postEvent;
                event_admin_service->sendEvent = eventAdmin_sendEvent;
                event_admin_service->createEvent = eventAdmin_createEvent;
                event_admin_service->containsProperty = eventAdmin_containsProperty;
                event_admin_service->event_equals = eventAdmin_event_equals;
                event_admin_service->getProperty = eventAdmin_getProperty;
                event_admin_service->getPropertyNames = eventAdmin_getPropertyNames;
                event_admin_service->getTopic = eventAdmin_getTopic;
                event_admin_service->hashCode = eventAdmin_hashCode;
                event_admin_service->matches = eventAdmin_matches;
                event_admin_service->toString = eventAdmin_toString;

            }
        }
        activator->event_admin_service = event_admin_service;
    }


    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator *activator = userData;
    event_admin_service_pt event_admin_service = NULL;

    if(status == CELIX_SUCCESS) {
        struct activator * data = (struct activator *) userData;
        service_tracker_customizer_pt cust = NULL;
        service_tracker_pt tracker = NULL;
        data->context = context;

        serviceTrackerCustomizer_create(data->event_admin_service->eventAdmin, eventAdmin_addingService, eventAdmin_addedService, eventAdmin_modifiedService, eventAdmin_removedService, &cust);
        serviceTracker_create(context, (char *) EVENT_HANDLER_SERVICE, cust, &tracker);

        data->tracker = tracker;

        serviceTracker_open(tracker);
        properties_pt properties = NULL;
        properties = properties_create();
        event_admin_service = activator->event_admin_service;
        bundleContext_registerService(context, (char *) EVENT_ADMIN_NAME, event_admin_service, properties, &activator->registration);
        logHelper_start(activator->loghelper);
    }
    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    struct activator * data =  userData;
    serviceRegistration_unregister(data->registration);
    serviceTracker_close(data->tracker);
    status = logHelper_stop(data->loghelper);
    logHelper_destroy(&data->loghelper);

    return status;
}


celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    //stop  struct activator *activator = userData;

    // free(activator);

    return status;
}


