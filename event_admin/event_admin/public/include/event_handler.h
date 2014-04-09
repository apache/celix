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
 * event_handler.h
 *
 *  Created on: Jul 22, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EVENT_HANDLER_H_
#define EVENT_HANDLER_H_
#include "event_admin.h"
#include "properties.h"
static const char * const EVENT_HANDLER_SERVICE = "service.event.handler";

typedef struct event_handler_service *event_handler_service_pt;
typedef struct event_handler *event_handler_pt; //ADT
/**
 * @desc description of the event handler service
 * @param event_handler_pt event_handler incomplete type pointer for the event_handler instance
 * @param celix_status_t handle event. pointer to the handle event method.
 */
struct event_handler_service {
		event_handler_pt event_handler;
        celix_status_t (*handle_event)(event_handler_pt *event_handler, event_pt event);
};

#endif /* EVENT_HANDLER_H_ */
