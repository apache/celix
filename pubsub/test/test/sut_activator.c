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

#include <stdio.h>
#include <stdlib.h>
#include <constants.h>

#include "bundle_activator.h"
#include "service_tracker.h"

#include "pubsub/subscriber.h"
#include "pubsub/publisher.h"

static int sut_receive(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, pubsub_multipart_callbacks_t *callbacks, bool *release);
static int sut_pubAdded(void *handle, service_reference_pt reference, void *service);
static int sut_pubRemoved(void *handle, service_reference_pt reference, void *service);


struct activator {
	pubsub_subscriber_t subSvc;
	service_registration_pt reg;

	service_tracker_pt tracker;

	pthread_mutex_t mutex;
	pubsub_publisher_t* pubSvc;
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	struct activator* act = malloc(sizeof(*act));
	*userData = act;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	struct activator* act = (struct activator*) userData;

	properties_pt props = properties_create();
	properties_set(props, "pubsub.topic", "ping");
	act->subSvc.handle = act;
	act->subSvc.receive = sut_receive;
	act->reg = NULL;
	bundleContext_registerService(context, PUBSUB_SUBSCRIBER_SERVICE_NAME, &act->subSvc, props, &act->reg);

	char filter[512];
	snprintf(filter, 512, "(&(%s=%s)(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_PUBLISHER_SERVICE_NAME, PUBSUB_PUBLISHER_TOPIC, "pong");

	service_tracker_customizer_pt customizer = NULL;
	serviceTrackerCustomizer_create(act, NULL, sut_pubAdded, NULL, sut_pubRemoved, &customizer);
	serviceTracker_createWithFilter(context, filter, customizer, &act->tracker);
	serviceTracker_open(act->tracker);

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt __attribute__((unused)) context) {
	struct activator* act = userData;
	serviceTracker_close(act->tracker);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt  __attribute__((unused)) context) {
	struct activator* act = userData;
	serviceTracker_destroy(act->tracker);
	return CELIX_SUCCESS;
}

static int sut_receive(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, pubsub_multipart_callbacks_t *callbacks, bool *release) {
	struct activator* act = handle;
	printf("Received msg '%s', sending back\n", msgType);
	pthread_mutex_lock(&act->mutex);
	if (act->pubSvc != NULL) {
		unsigned int sendId = 0;
		act->pubSvc->localMsgTypeIdForMsgType(act->pubSvc->handle, msgType, &sendId);
		act->pubSvc->send(act->pubSvc->handle, sendId, msg);
	}
	pthread_mutex_unlock(&act->mutex);
	return CELIX_SUCCESS;
}

static int sut_pubAdded(void *handle, service_reference_pt reference, void *service) {
	struct activator* act = handle;
	pthread_mutex_lock(&act->mutex);
	act->pubSvc = service;
	pthread_mutex_unlock(&act->mutex);
	return CELIX_SUCCESS;

}

static int sut_pubRemoved(void *handle, service_reference_pt reference, void *service) {
	struct activator* act = handle;
	pthread_mutex_lock(&act->mutex);
	if (act->pubSvc == service) {
		act->pubSvc = NULL;
	}
	pthread_mutex_unlock(&act->mutex);
	return CELIX_SUCCESS;
}

