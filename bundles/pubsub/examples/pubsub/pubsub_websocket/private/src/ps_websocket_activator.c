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
 * ps_websocket_activator.c
 *
 *  \date       Jan 16, 2020
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <pubsub/subscriber.h>
#include <pubsub/publisher.h>

#include "celix_api.h"

#include "pubsub_websocket_private.h"


static const char * PUB_TOPICS[] = {
        "poi1",
        "poi2",
        NULL
};


#define SUB_NAME "poiCmd"
static const char * SUB_TOPICS[] = {
        "poiCmd",
        NULL
};

struct ps_websocketActivator {
    pubsub_info_t *pubsub;
    //Publisher vars
    array_list_t *trackerList;//List<service_tracker_pt>
    //Subscriber vars
    array_list_t *registrationList; //List<service_registration_pt>
    pubsub_subscriber_t *subsvc;
};

static int pubsub_start(struct ps_websocketActivator *act, celix_bundle_context_t *ctx) {
    const char *fwUUID = celix_bundleContext_getProperty(ctx,OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (fwUUID == NULL) {
        printf("PUBLISHER: Cannot retrieve fwUUID.\n");
        return CELIX_INVALID_BUNDLE_CONTEXT;
    }

    celix_bundle_t *bnd = celix_bundleContext_getBundle(ctx);
    long bundleId = celix_bundle_getId(bnd);

    act->pubsub = calloc(1, sizeof(*act->pubsub));

    //Publisher
    act->trackerList = celix_arrayList_create();
    act->pubsub->sender = publisher_create(act->trackerList, fwUUID, bundleId);

    int i;
    char filter[128];
    for (i = 0; PUB_TOPICS[i] != NULL; i++) {
        const char* topic = PUB_TOPICS[i];
        memset(filter,0,128);
#ifdef USE_SCOPE
        char *scope;
        asprintf(&scope, "my_scope_%d", i);
        snprintf(filter, 128, "(%s=%s)(%s=%s)", PUBSUB_PUBLISHER_TOPIC, topic, PUBSUB_PUBLISHER_SCOPE, scope);
        free(scope);
#else
        snprintf(filter, 128, "(%s=%s)", (char*) PUBSUB_PUBLISHER_TOPIC, topic);
#endif
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.callbackHandle = act->pubsub;
        opts.addWithProperties = publisher_publishSvcAdded;
        opts.removeWithProperties = publisher_publishSvcRemoved;
        opts.filter.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.filter.filter = filter;
        opts.filter.ignoreServiceLanguage = true;
        long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

        celix_arrayList_addLong(act->trackerList,trackerId);
    }

    publisher_start(act->pubsub->sender);

    //Subscriber
    act->registrationList = celix_arrayList_create();

    pubsub_subscriber_t *subsvc = calloc(1,sizeof(*subsvc));
    act->pubsub->receiver = subscriber_create(SUB_NAME);
    subsvc->handle = act->pubsub;
    subsvc->receive = pubsub_subscriber_recv;

    act->subsvc = subsvc;

    for (i = 0; SUB_TOPICS[i] != NULL; i++) {
        const char* topic = SUB_TOPICS[i];
        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, topic);
#ifdef USE_SCOPE
        char *scope;
        asprintf(&scope, "my_scope_%d", i);
        celix_properties_set(props, PUBSUB_SUBSCRIBER_SCOPE, scope);
        free(scope);
#endif
        service_registration_pt reg = NULL;
        bundleContext_registerService(ctx, PUBSUB_SUBSCRIBER_SERVICE_NAME, subsvc, props, &reg);
        arrayList_add(act->registrationList,reg);
    }

    subscriber_start((pubsub_receiver_t *) act->subsvc->handle);

    return 0;
}

static int pubsub_stop(struct ps_websocketActivator *act, celix_bundle_context_t *ctx) {
    int i;

    //Publishers
    for (i = 0; i < celix_arrayList_size(act->trackerList); i++) {
        long trkId = celix_arrayList_getLong(act->trackerList, i);
        celix_bundleContext_stopTracker(ctx, trkId);
    }
    publisher_stop(act->pubsub->sender);

    publisher_destroy(act->pubsub->sender);
    celix_arrayList_destroy(act->trackerList);

    //Subscriber
    for (i = 0; i < celix_arrayList_size(act->registrationList); i++) {
        service_registration_pt reg = celix_arrayList_get(act->registrationList, i);
        serviceRegistration_unregister(reg);
    }

    subscriber_stop((pubsub_receiver_t *) act->pubsub->receiver);

    act->subsvc->receive = NULL;
    subscriber_destroy((pubsub_receiver_t *) act->pubsub->receiver);
    act->subsvc->handle = NULL;
    free(act->subsvc);
    act->subsvc = NULL;

    celix_arrayList_destroy(act->registrationList);
    free(act->pubsub);

    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct ps_websocketActivator, pubsub_start, pubsub_stop)