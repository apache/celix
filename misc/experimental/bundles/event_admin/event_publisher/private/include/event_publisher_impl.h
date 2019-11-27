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
 * event_publisher.h
 *
 *  \date       Aug 9, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EVENT_PUBLISHER_H_
#define EVENT_PUBLISHER_H_
#include "celix_errno.h"
#include "event_admin.h"

#include "listener_hook_service.h"
#include "service_tracker.h"
#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_tracker.h"
#include "celix_service_listener.h"
#include "service_registration.h"
#include "event_constants.h"
#include "log_helper.h"
#include "log_service.h"


typedef struct event_publisher *event_publisher_pt;
struct event_publisher {
	event_admin_service_pt event_admin_service;
	bool running;
	bool eventAdminAdded;
	celix_thread_t sender;
	bundle_context_pt context;
	log_helper_t *loghelper;
};
/**
 * @desc create the event publisher
 * @param apr_pool_t *pool. the memory pool to store the publisher
 * @param bundle_context_pt context the bundle context
 * @param event_publisher_pt *event_publisher. The publisher to be made.
 */
celix_status_t eventPublisherCreate(bundle_context_pt context, event_publisher_pt *event_publisher);
/**
 * @desc start the event publisher. Starts the threads and trackers.
 * @param event_publisher_pt *event_publisher the publisher to start
 */
celix_status_t eventPublisherStart(event_publisher_pt *event_publisher);

/**
 * @desc functions used by the event admin tracker
 * @param void *handle, pointer to the event publisher
 * @param service_reference_pt ref. pointer to the reference of the event admin
 * @param void **service. pointer to the event admin service.
 */
celix_status_t eventPublisherAddingService(void * handle, service_reference_pt ref, void **service);
celix_status_t eventPublisherAddedService(void * handle, service_reference_pt ref, void * service);
celix_status_t eventPublisherModifiedService(void * handle, service_reference_pt ref, void * service);
celix_status_t eventPublisherRemovedService(void * handle, service_reference_pt ref, void * service);
/**
 * @desc stop the event publisher. stopping threads and tracker
 * @param event_publisher_pt *event_publisher. pointer to the publisher.
 */
celix_status_t eventPublisherStop(event_publisher_pt *event_publisher);

void *produceEvents(void *handle);
#endif /* EVENT_PUBLISHER_H_ */
