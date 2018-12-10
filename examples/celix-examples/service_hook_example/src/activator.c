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
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "bundle_activator.h"
#include "service_tracker_customizer.h"
#include "service_tracker.h"
#include "bundle_context.h"
#include "listener_hook_service.h"
#include "service_registry.h"

struct userData {
    service_registration_pt hookReg;
    service_tracker_pt trackerBefore;
    service_tracker_pt trackerAfter;
    listener_hook_service_pt hookService; 
};


celix_status_t tracker_added(void*hook, celix_array_list_t *listeners) {
    for(unsigned int i = 0; i < arrayList_size(listeners); i++) {
        listener_hook_info_pt info = arrayList_get(listeners, i);
        printf("Added tracker for service %s\n", info->filter);
    }

    return CELIX_SUCCESS;
}

celix_status_t tracker_removed(void*hook, celix_array_list_t *listeners) {
    for(unsigned int i = 0; i < arrayList_size(listeners); i++) {
        listener_hook_info_pt info = arrayList_get(listeners, i);
        printf("Removed tracker for service %s\n", info->filter);
    }

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
    *userData = malloc(sizeof(struct userData));
    if (*userData != NULL) {
       
	} else {
		status = CELIX_START_ERROR;
	}
	return status;
}

celix_status_t bundleActivator_start(void * handle, celix_bundle_context_t *context) {
	printf("Starting hook example bundle\n");
    struct userData *userData = (struct userData*)handle;   
    
    userData->trackerBefore = 0;
    serviceTracker_create(context, "MY_SERVICE_BEFORE_REGISTERING_HOOK", NULL, &userData->trackerBefore);
    serviceTracker_open(userData->trackerBefore);
    
    listener_hook_service_pt hookService = calloc(1, sizeof(*hookService));
    hookService->handle = userData;
    hookService->added = tracker_added;
    hookService->removed = tracker_removed;

    userData->hookService = hookService;
    userData->hookReg = NULL;

    printf("Registering hook service\n");
    bundleContext_registerService(context, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, hookService, NULL, &userData->hookReg);

    printf("Unregistering hook service\n");
    serviceRegistration_unregister(userData->hookReg);

    printf("Re-Registering hook service\n");
    userData->hookReg = NULL;
    bundleContext_registerService(context, OSGI_FRAMEWORK_LISTENER_HOOK_SERVICE_NAME, hookService, NULL, &userData->hookReg);

    userData->trackerAfter = 0;
    serviceTracker_create(context, "MY_SERVICE_AFTER_REGISTERING_HOOK", NULL, &userData->trackerAfter);
    serviceTracker_open(userData->trackerAfter);
  
    sleep(1);
    printf("Closing tracker\n");
    serviceTracker_close(userData->trackerAfter);
    printf("Reopening tracker\n");
    serviceTracker_open(userData->trackerAfter);
    
    sleep(1);
    printf("Closing tracker\n");
    serviceTracker_close(userData->trackerAfter);
    printf("Reopening tracker\n");
    serviceTracker_open(userData->trackerAfter);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * handle, celix_bundle_context_t *context) {
	printf("Stopping hook example bundle\n");
    struct userData *userData = (struct userData*)handle;   

    serviceTracker_close(userData->trackerAfter);
    serviceTracker_close(userData->trackerBefore);
    serviceTracker_destroy(userData->trackerAfter);
    serviceTracker_destroy(userData->trackerBefore);

    serviceRegistration_unregister(userData->hookReg);
    free(userData->hookService);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * handle, celix_bundle_context_t *context) {
    free(handle);
	return CELIX_SUCCESS;
}
