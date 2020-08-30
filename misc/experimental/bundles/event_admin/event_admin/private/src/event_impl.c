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
 * event_impl.c
 *
 *  \date:      Aug 22, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "event_admin.h"
#include "event_admin_impl.h"
#include "event_constants.h"
#include "celix_errno.h"
#include "stddef.h"

celix_status_t eventAdmin_createEvent(event_admin_pt event_admin, const char *topic, properties_pt properties,
									  event_pt *event) {
	celix_status_t status = CELIX_SUCCESS;

    celix_logHelper_log(*event_admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "create event event admin pointer: %p",event_admin);


	*event = calloc(1, sizeof(**event));
	if(!*event){
	       status = CELIX_ENOMEM;
            celix_logHelper_log(*event_admin->loghelper, CELIX_LOG_LEVEL_ERROR, "No MEM");
	}else {
        celix_logHelper_log(*event_admin->loghelper, CELIX_LOG_LEVEL_INFO, "Event created : %s", topic);
		(*event)->topic = topic;
		(*event)->properties = properties;
		properties_set((*event)->properties, (char *)EVENT_TOPIC, topic);
	}
	return status;
}

celix_status_t eventAdmin_containsProperty( event_pt *event, char *property, bool *result){
	celix_status_t status = CELIX_SUCCESS;
	if((*event)==NULL || property == NULL){
		status = CELIX_BUNDLE_EXCEPTION;
	}else {
		const char *propertyValue = properties_get((*event)->properties, property);
		if(propertyValue == NULL){
			(*result)= false;
		}else {
			(*result) = true;
		}
	}
	return status;
}

celix_status_t eventAdmin_event_equals( event_pt *event, event_pt *compare, bool *result){
	celix_status_t status = CELIX_SUCCESS;
	if(event == compare){
		(*result) = true;
	}else {
		int sizeofEvent = hashMap_size((*event)->properties);
		int sizeofCompare = hashMap_size((*compare)->properties);
		if(sizeofEvent == sizeofCompare){
			(*result) = true;
		}else {

		}
	}
	return status;
}

celix_status_t eventAdmin_getProperty(event_pt *event, char *propertyKey, const char **propertyValue) {
	celix_status_t status = CELIX_SUCCESS;
	*propertyValue = properties_get((*event)->properties,propertyKey);

	return status;
}

celix_status_t eventAdmin_getPropertyNames( event_pt *event, array_list_pt *names){
	celix_status_t status = CELIX_SUCCESS;
	properties_pt properties =  (*event)->properties;
	if (hashMap_size(properties) > 0) {
		hash_map_iterator_pt iterator = hashMapIterator_create(properties);
		while (hashMapIterator_hasNext(iterator)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
			char * key =hashMapEntry_getKey(entry);
			arrayList_add((*names),key);
		}
	}
	return status;
}

celix_status_t eventAdmin_getTopic(event_pt *event, const char **topic) {
	celix_status_t status = CELIX_SUCCESS;
	const char *value;
	value = properties_get((*event)->properties,(char*) EVENT_TOPIC);
	*topic = value;

	return status;
}

celix_status_t eventAdmin_hashCode( event_pt *event, int *hashCode){
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t eventAdmin_matches( event_pt *event){
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t eventAdmin_toString( event_pt *event, char *eventString){
	celix_status_t status = CELIX_SUCCESS;
	return status;
}





