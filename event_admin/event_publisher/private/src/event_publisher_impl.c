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
 * event_publisher_impl.c
 *
 * Created on: Jul 24, 2013
 * \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 * \copyright	Apache License, Version 2.0
 */

#include "event_publisher_impl.h"

static void *APR_THREAD_FUNC eventPublisherSendEventThread(apr_thread_t *thd, void *handle);

celix_status_t eventPublisherCreate(apr_pool_t *pool, bundle_context_pt context, event_publisher_pt *event_publisher) {
    celix_status_t status = CELIX_SUCCESS;
    *event_publisher = apr_palloc(pool, sizeof(**event_publisher));
    if (!*event_publisher) {
        status = CELIX_ENOMEM;
    } else {
        (*event_publisher)->event_admin_service = NULL;
        (*event_publisher)->pool = NULL;
        (*event_publisher)->eventAdminAdded = false;
        (*event_publisher)->running = false;
        (*event_publisher)->pool = pool;
        (*event_publisher)->context = context;

        logHelper_create(context, &(*event_publisher)->loghelper);
    }
    return status;
}

celix_status_t eventPublisherStart(event_publisher_pt *event_publisher) {
	(*event_publisher)->running = true;
    logHelper_start((*event_publisher)->loghelper);
	apr_thread_create(&(*event_publisher)->sender, NULL, eventPublisherSendEventThread, event_publisher, (*event_publisher)->pool);
	return CELIX_SUCCESS;
}

celix_status_t eventPublisherStop(event_publisher_pt *event_publisher) {
	(*event_publisher)->running = false;
	apr_status_t status;
	apr_thread_join(&status,(*event_publisher)->sender);
	logHelper_stop((*event_publisher)->loghelper);
	logHelper_destroy(&(*event_publisher)->loghelper);
	return CELIX_SUCCESS;
}

static void *APR_THREAD_FUNC eventPublisherSendEventThread(apr_thread_t *thd, void *handle) {
    event_publisher_pt *client = (event_publisher_pt *) handle;

    while ((*client)->running && (*client)->eventAdminAdded) {
        apr_sleep(1000000); // 1 sec.

        event_admin_service_pt *event_admin_service = &(*client)->event_admin_service;
        event_admin_pt event_admin = (*event_admin_service)->eventAdmin;
        if (event_admin_service != NULL) {
            event_pt event;
            properties_pt props = properties_create();
            properties_set(props, "This is a key", "this is a value");
            (*event_admin_service)->createEvent(event_admin, "log/error/eventpublishers/event", props, &event);
            (*event_admin_service)->postEvent(event_admin, event);
            (*event_admin_service)->sendEvent(event_admin, event);
        }
    }
	apr_thread_exit(thd, APR_SUCCESS);
	return NULL;
}

celix_status_t eventPublisherAddingService(void * handle, service_reference_pt ref, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	event_publisher_pt event_publisher = handle;
	status = bundleContext_getService(event_publisher->context, ref, service);
	return status;
}

celix_status_t eventPublisherAddedService(void * handle, service_reference_pt ref, void * service) {
    event_publisher_pt data = (event_publisher_pt) handle;
	logHelper_log(data->loghelper, OSGI_LOGSERVICE_DEBUG, "[PUB] Event admin added.");

    data->event_admin_service = (event_admin_service_pt) service;
    data->eventAdminAdded = true;
	return CELIX_SUCCESS;
}

celix_status_t eventPublisherModifiedService(void * handle, service_reference_pt ref, void * service) {
	event_publisher_pt data = (event_publisher_pt) handle;
	logHelper_log(data->loghelper, OSGI_LOGSERVICE_DEBUG, "[PUB] Event admin modified.");
	return CELIX_SUCCESS;
}

celix_status_t eventPublisherRemovedService(void * handle, service_reference_pt ref, void * service) {
	event_publisher_pt data = (event_publisher_pt) handle;
	logHelper_log(data->loghelper, OSGI_LOGSERVICE_DEBUG, "[PUB] Event admin removed.");

    data->event_admin_service = NULL;
    data->eventAdminAdded = false;
	return CELIX_SUCCESS;
}


