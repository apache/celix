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
#include <stdlib.h>

#include "event_admin.h"
#include "event_admin_impl.h"
#include "event_handler.h"
#include "hash_map.h"
#include "utils.h"
#include "celix_log.h"


celix_status_t eventAdmin_create(apr_pool_t *pool, bundle_context_pt context, event_admin_pt *event_admin){
	celix_status_t status = CELIX_SUCCESS;
	*event_admin = apr_palloc(pool, sizeof(**event_admin));
	if (!*event_admin) {
        status = CELIX_ENOMEM;
    } else {
        (*event_admin)->pool = pool;
        (*event_admin)->channels = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
        (*event_admin)->context =context;
        status = arrayList_create(&(*event_admin)->event_handlers);
    }
	return status;
}

celix_status_t eventAdmin_getEventHandlersByChannel(bundle_context_pt context, const char * serviceName, array_list_pt *eventHandlers) {
	celix_status_t status = bundleContext_getServiceReferences(context, serviceName, NULL, eventHandlers);
	return status;
}

celix_status_t eventAdmin_postEvent(event_admin_pt event_admin, event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	char *topic;

    eventAdmin_getTopic(&event, &topic);

	array_list_pt event_handlers;
	arrayList_create(&event_handlers);
	eventAdmin_lockHandlersList(event_admin, topic);
	eventAdmin_findHandlersByTopic(event_admin, topic, event_handlers);
    // TODO make this async!
	array_list_iterator_pt handlers_iterator = arrayListIterator_create(event_handlers);
	while (arrayListIterator_hasNext(handlers_iterator)) {
		event_handler_service_pt event_handler_service = (event_handler_service_pt) arrayListIterator_next(handlers_iterator);
		logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_INFO, "handler found (POST EVENT) for %s", topic);
		event_handler_service->handle_event(&event_handler_service->event_handler, event);
	}
	eventAdmin_releaseHandersList(event_admin, topic);
	return status;
}

celix_status_t eventAdmin_sendEvent(event_admin_pt event_admin, event_pt event) {
	celix_status_t status = CELIX_SUCCESS;

	char *topic;
	eventAdmin_getTopic(&event, &topic);

	array_list_pt event_handlers;
	arrayList_create(&event_handlers);
	eventAdmin_lockHandlersList(event_admin, topic);
	eventAdmin_findHandlersByTopic(event_admin, topic, event_handlers);
	array_list_iterator_pt handlers_iterator = arrayListIterator_create(event_handlers);
	while (arrayListIterator_hasNext(handlers_iterator)) {
		event_handler_service_pt event_handler_service = (event_handler_service_pt) arrayListIterator_next(handlers_iterator);
		logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_INFO, "handler found (SEND EVENT) for %s", topic);
		event_handler_service->handle_event(&event_handler_service->event_handler, event);
	}
	eventAdmin_releaseHandersList(event_admin, topic);
	return status;
}

celix_status_t eventAdmin_findHandlersByTopic(event_admin_pt event_admin, char *topic, array_list_pt event_handlers) {
	celix_status_t status = CELIX_SUCCESS;
	hash_map_pt channels = event_admin->channels;
    channel_t channel = hashMap_get(channels, topic);
	if (channel != NULL) {
		logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_INFO, "found channel: %s", topic);
		if (channel->eventHandlers != NULL && !hashMap_isEmpty(channel->eventHandlers)) {
			// iterate throught the handlers and add them to the array list for result.
			hash_map_iterator_pt hashmap_iterator =  hashMapIterator_create(channel->eventHandlers);
			while (hashMapIterator_hasNext(hashmap_iterator)) {
				arrayList_add(event_handlers, (event_handler_service_pt) hashMapIterator_nextValue(hashmap_iterator));
			}
		}
	} else {
		logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_WARNING, "no such channel: %s", topic);
	}
	return status;
}

celix_status_t eventAdmin_createEventChannels(event_admin_pt *event_admin, char *topic, event_handler_service_pt event_handler_service){
	celix_status_t status = CELIX_SUCCESS;
    channel_t channel = hashMap_get((*event_admin)->channels, topic);
	if (channel == NULL) {
		//create channel
		logHelper_log(*(*event_admin)->loghelper, OSGI_LOGSERVICE_INFO, "Creating channel: %s", topic);

        apr_pool_t *subPool = NULL;
        apr_pool_create(&subPool, (*event_admin)->pool);

		channel = apr_palloc(subPool, sizeof(*channel));
		if (!channel) {
            status = CELIX_ENOMEM;
        } else {
            char *channel_name = strdup(topic);
			channel->topic = channel_name;
			channel->eventHandlers = hashMap_create(NULL,NULL,NULL,NULL);
			channel->channelLock = NULL;
            apr_thread_mutex_create(&channel->channelLock, APR_THREAD_MUTEX_NESTED, subPool);
			hashMap_put((*event_admin)->channels, channel_name, channel);
		}
    }
    if (channel) {
        hashMap_put(channel->eventHandlers, &event_handler_service, event_handler_service);
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

celix_status_t eventAdmin_lockHandlersList(event_admin_pt event_admin, char *topic) {
	celix_status_t status = CELIX_SUCCESS;
    channel_t channel = hashMap_get(event_admin->channels, topic);
	if (channel != NULL) {
        // TODO verify this will never deadlock...
        apr_status_t status;
        do {
            status = apr_thread_mutex_trylock(channel->channelLock);
        } while (status != 0 && !APR_STATUS_IS_EBUSY(status));
        logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_DEBUG, "LOCK: %s!", topic);
    }
	return status;
}

celix_status_t eventAdmin_releaseHandersList(event_admin_pt event_admin, char *topic) {
	celix_status_t status = CELIX_SUCCESS;
    channel_t channel = hashMap_get(event_admin->channels, topic);
	if (channel != NULL) {
        // TODO check the result value...
        apr_thread_mutex_unlock(channel->channelLock);
        logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_DEBUG, "UNLOCK: %s!", topic);
    }
	return status;
}

celix_status_t eventAdmin_addingService(void * handle, service_reference_pt ref, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	event_admin_pt  event_admin = handle;
	status = bundleContext_getService(event_admin->context, ref, service);
  	return status;
}

celix_status_t eventAdmin_addedService(void * handle, service_reference_pt ref, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	event_admin_pt event_admin = handle;
	event_handler_service_pt event_handler_service = NULL;
	event_handler_service = (event_handler_service_pt) service;
	char *topic = NULL;
	serviceReference_getProperty(ref, (char*)EVENT_TOPIC, &topic);
	logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_DEBUG, "Original TOPIC: %s", topic);
	eventAdmin_createEventChannels(&event_admin,topic,event_handler_service);
	return status;
}

celix_status_t eventAdmin_modifiedService(void * handle, service_reference_pt ref, void * service) {
	event_admin_pt event_admin = (event_admin_pt) handle;
	logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_DEBUG, "Event admin Modified");
	return CELIX_SUCCESS;
}

celix_status_t eventAdmin_removedService(void * handle, service_reference_pt ref, void * service) {
	event_admin_pt event_admin = (event_admin_pt) handle;
	logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_DEBUG, "Event admin Removed %p", service);
	return CELIX_SUCCESS;
}
