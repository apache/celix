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

static int tst_receive(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool *release);
static void sut_pubSet(void *handle, void *service);

struct activator {
	long pubTrkId;
  long subSvcId;
	pubsub_publisher_t* pubSvc;
  pubsub_subscriber_t subSvc;
  pthread_mutex_t mutex;
  unsigned int count;
  unsigned int msgId;
};

celix_status_t bnd_start(struct activator *act, celix_bundle_context_t *ctx) {

	char filter[512];
	snprintf(filter, 512, "(%s=%s)", PUBSUB_PUBLISHER_TOPIC, "ping3");
	celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
	opts.set = sut_pubSet;
	opts.callbackHandle = act;
	opts.filter.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
	opts.filter.filter = filter;
	act->pubTrkId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
	act->count = 0;
  act->msgId = 0;

  pthread_mutex_init(&act->mutex, NULL);

  celix_properties_t *props = celix_properties_create();
  celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, "pong2");
  act->subSvc.handle = act;
  act->subSvc.receive = tst_receive;
  act->subSvcId = celix_bundleContext_registerService(ctx, &act->subSvc, PUBSUB_SUBSCRIBER_SERVICE_NAME, props);


	return CELIX_SUCCESS;
}

celix_status_t bnd_stop(struct activator *act, celix_bundle_context_t *ctx) {
	celix_bundleContext_stopTracker(ctx, act->pubTrkId);
  celix_bundleContext_unregisterService(ctx, act->subSvcId);
  pthread_mutex_destroy(&act->mutex);
	return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bnd_start, bnd_stop);

static void sut_pubSet(void *handle, void *service) {
	struct activator* act = handle;
	pthread_mutex_lock(&act->mutex);
	act->pubSvc = service;
	pthread_mutex_unlock(&act->mutex);
}


static int tst_receive(void *handle, const char *msgType, unsigned int msgTypeId, void * voidMsg, const celix_properties_t *metadata, bool *release) {
  struct activator *act =handle;
  msg_t *msg = voidMsg;
  msg_t send_msg = *msg;
  pthread_mutex_lock(&act->mutex);

  if (act->pubSvc != NULL) {
    if (act->count == 0) {
      act->pubSvc->localMsgTypeIdForMsgType(act->pubSvc->handle, MSG_NAME, &act->msgId);
    }
    act->pubSvc->send(act->pubSvc->handle, act->msgId, &send_msg, (celix_properties_t *) metadata);
    act->count += 1;
  }
  pthread_mutex_unlock(&act->mutex);
  return CELIX_SUCCESS;
}
