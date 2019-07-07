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
#include <string.h>
#include <unistd.h>

#include "celix_api.h"
#include "pubsub/api.h"

#include "msg.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <constants.h>

extern "C" {

static int tst_receive(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, bool *release);

struct activator {
    pubsub_subscriber_t subSvc;
    long subSvcId;

    pthread_mutex_t mutex;
    unsigned int count = 0;
};

static struct activator *g_act = NULL; //global

celix_status_t bnd_start(struct activator *act, celix_bundle_context_t *ctx) {
    pthread_mutex_init(&act->mutex, NULL);

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, "ping2");
    act->subSvc.handle = act;
    act->subSvc.receive = tst_receive;
    act->subSvcId = celix_bundleContext_registerService(ctx, &act->subSvc, PUBSUB_SUBSCRIBER_SERVICE_NAME, props);

    g_act = act;

    return CELIX_SUCCESS;
}

celix_status_t bnd_stop(struct activator *act, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, act->subSvcId);
    pthread_mutex_destroy(&act->mutex);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct activator, bnd_start, bnd_stop) ;


static int tst_receive(void *handle, const char * /*msgType*/, unsigned int /*msgTypeId*/, void * voidMsg, bool */*release*/) {
    struct activator *act = static_cast<struct activator *>(handle);

    msg_t *msg = static_cast<msg_t*>(voidMsg);
    static int prevSeqNr = 0;
    int delta = msg->seqNr - prevSeqNr;
    if (delta != 1) {
        fprintf(stderr, "Warning: missing messages. seq jumped from %i to %i\n", prevSeqNr, msg->seqNr);
    }
    prevSeqNr = msg->seqNr;

    pthread_mutex_lock(&act->mutex);
    act->count += 1;
    pthread_mutex_unlock(&act->mutex);
    return CELIX_SUCCESS;
}

} //end extern C

TEST_GROUP(PUBSUB_INT_GROUP)
{
	void setup() {
	    //nop
	}

	void teardown() {
		//nop
	}
};

TEST(PUBSUB_INT_GROUP, recvTest) {
    constexpr int TRIES = 25;
    constexpr int TIMEOUT = 250000;
    constexpr int MSG_COUNT = 100;

    int count = 0;

    for (int i = 0; i < TRIES; ++i) {
        pthread_mutex_lock(&g_act->mutex);
        count = g_act->count;
        pthread_mutex_unlock(&g_act->mutex);
        printf("Current msg count is %i, waiting for at least %i\n", count, MSG_COUNT);
        if (count >= MSG_COUNT) {
            break;
        }
        usleep(TIMEOUT);
    }
    CHECK(count >= MSG_COUNT);

}
