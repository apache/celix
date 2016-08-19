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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <unistd.h>

#include "event_admin.h"
#include "event_admin_impl.h"
#include "event_handler.h"
#include "hash_map.h"
#include "utils.h"
#include "celix_log.h"


celix_status_t eventAdmin_create(bundle_context_pt context, event_admin_pt *event_admin) {
	celix_status_t status = CELIX_SUCCESS;
	*event_admin = calloc(1,sizeof(**event_admin));
	if (!*event_admin) {
        status = CELIX_ENOMEM;
    } else {
        (*event_admin)->channels = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
        (*event_admin)->context =context;
        (*event_admin)->eventListLock = calloc(1, sizeof(celix_thread_mutex_t));
        (*event_admin)->eventListProcessor = celix_thread_default;
        (*event_admin)->eventAdminRunning = false;
        linkedList_create(&(*event_admin)->eventList);//

        status = arrayList_create(&(*event_admin)->event_handlers);
    }
	return status;
}

celix_status_t eventAdmin_start(event_admin_pt *event_admin) {
    celix_status_t status = CELIX_SUCCESS;
    status = celixThread_create(&(*event_admin)->eventListProcessor, NULL, eventProcessor, &(*event_admin));
    celixThreadMutex_create((*event_admin)->eventListLock, NULL);
    (*event_admin)->eventAdminRunning = true;
    return status;
}

celix_status_t eventAdmin_stop(event_admin_pt *event_admin)
{
    (*event_admin)->eventAdminRunning = false;
    return CELIX_SUCCESS;
}

celix_status_t eventAdmin_destroy(event_admin_pt *event_admin) {
	celix_status_t status = CELIX_SUCCESS;
    arrayList_destroy((*event_admin)->event_handlers);
	free(*event_admin);
	return status;
}

celix_status_t eventAdmin_getEventHandlersByChannel(bundle_context_pt context, const char * serviceName, array_list_pt *eventHandlers) {
	celix_status_t status = CELIX_SUCCESS;
	//celix_status_t status = bundleContext_getServiceReferences(context, serviceName, NULL, eventHandlers);
	return status;
}

celix_status_t eventAdmin_postEvent(event_admin_pt event_admin, event_pt event) {
    bool added = false;
    while (event_admin->eventAdminRunning && added == false) {
        if (celixThreadMutex_tryLock(event_admin->eventListLock) == 0) {
            linkedList_addLast(event_admin->eventList, event);
            celixThreadMutex_unlock(event_admin->eventListLock);
            added = true;
        }

    }
    return CELIX_SUCCESS;
}

void *eventProcessor(void *handle) {
    celix_status_t status = CELIX_SUCCESS;
    event_admin_pt *eventAdminPt = handle;
    event_pt event = NULL;
    int waitcounter = 1;
    while ((*eventAdminPt)->eventAdminRunning) {
        if (celixThreadMutex_tryLock((*eventAdminPt)->eventListLock) == 0) {
            if (linkedList_isEmpty((*eventAdminPt)->eventList)) {
                if (waitcounter < 10) {
                    waitcounter++;
                } else {
                    waitcounter = 1;
                }
            } else {
                event = linkedList_removeFirst((*eventAdminPt)->eventList);
                waitcounter = 1;

            }
            celixThreadMutex_unlock((*eventAdminPt)->eventListLock);
        } else {
            if (waitcounter < 10) {
                waitcounter++;
            } else {
                waitcounter = 1;
            }
        }
        if (event != NULL) {
            status = processEvent((*eventAdminPt), event);
            if (status == CELIX_SUCCESS) {
                const char *topic;

                eventAdmin_getTopic(&event, &topic);
                eventAdmin_unlockTopic((*eventAdminPt), topic);
                event = NULL;


            } else {
                //processing event unsuccesfull since topic is locked. put event back into list for later processing.
                eventAdmin_postEvent(*eventAdminPt, event);
            }
        }

        usleep(waitcounter * 1000);
    }
    return CELIX_SUCCESS;
}

