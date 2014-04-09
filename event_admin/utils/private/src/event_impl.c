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
 * event_impl.c
 *
 *  \Created on: Aug 22, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 *
 */


#include "event.h"
#include "event_constants.h"
#include "celix_errno.h"
#include "stddef.h"
#include <apr.h>
#include <apr_pools.h>
struct event {
	char *topic;
	properties_pt properties;
};

celix_status_t createEvent(char *topic, properties_pt properties, apr_pool_t *pool, event_pt *event){
	celix_status_t status = CELIX_SUCCESS;

	*event= apr_palloc(pool,sizeof(**event));
	if(!*event){
	       status = CELIX_ENOMEM;
	}else {
		(*event)->topic = topic;
		(*event)->properties = properties;
		properties_set((*event)->properties, (char *)EVENT_TOPIC, topic);
	}
	return status;
}

celix_status_t containsProperty(event_pt *event, char *property, bool *result){
	celix_status_t status = CELIX_SUCCESS;
	if((*event)==NULL || property == NULL){
		status = CELIX_BUNDLE_EXCEPTION;
	}else {
		char * propertyValue = properties_get((*event)->properties,property);
		if(propertyValue == NULL){
			(*result)= false;
		}else {
			(*result) = true;
		}
	}
	return status;
}

celix_status_t event_equals(event_pt *event, event_pt *compare, bool *result){
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

celix_status_t getProperty(event_pt *event, char *propertyKey,  char **propertyValue){
	celix_status_t status = CELIX_SUCCESS;
	*propertyValue = properties_get((*event)->properties,propertyKey);

	return status;
}

celix_status_t getPropertyNames(event_pt *event, array_list_pt *names){
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

celix_status_t getTopic(event_pt *event, char **topic){
	celix_status_t status = CELIX_SUCCESS;
	char *value;
	value = properties_get((*event)->properties,(char*) EVENT_TOPIC);
	*topic = value;

	return status;
}

celix_status_t hashCode(event_pt *event, int *hashCode){
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t matches(event_pt *event){
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t toString(event_pt *event, char *eventString){
	celix_status_t status = CELIX_SUCCESS;
	return status;
}





