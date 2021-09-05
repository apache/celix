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

#include <stdio.h>
#include <stdlib.h>
#include "celix_constants.h"
#include <unistd.h>

#include "celix_api.h"
#include "pubsub/api.h"
#include "msg.h"

static void sut_pubSet(void *handle, void *service);
static void* sut_sendThread(void *data);

struct activator {
	long pubTrkId;

	pthread_t sendThread;

	pthread_mutex_t mutex;
	bool running;
	pubsub_publisher_t* pubSvc;
};

celix_status_t bnd_start(struct activator *act, celix_bundle_context_t *ctx) {

	char filter[512];
    bool useNegativeScopeFilter = celix_bundleContext_getPropertyAsBool(ctx, "CELIX_PUBSUB_TEST_USE_NEGATIVE_SCOPE_FILTER", true);
    if (useNegativeScopeFilter) {
        snprintf(filter, 512, "(%s=%s)(!(scope=*))", PUBSUB_PUBLISHER_TOPIC, "ping");
    } else {
        snprintf(filter, 512, "(%s=%s)", PUBSUB_PUBLISHER_TOPIC, "ping");
    }
	celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
	opts.set = sut_pubSet;
	opts.callbackHandle = act;
	opts.filter.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
	opts.filter.filter = filter;
	act->pubTrkId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

	act->running = true;
	pthread_create(&act->sendThread, NULL, sut_sendThread, act);

	return CELIX_SUCCESS;
}

celix_status_t bnd_stop(struct activator *act, celix_bundle_context_t *ctx) {
	pthread_mutex_lock(&act->mutex);
	act->running = false;
	pthread_mutex_unlock(&act->mutex);
	pthread_join(act->sendThread, NULL);
	pthread_mutex_destroy(&act->mutex);

	celix_bundleContext_stopTracker(ctx, act->pubTrkId);
	return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bnd_start, bnd_stop);

static void sut_pubSet(void *handle, void *service) {
	struct activator* act = handle;
	pthread_mutex_lock(&act->mutex);
	act->pubSvc = service;
	pthread_mutex_unlock(&act->mutex);
}

static void* sut_sendThread(void *data) {
	struct activator *act = data;

	pthread_mutex_lock(&act->mutex);
	bool running = act->running;
	pthread_mutex_unlock(&act->mutex);

	unsigned int msgId = 0;
	msg_t msg;
	msg.seqNr = 1;

	while (running) {
		pthread_mutex_lock(&act->mutex);
		if (act->pubSvc != NULL) {
		    if (msgId == 0) {
		        act->pubSvc->localMsgTypeIdForMsgType(act->pubSvc->handle, MSG_NAME, &msgId);
		    }
            act->pubSvc->send(act->pubSvc->handle, msgId, &msg, NULL);
            if (msg.seqNr % 1000 == 0) {
                printf("Send %i messages\n", msg.seqNr);
            }

		    msg.seqNr += 1;

        }
        pthread_mutex_unlock(&act->mutex);

		usleep(10000);

		pthread_mutex_lock(&act->mutex);
		running = act->running;
		pthread_mutex_unlock(&act->mutex);
	}
    printf("Send %i messages\n", msg.seqNr);

    return NULL;
}
