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
 * event_handler_impl.h
 *
 *  Created on: Aug 20, 2013
  *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EVENT_HANDLER_IMPL_H_
#define EVENT_HANDLER_IMPL_H_
#include "event_admin.h"
#include "event_constants.h"
#include "event_handler.h"

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_tracker.h"
#include "service_listener.h"
#include "service_registration.h"
#include "listener_hook_service.h"
#include "event_constants.h"
/**
 * @desc handle the event send to the event handler
 * @param event_handler_pt *instance the instance of the event handlers
 * @param event_pt event. the event to be handled.
 */
celix_status_t eventHandlerHandleEvent(event_handler_pt *instance, event_pt event) ;

/**
 * @desc create the event handler
 * @param apr_pool_t *pool the apr pool to contain the handler
 * @param event_handler_pt *event_handler. the event handler to be made.
 */
celix_status_t eventHandlerCreate(apr_pool_t *pool, bundle_context_pt context,  event_handler_pt *event_handler);
celix_status_t  eventHandlerRemovedService(void * handle, service_reference_pt ref, void * service) ;
celix_status_t  eventHandlerModifiedService(void * handle, service_reference_pt ref, void * service) ;
celix_status_t  eventHandlerAddedService(void * handle, service_reference_pt ref, void * service) ;
celix_status_t  eventHandlerAddingService(void * handle, service_reference_pt ref, void **service) ;
#endif /* EVENT_HANDLER_IMPL_H_ */
