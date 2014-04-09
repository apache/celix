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
 * event_admin_impl.c
 *
 *  Created on: Jul 24, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include "event_handler.h"

struct event_handler {
	apr_pool_t *pool;
	event_admin_service_pt event_admin_service;
	bundle_context_pt context;
};

celix_status_t eventHandlerCreate(apr_pool_t *pool, bundle_context_pt context,  event_handler_pt *event_handler){
	celix_status_t status = CELIX_SUCCESS;
	*event_handler = apr_palloc(pool, sizeof(**event_handler));
	if(!*event_handler){
			status = CELIX_ENOMEM;
	}else {
			(*event_handler)->pool = pool;
			(*event_handler)->event_admin_service = NULL;
			(*event_handler)->context = context;
	}
	return status;
}

celix_status_t eventHandlerHandleEvent(event_handler_pt *event_handler, event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	printf("handle event called in first handler\n");
	if(event != NULL){
		char *topic = NULL;
		status = (*event_handler)->event_admin_service->getTopic(&event,&topic);
		printf("topic of event: %s\n",topic);
		array_list_pt propertyNames;
		arrayList_create(&propertyNames);
		status = (*event_handler)->event_admin_service->getPropertyNames(&event,&propertyNames);
		array_list_iterator_pt propertyIter = arrayListIterator_create(propertyNames);
		while(arrayListIterator_hasNext(propertyIter)){
			char *key = arrayListIterator_next(propertyIter);
			char *value = NULL;
			(*event_handler)->event_admin_service->getProperty(&event,key,&value);
			//getProperty(&event,key,&value);
			printf("Key: %s value: %s\n",key,value);
		}
	}
	return status;
}


celix_status_t  eventHandlerAddingService(void * handle, service_reference_pt ref, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	printf("event handler adding service \n");
	event_handler_pt event_handler = handle;
	status = bundleContext_getService(event_handler->context, ref, service);
	return status;
}

celix_status_t  eventHandlerAddedService(void * handle, service_reference_pt ref, void * service) {
	printf("Event handler Added service \n");
	event_handler_pt  data =  handle;
	event_admin_service_pt event_admin_service = NULL;
	event_admin_service = (event_admin_service_pt ) service;
	data->event_admin_service = event_admin_service;
	return CELIX_SUCCESS;
}

celix_status_t  eventHandlerModifiedService(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Event admin Modified\n");
	return CELIX_SUCCESS;
}

celix_status_t  eventHandlerRemovedService(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Event admin Removed %p\n", service);
	return CELIX_SUCCESS;
}