celix_status_t processEvent(event_admin_pt event_admin, event_pt event) {
    celix_status_t status = CELIX_SUCCESS;

    const char *topic;

    eventAdmin_getTopic(&event, &topic);

    array_list_pt event_handlers;
    arrayList_create(&event_handlers);
    //eventAdmin_lockHandlersList(event_admin, topic);
    status = eventAdmin_findHandlersByTopic(event_admin, topic, event_handlers);
    if (status == CELIX_SUCCESS) {
        array_list_iterator_pt handlers_iterator = arrayListIterator_create(event_handlers);
        while (arrayListIterator_hasNext(handlers_iterator)) {
            event_handler_service_pt event_handler_service = (event_handler_service_pt) arrayListIterator_next(
                    handlers_iterator);
            event_handler_service->handle_event(&event_handler_service->event_handler, event);
        }
    }
    //eventAdmin_releaseHandersList(event_admin, topic);
    return status;
}
celix_status_t eventAdmin_sendEvent(event_admin_pt event_admin, event_pt event) {
    return processEvent(event_admin, event);
}

celix_status_t eventAdmin_findHandlersByTopic(event_admin_pt event_admin, const char *topic,
											  array_list_pt event_handlers) {
	celix_status_t status = CELIX_SUCCESS;
	hash_map_pt channels = event_admin->channels;
    channel_t channel = hashMap_get(channels, topic);
	if (channel != NULL) {
        status = celixThreadMutex_tryLock(channel->channelLock);
        if (status == 0) {
            if (channel->eventHandlers != NULL && !hashMap_isEmpty(channel->eventHandlers)) {
                // iterate throught the handlers and add them to the array list for result.
                hash_map_iterator_pt hashmap_iterator = hashMapIterator_create(channel->eventHandlers);
                while (hashMapIterator_hasNext(hashmap_iterator)) {
                    arrayList_add(event_handlers,
                                  (event_handler_service_pt) hashMapIterator_nextValue(hashmap_iterator));
                }
            }
        } else {
            status = CELIX_ILLEGAL_STATE;
        }
	}
	return status;
}

celix_status_t eventAdmin_unlockTopic(event_admin_pt event_admin, const char *topic) {
    celix_status_t status = CELIX_SUCCESS;
    hash_map_pt channels = event_admin->channels;
    channel_t channel = hashMap_get(channels, topic);
    if (channel != NULL) {
        status = celixThreadMutex_unlock(channel->channelLock);
    }
    return status;
}

celix_status_t eventAdmin_createEventChannels(event_admin_pt *event_admin, const char *topic,
											  event_handler_service_pt event_handler_service) {
	celix_status_t status = CELIX_SUCCESS;
    channel_t channel = hashMap_get((*event_admin)->channels, topic);
	if (channel == NULL) {
		//create channel
		channel = calloc(1, sizeof(*channel));
		if (!channel) {
            status = CELIX_ENOMEM;
        } else {
            char *channel_name = strdup(topic);
			channel->topic = channel_name;
			channel->eventHandlers = hashMap_create(NULL,NULL,NULL,NULL);
            channel->channelLock = calloc(1, sizeof(celix_thread_mutex_t));
            celixThreadMutex_create(channel->channelLock, NULL);
			hashMap_put((*event_admin)->channels, channel_name, channel);
		}
    }
    if (channel) {
        hashMap_put(channel->eventHandlers, &event_handler_service, event_handler_service);
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
	const char *topic = NULL;
	serviceReference_getProperty(ref, (const char*)EVENT_TOPIC, &topic);
	eventAdmin_createEventChannels(&event_admin,topic,event_handler_service);
	return status;
}

celix_status_t eventAdmin_modifiedService(void * handle, service_reference_pt ref, void * service) {
/*	event_admin_pt event_admin = (event_admin_pt) handle;
	logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_ERROR, "Event admin Modified");*/
	return CELIX_SUCCESS;
}

celix_status_t eventAdmin_removedService(void * handle, service_reference_pt ref, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	//event_admin_pt  event_admin = handle;
	//hashMap_clear(event_admin->)
	/*event_admin_pt event_admin = (event_admin_pt) handle;
	logHelper_log(*event_admin->loghelper, OSGI_LOGSERVICE_ERROR, "Event admin Removed %p", service);
	printf("Event admin Removed %p", service);*/
	return status;
}
