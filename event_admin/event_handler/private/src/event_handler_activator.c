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
 *  Created on: Jul 9, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>



#include "event_handler_impl.h"

static const char * const EVENT_HANDLER_NAME = "demo";
struct activator {
	apr_pool_t *pool;
	event_handler_service_pt event_handler_service;
	service_registration_pt registration;
	service_tracker_pt eventAdminTracker;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool = NULL;
	apr_pool_t *parentPool;
	struct activator *activator;
	status = bundleContext_getMemoryPool(context, &parentPool);
	if( status == CELIX_SUCCESS ) {
		if(apr_pool_create(&pool,parentPool) != APR_SUCCESS) {
			status = CELIX_BUNDLE_EXCEPTION;
		}else {
			activator = apr_palloc(pool,sizeof(*activator));
			activator->pool = pool;
			activator->registration = NULL;
			*userData = activator;
			event_handler_pt event_handler = NULL;
			event_handler_service_pt event_handler_service = NULL;
			status = eventHandlerCreate(activator->pool,context,&event_handler);
			if(status == CELIX_SUCCESS){
				event_handler_service = apr_palloc(activator->pool,sizeof(event_handler_service));
				if(!event_handler_service){
					status = CELIX_ENOMEM;
				}else {
					event_handler_service->event_handler = event_handler;
					event_handler_service->handle_event = eventHandlerHandleEvent;
				}
			}
			activator->event_handler_service = event_handler_service;
		}
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;
	properties_pt properties = NULL;
	properties = properties_create();
	properties_set(properties,(char*)EVENT_HANDLER_SERVICE,(char *)EVENT_HANDLER_NAME);
	properties_set(properties,(char*)EVENT_TOPIC, (char*) "log/error/eventpublishers/event");
	event_handler_service_pt event_handler_service = activator->event_handler_service;
	bundleContext_registerService(context, (char *) EVENT_HANDLER_SERVICE, event_handler_service, properties, &activator->registration);
	apr_pool_t *pool;
	status = bundleContext_getMemoryPool(context, &pool);
	if(status == CELIX_SUCCESS) {
		service_tracker_customizer_pt customizer = NULL;
		service_tracker_pt tracker = NULL;
		serviceTrackerCustomizer_create(activator->event_handler_service->event_handler,  eventHandlerAddingService, eventHandlerAddedService, eventHandlerModifiedService, eventHandlerRemovedService, &customizer);
		serviceTracker_create(context, (char *) EVENT_ADMIN_NAME, customizer,
									&tracker);
		activator->eventAdminTracker = tracker;
		serviceTracker_open(tracker);
		properties_pt properties = NULL;
		properties = properties_create();
	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	return status;
}


celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	return status;
}
