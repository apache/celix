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

#include "event_admin.h"
#include "event_admin_impl.h"
#include "event_handler.h"
#include "hash_map.h"
#include "utils.h"


celix_status_t eventAdmin_create(apr_pool_t *pool, bundle_context_pt context, event_admin_pt *event_admin){
	celix_status_t status = CELIX_SUCCESS;
	*event_admin = apr_palloc(pool, sizeof(**event_admin));
	if(!*event_admin){
				status = CELIX_ENOMEM;
		}else {

			(*event_admin)->pool = pool;
			printf("pool evenadmin create: %p\n",(*event_admin)->pool);
			(*event_admin)->channels = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
			(*event_admin)->context =context;
			printf("event admin pointer: %p\n",(*event_admin));
			status = arrayList_create(&(*event_admin)->event_handlers);
		}
	return status;
}

celix_status_t eventAdmin_getEventHandlersByChannel(bundle_context_pt context, const char * serviceName, array_list_pt *eventHandlers) {
	celix_status_t status = CELIX_SUCCESS;
	status = bundleContext_getServiceReferences(context, serviceName,NULL, eventHandlers);
	return status;
}

celix_status_t eventAdmin_postEvent(event_admin_pt event_admin, event_pt event){
	celix_status_t status = CELIX_SUCCESS;
	array_list_pt event_handlers = NULL;
	char *topic;
		eventAdmin_getTopic(&event,&topic);
	eventAdmin_lockHandlersList(event_admin,topic);
	printf("post called\n");
	eventAdmin_releaseHandersList(event_admin,topic);
	return status;
}

celix_status_t eventAdmin_sendEvent(event_admin_pt event_admin, event_pt event){
	celix_status_t status = CELIX_SUCCESS;
	printf("send called\n");
	char *topic;
	eventAdmin_getTopic(&event,&topic);

	array_list_pt event_handlers;
	arrayList_create(&event_handlers);
	eventAdmin_lockHandlersList(event_admin,topic);
	eventAdmin_findHandlersByTopic(event_admin->channels,topic,event_handlers);
	array_list_iterator_pt handlers_itterator = arrayListIterator_create(event_handlers);
	while(arrayListIterator_hasNext(handlers_itterator)){
		void *handler = arrayListIterator_next(handlers_itterator);
		event_handler_service_pt event_handler_service = (event_handler_service_pt) handler;
		event_handler_service->handle_event(&event_handler_service->event_handler,event);
		printf("handler found \n");
	}
	eventAdmin_releaseHandersList(event_admin,topic);
	return status;
}



celix_status_t eventAdmin_findHandlersByTopic(hash_map_pt channels, char *topic , array_list_pt event_handlers){
	celix_status_t status = CELIX_SUCCESS;
	channel_t channel;
	char channel_name[strlen(topic)+1];
	strcpy(channel_name,topic);
	printf("Finding channel: %s\n",channel_name);
	//check if the channel exists
	if(hashMap_containsKey(channels,channel_name)==1) {
		printf("found channel \n");
		channel = hashMap_get(channels,channel_name);
		if(channel != NULL && channel->eventHandlers != NULL && hashMap_size(channel->eventHandlers)> 0 ){
			//itterate throught the handlers and add them to the array list for result.
			hash_map_iterator_pt hashmap_itterator =  hashMapIterator_create(channel->eventHandlers);
			while(hashMapIterator_hasNext(hashmap_itterator)){
				printf("add handler \n");
				event_handler_service_pt event_handler_service = NULL;
				event_handler_service = (event_handler_service_pt) hashMapIterator_nextValue(hashmap_itterator);
				arrayList_add(event_handlers,event_handler_service);
			}
		}
	}else {
		printf("no channel\n");
	}
	return status;
}

