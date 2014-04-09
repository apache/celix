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

#include "event_publisher_impl.h"

celix_status_t eventPublisherCreate(apr_pool_t *pool, bundle_context_pt context, event_publisher_pt *event_publisher){
        celix_status_t status = CELIX_SUCCESS;
        *event_publisher = apr_palloc(pool, sizeof(**event_publisher));

        if(!*event_publisher){
                status = CELIX_ENOMEM;
        }else {
        	(*event_publisher)->event_admin_service = NULL;
        	(*event_publisher)->pool = NULL;
        	(*event_publisher)->eventAdminAdded = false;
        	(*event_publisher)->running = false;
        	(*event_publisher)->pool = pool;
        	(*event_publisher)->context = context;
        }
        return status;
}

celix_status_t eventPublisherStart(event_publisher_pt *event_publisher){
	(*event_publisher)->running = true;
	apr_thread_create(&(*event_publisher)->sender, NULL, eventPublisherSendEventThread, event_publisher, (*event_publisher)->pool);

	return CELIX_SUCCESS;
}

celix_status_t eventPublisherStop(event_publisher_pt *event_publisher){
	(*event_publisher)->running = false;
	apr_status_t status;
	apr_thread_join(&status,(*event_publisher)->sender);
	return CELIX_SUCCESS;
}

static void *APR_THREAD_FUNC eventPublisherSendEventThread(apr_thread_t *thd, void *handle) {
//celix_status_t trk_send(apr_thread_t *thd, void *handle) { //this function decleration is for debuggin purposes only.
	/*event_publisher_pt client =  handle;

	while (client->running && client->eventAdminAdded) {
		apr_sleep(1000000);
		event_admin_service_pt event_admin_service = client->event_admin_service ;
		event_admin_pt event_admin =   event_admin_service->eventAdmin;
		if (event_admin_service != NULL) {
			event_pt event;
			properties_pt props = properties_create();
			properties_set(props,"This is a key","this is a value");

			createEvent("log/error/eventpublishers/event/testChannel",props,client->pool,&event);
			event_admin_service->postEvent(event_admin,event);
			event_admin_service->sendEvent(event_admin,event);
		}

	}*/
	 event_publisher_pt *client = (event_publisher_pt *) handle;

		while ((*client)->running && (*client)->eventAdminAdded) {
				apr_sleep(1000000);
				event_admin_service_pt *event_admin_service = &(*client)->event_admin_service ;
				printf("event admin service pointer in publisher : %p\n",event_admin_service);
				event_admin_pt event_admin =   (*event_admin_service)->eventAdmin;
				if (event_admin_service != NULL) {

						event_pt event;
						properties_pt props = properties_create();
						properties_set(props,"This is a key","this is a value");
						(*event_admin_service)->createEvent(event_admin,"log/error/eventpublishers/event/testChannel",props,&event);
						(*event_admin_service)->postEvent(event_admin,event);
						(*event_admin_service)->sendEvent(event_admin,event);
				}

		}
	apr_thread_exit(thd, APR_SUCCESS);
	return NULL;
}

celix_status_t  eventPublisherAddingService(void * handle, service_reference_pt ref, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	event_publisher_pt event_publisher = handle;
	status = bundleContext_getService(event_publisher->context, ref, service);
	return status;
}

celix_status_t  eventPublisherAddedService(void * handle, service_reference_pt ref, void * service) {

	 event_publisher_pt  data =  handle;
	 event_admin_service_pt event_admin_service = NULL;
	 event_admin_service = (event_admin_service_pt ) service;
	 data->event_admin_service = event_admin_service;
	 data->eventAdminAdded = true;
	return CELIX_SUCCESS;
}

celix_status_t  eventPublisherModifiedService(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Event admin Modified\n");
	return CELIX_SUCCESS;
}

celix_status_t  eventPublisherRemovedService(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Event admin Removed %p\n", service);
	return CELIX_SUCCESS;
}


