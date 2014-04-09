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
 * event_admin.h
 *
 *  Created on: Jul 9, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EVENT_ADMIN_H_
#define EVENT_ADMIN_H_
/*#include "event.h"*/
#include "celix_errno.h"

#include "listener_hook_service.h"

#define EVENT_ADMIN_NAME "event_admin"
typedef struct event_admin *event_admin_pt;
typedef struct event_admin_service *event_admin_service_pt;
typedef struct event *event_pt;
/**
 * @desc service description for the event admin.
 * @param event_admin_pt eventAdmin. incomplete type for the event admin instance.
 * @param celix_status_t postEvent. Pointer to the post event function. For async sending
 * @param celix_status_t sendEvent. Pointer to the send event function. for Sync sending
 */
struct event_admin_service {
	event_admin_pt eventAdmin;
	celix_status_t (*postEvent)(event_admin_pt event_admin, event_pt event);
	celix_status_t (*sendEvent)(event_admin_pt event_admin, event_pt event);

	celix_status_t (*createEvent)(event_admin_pt event_admin, char *topic, properties_pt properties,  event_pt *event);
	celix_status_t (*containsProperty)(event_pt *event, char *property, bool *result);
	celix_status_t (*event_equals)(event_pt *event, event_pt *compare, bool *result);
	celix_status_t (*getProperty)( event_pt *event, char *propertyKey, char **propertyValue);
	celix_status_t (*getPropertyNames)(event_pt *event, array_list_pt *names);
	celix_status_t (*getTopic)( event_pt *event, char **topic);
	celix_status_t (*hashCode)(event_pt *event, int *hashCode);
	celix_status_t (*matches)( event_pt *event);
	celix_status_t (*toString)( event_pt *event, char *eventString);

};


#endif /* EVENT_ADMIN_H_ */
