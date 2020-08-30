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
 * event_admin_impl.c
 *
 *  \date       Jul 24, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include "event_handler.h"
#include "log_helper.h"
#include "log_service.h"

struct event_handler {
	event_admin_service_pt event_admin_service;
	bundle_context_pt context;
	log_helper_t *loghelper;

};

celix_status_t eventHandlerCreate(bundle_context_pt context, event_handler_pt *event_handler) {
	celix_status_t status = CELIX_SUCCESS;
    *event_handler = calloc(1, sizeof(**event_handler));
	if (!*event_handler) {
        status = CELIX_ENOMEM;
	} else {
        (*event_handler)->event_admin_service = NULL;
        (*event_handler)->context = context;

        if (celix_logHelper_create(context, &(*event_handler)->loghelper) == CELIX_SUCCESS) {
            //nop
        }
	}
	return status;
}

celix_status_t eventHandlerHandleEvent(event_handler_pt *event_handler, event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	if (event != NULL) {
        const char *topic = event->topic;
        //status = (*event_handler)->event_admin_service->getTopic(&event, &topic);
		celix_logHelper_log((*event_handler)->loghelper, CELIX_LOG_LEVEL_INFO, "[SUB] topic of event: %s.", topic);

		array_list_pt propertyNames;
		arrayList_create(&propertyNames);
        properties_pt properties = event->properties;
        if (hashMap_size(properties) > 0) {
            hash_map_iterator_pt iterator = hashMapIterator_create(properties);
            while (hashMapIterator_hasNext(iterator)) {
                hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
                char *key = hashMapEntry_getKey(entry);
                arrayList_add(propertyNames, key);
            }
        }
		array_list_iterator_pt propertyIter = arrayListIterator_create(propertyNames);
		while (arrayListIterator_hasNext(propertyIter)) {
            char *key = arrayListIterator_next(propertyIter);
            const char *value = NULL;
            value = properties_get((*event).properties, key);


			celix_logHelper_log((*event_handler)->loghelper, CELIX_LOG_LEVEL_INFO, "[SUB] Key: %s value: %s.", key, value);
		}
	}
	return status;
}


celix_status_t eventHandlerAddingService(void * handle, service_reference_pt ref, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	event_handler_pt event_handler = handle;
	status = bundleContext_getService(event_handler->context, ref, service);
	return status;
}

celix_status_t eventHandlerAddedService(void * handle, service_reference_pt ref, void * service) {
	event_handler_pt data = (event_handler_pt) handle;
	celix_logHelper_log(data->loghelper, OSGI_LOGSERVICE_DEBUG, "[SUB] Event admin added.");
	data->event_admin_service = (event_admin_service_pt) service;
	return CELIX_SUCCESS;
}

celix_status_t eventHandlerModifiedService(void * handle, service_reference_pt ref, void * service) {
	event_handler_pt data = (event_handler_pt) handle;
	celix_logHelper_log(data->loghelper, OSGI_LOGSERVICE_DEBUG, "[SUB] Event admin modified.");
	return CELIX_SUCCESS;
}

celix_status_t eventHandlerRemovedService(void * handle, service_reference_pt ref, void * service) {
	event_handler_pt data = (event_handler_pt) handle;
    celix_logHelper_log(data->loghelper, OSGI_LOGSERVICE_DEBUG, "[SUB] Event admin removed.");
	data->event_admin_service = NULL;
	return CELIX_SUCCESS;
}