celix_status_t eventAdmin_createEventChannels(event_admin_pt *event_admin, char *topic, event_handler_service_pt event_handler_service){
	celix_status_t status = CELIX_SUCCESS;
	char channel_name[strlen(topic)+1];
	strcpy(channel_name,topic);
	printf("Creating channel: %s\n");
	//char complete_channel_name[strlen(topic)+1];
	channel_t channel;
	apr_pool_t *subPool= NULL;
	apr_pool_create(&subPool,(*event_admin)->pool);
	if(hashMap_containsKey((*event_admin)->channels,channel_name)==1) {
		channel = hashMap_get((*event_admin)->channels,channel_name);
		//hashMap_put(channel->eventHandlers,&event_handler_service,event_handler_service);
	}else {
		//create channel
		channel = apr_palloc(subPool, sizeof(*channel));
		if(!channel){
			status = CELIX_ENOMEM;
		}else {
			channel->topic = channel_name;
			channel->eventHandlers = hashMap_create(NULL,NULL,NULL,NULL);
			channel->channelLock = NULL;
			apr_thread_mutex_create(&channel->channelLock,APR_THREAD_MUTEX_NESTED, subPool);
			hashMap_put((*event_admin)->channels,channel_name,channel);
		}
		hashMap_put(channel->eventHandlers,&event_handler_service,event_handler_service);
	}

	return status;

	/*apr_pool_t *subPool= NULL;
	apr_pool_create(&subPool,(*event_admin)->pool);
	char delims[] = "/";
	char *result = NULL;
	result = strtok( topic, delims );
	if(result != NULL){
		strcpy(complete_channel_name, result);
	}
	while( result != NULL  && status == CELIX_SUCCESS) {
	    channel_t channel;
	    //check if the channel exists
	    if(hashMap_containsKey((*event_admin)->channels,complete_channel_name)==1) {
	    	channel = hashMap_get((*event_admin)->channels,complete_channel_name);
	    	//hashMap_put(channel->eventHandlers,&event_handler_service,event_handler_service);
	    }else {
	    	//create channel
	    	channel = apr_palloc(subPool, sizeof(*channel));
	    	if(!channel){
	    		status = CELIX_ENOMEM;
	    	}else {
	    		channel->topic = complete_channel_name;
				channel->eventHandlers = hashMap_create(NULL,NULL,NULL,NULL);
				channel->channelLock = NULL;
				apr_thread_mutex_create(&channel->channelLock,APR_THREAD_MUTEX_NESTED, subPool);
	    	}
	    }
	    result = strtok( NULL, delims );
	    if(result != NULL && status == CELIX_SUCCESS){
	    	strcat(complete_channel_name, "/");
	    	strcat(complete_channel_name, result);
	    }else if(status == CELIX_SUCCESS) {
	    	hashMap_put(channel->eventHandlers,&event_handler_service,event_handler_service);
	    }
	}*/

}

celix_status_t eventAdmin_lockHandlersList(event_admin_pt event_admin, char *topic ){
	celix_status_t status = CELIX_SUCCESS;
	char *channel_name;
	printf("lock: %s\n",topic);

	printf("LOCK!\n");
	return status;
}
celix_status_t eventAdmin_releaseHandersList(event_admin_pt event_admin, char *topic ){
	celix_status_t status = CELIX_SUCCESS;
	char *channel_name;
	printf("release: %s\n",topic);

	printf("UNLOCK\n");
	return status;
}

celix_status_t eventAdmin_addingService(void * handle, service_reference_pt ref, void **service) {

	celix_status_t status = CELIX_SUCCESS;
	event_admin_pt  event_admin = handle;
	status = bundleContext_getService(event_admin->context, ref, service);
  	return CELIX_SUCCESS;
}

celix_status_t eventAdmin_addedService(void * handle, service_reference_pt ref, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	event_admin_pt  event_admin = handle;
	event_handler_service_pt event_handler_service = NULL;
	event_handler_service = (event_handler_service_pt) service;
	service_registration_pt registration = NULL;
	serviceReference_getServiceRegistration(ref,&registration);
	properties_pt props = NULL;
	serviceRegistration_getProperties(registration, &props);
	char *topic = properties_get(props, (char*)EVENT_TOPIC);
	printf("Original TOPIC: %s\n,",topic);
	eventAdmin_createEventChannels(&event_admin,topic,event_handler_service);
	return CELIX_SUCCESS;
}

celix_status_t eventAdmin_modifiedService(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Event admin Modified\n");
	return CELIX_SUCCESS;
}

celix_status_t eventAdmin_removedService(void * handle, service_reference_pt ref, void * service) {
	struct data * data = (struct data *) handle;
	printf("Event admin Removed %p\n", service);
	return CELIX_SUCCESS;
}
