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

#include "bundle_activator.h"
#include "service_tracker.h"

#include "pubsub/subscriber.h"
#include "pubsub/publisher.h"

#include "msg.h"
#include "sync.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <constants.h>


static int tst_receive(void *handle, const char *msgType, unsigned int msgTypeId, void *msg, pubsub_multipart_callbacks_t *callbacks, bool *release);
static int tst_pubAdded(void *handle, service_reference_pt reference, void *service);
static int tst_pubRemoved(void *handle, service_reference_pt reference, void *service);

struct activator {
	pubsub_subscriber_t subSvc{};
	service_registration_pt reg = nullptr;

	service_tracker_pt tracker = nullptr;

	pthread_mutex_t mutex{}; //protects below
	pubsub_publisher_t* pubSvc = nullptr;

    unsigned int syncId = 0;
    bool gotSync = 0;

    unsigned int msgId = 0;
    unsigned int count = 0;

    bool started = false;

};

static struct activator g_act; //global

celix_status_t bundleActivator_create(__attribute__((unused)) bundle_context_pt context, __attribute__((unused)) void **userData) {
    //memset(&g_act, 0, sizeof(g_act));
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(__attribute__((unused)) void * userData, bundle_context_pt context) {
	properties_pt props = properties_create();
	properties_set(props, "pubsub.topic", "pong");
	g_act.subSvc.handle = &g_act;
	g_act.subSvc.receive = tst_receive;
	bundleContext_registerService(context, PUBSUB_SUBSCRIBER_SERVICE_NAME, &g_act.subSvc, props, &g_act.reg);

    char filter[512];
    snprintf(filter, 512, "(&(%s=%s)(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_PUBLISHER_SERVICE_NAME, PUBSUB_PUBLISHER_TOPIC, "ping");

    service_tracker_customizer_pt customizer = NULL;
	serviceTrackerCustomizer_create(&g_act, NULL, tst_pubAdded, NULL, tst_pubRemoved, &customizer);
	serviceTracker_createWithFilter(context, filter, customizer, &g_act.tracker);
	serviceTracker_open(g_act.tracker);

    g_act.started = true;

	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(__attribute__((unused)) void * userData, bundle_context_pt __attribute__((unused)) context) {
	serviceTracker_close(g_act.tracker);
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(__attribute__((unused)) void * userData, bundle_context_pt  __attribute__((unused)) context) {
	serviceTracker_destroy(g_act.tracker);
	return CELIX_SUCCESS;
}

static int tst_receive(void *handle, const char * /*msgType*/, unsigned int msgTypeId, void * /*msg*/, pubsub_multipart_callbacks_t* /*callbacks*/, bool* /*release*/) {
	struct activator* act = static_cast<struct activator*>(handle);
    pthread_mutex_lock(&act->mutex);
    if (msgTypeId == act->syncId) {
        act->gotSync = true;
    } else if (msgTypeId == act->msgId) {
        act->count += 1;
    }
    pthread_mutex_unlock(&act->mutex);
	return CELIX_SUCCESS;
}

static int tst_pubAdded(void *handle, service_reference_pt /*reference*/, void *service) {
    struct activator* act = static_cast<struct activator*>(handle);
    pthread_mutex_lock(&act->mutex);
	act->pubSvc = static_cast<pubsub_publisher_t*>(service);
    act->pubSvc->localMsgTypeIdForMsgType(act->pubSvc->handle, MSG_NAME, &act->msgId);
    act->pubSvc->localMsgTypeIdForMsgType(act->pubSvc->handle, SYNC_NAME, &act->syncId);

	pthread_mutex_unlock(&act->mutex);
	return CELIX_SUCCESS;

}

static int tst_pubRemoved(void *handle, service_reference_pt /*reference*/, void *service) {
    struct activator* act = static_cast<struct activator*>(handle);
    pthread_mutex_lock(&act->mutex);
	if (act->pubSvc == service) {
		act->pubSvc = NULL;
	}
	pthread_mutex_unlock(&act->mutex);
	return CELIX_SUCCESS;
}


TEST_GROUP(PUBSUB_INT_GROUP)
{
	void setup() {
        constexpr int TRIES = 25;
        constexpr int TIMEOUT = 250000;

        pthread_mutex_lock(&g_act.mutex);
		CHECK_EQUAL(true, g_act.started);
        g_act.gotSync = false;
        pthread_mutex_unlock(&g_act.mutex);

        //check if publisher is available
        unsigned int syncId = 0;
        for (int i = 0; i < TRIES; ++i) {
            pthread_mutex_lock(&g_act.mutex);
            syncId = g_act.syncId;
            pthread_mutex_unlock(&g_act.mutex);
            if (syncId == 0) {
                printf("publisher still nullptr / sync msg type id is still 0, waiting for a while\n");
                usleep(TIMEOUT);
            } else {
                break;
            }
        }
        CHECK(syncId != 0);

        //check if message are returned
        sync_t syncMsg;
        bool gotSync = false;
        for (int i = 0; i < TRIES; ++i) {
            printf("Sending sync message. Try %d/%d\n", i+1, TRIES);
            pthread_mutex_lock(&g_act.mutex);
            g_act.pubSvc->send(g_act.pubSvc->handle, g_act.syncId, &syncMsg);
            pthread_mutex_unlock(&g_act.mutex);
            usleep(TIMEOUT);
            pthread_mutex_lock(&g_act.mutex);
            gotSync = g_act.gotSync;
            pthread_mutex_unlock(&g_act.mutex);
            if (gotSync) {
                break;
            }
        }
        if (!gotSync) {
            printf("No sync message received, bailing\n");
        }
        CHECK(gotSync);
	}

	void teardown() {
		//nop
	}
};

TEST(PUBSUB_INT_GROUP, sendRecvTest) {
    pthread_mutex_lock(&g_act.mutex);
    g_act.count = 0; //reset counter
    pthread_mutex_unlock(&g_act.mutex);

    constexpr int COUNT = 50;
    msg_t msg;
    for (int i = 0; i < COUNT; ++i) {
        msg.seqNr = i+1;
        printf("Sending test msg %d of %d\n", i+1, COUNT);
        pthread_mutex_lock(&g_act.mutex);
        g_act.pubSvc->send(g_act.pubSvc->handle, g_act.msgId, &msg);
        pthread_mutex_unlock(&g_act.mutex);
        usleep(100000);
    }

    constexpr int TRIES = 25;
    constexpr int TIMEOUT = 250000;
    for (int i = 0; i < TRIES; ++i) {
        pthread_mutex_lock(&g_act.mutex);
        int count = g_act.count;
        pthread_mutex_unlock(&g_act.mutex);
        if (count == COUNT) {
            break;
        } else {
            printf("Current count is %i, should be %i. Waiting a little\n", count, COUNT);
            usleep(TIMEOUT);
        }
    }

    pthread_mutex_lock(&g_act.mutex);
    int count = g_act.count;
    pthread_mutex_unlock(&g_act.mutex);
    CHECK_EQUAL(COUNT, count);
}
