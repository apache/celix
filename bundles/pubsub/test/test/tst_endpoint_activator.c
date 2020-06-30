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
#include <string.h>
#include <unistd.h>

#include "celix_api.h"
#include "pubsub/api.h"

#include "msg.h"
#include "receive_count_service.h"


static int tst_receive(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool *release);
static size_t tst_count(void *handle);

struct activator {
    pubsub_subscriber_t subSvc;
    long subSvcId;

    celix_receive_count_service_t countSvc;
    long countSvcId;

    pthread_mutex_t mutex;
    unsigned int count;
};

celix_status_t bnd_start(struct activator *act, celix_bundle_context_t *ctx) {
    pthread_mutex_init(&act->mutex, NULL);

    {
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, "ping2");
        act->subSvc.handle = act;
        act->subSvc.receive = tst_receive;
        act->subSvcId = celix_bundleContext_registerService(ctx, &act->subSvc, PUBSUB_SUBSCRIBER_SERVICE_NAME, props);
    }


    {
        act->countSvc.handle = act;
        act->countSvc.receiveCount = tst_count;
        act->countSvcId = celix_bundleContext_registerService(ctx, &act->countSvc, CELIX_RECEIVE_COUNT_SERVICE_NAME, NULL);
    }


    return CELIX_SUCCESS;
}

celix_status_t bnd_stop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->subSvcId);
    celix_bundleContext_unregisterService(ctx, act->countSvcId);
    pthread_mutex_destroy(&act->mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bnd_start, bnd_stop) ;


static int tst_receive(void *handle, const char * msgType __attribute__((unused)), unsigned int msgTypeId  __attribute__((unused)), void * voidMsg, const celix_properties_t *metadata  __attribute__((unused)), bool *release  __attribute__((unused))) {
    struct activator *act = handle;

    msg_t *msg = voidMsg;
    static uint32_t prevSeqNr = 0;
    long delta = msg->seqNr - prevSeqNr;
    if (delta != 1 && msg->seqNr > 1 && prevSeqNr < msg->seqNr) {
       fprintf(stderr, "Warning: missing messages. seq jumped from %i to %i\n", prevSeqNr, msg->seqNr);
    }
    prevSeqNr = msg->seqNr;

    pthread_mutex_lock(&act->mutex);
    act->count += 1;
    pthread_mutex_unlock(&act->mutex);
    return CELIX_SUCCESS;
}

static size_t tst_count(void *handle) {
    struct activator *act = handle;
    size_t count;
    pthread_mutex_lock(&act->mutex);
    count = act->count;
    pthread_mutex_unlock(&act->mutex);
    return count;
}
