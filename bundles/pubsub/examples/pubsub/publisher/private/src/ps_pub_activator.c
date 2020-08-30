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

#include <sys/cdefs.h>
#include <stdlib.h>
#include <string.h>

#include "celix_api.h"

#include "pubsub/publisher.h"
#include "pubsub_publisher_private.h"

static const char * PUB_TOPICS[] = {
        "poi1",
        "poi2",
        NULL
};

struct publisherActivator {
    pubsub_sender_t *client;
    array_list_pt trackerList;//List<service_tracker_pt>
};

static int pub_start(struct publisherActivator *act, celix_bundle_context_t *ctx) {
    const char *fwUUID = celix_bundleContext_getProperty(ctx,OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (fwUUID == NULL) {
        printf("PUBLISHER: Cannot retrieve fwUUID.\n");
        return CELIX_INVALID_BUNDLE_CONTEXT;
    }


    celix_bundle_t *bnd = celix_bundleContext_getBundle(ctx);
    long bundleId = celix_bundle_getId(bnd);

    act->trackerList = celix_arrayList_create();
    act->client = publisher_create(act->trackerList, fwUUID, bundleId);

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
        snprintf(filter, 128, "(&(%s=%s)(!(%s=*)))", (char*) PUBSUB_PUBLISHER_TOPIC, topic, PUBSUB_PUBLISHER_SCOPE);
#endif
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.callbackHandle = act->client;
        opts.addWithProperties = publisher_publishSvcAdded;
        opts.removeWithProperties = publisher_publishSvcRemoved;
        opts.filter.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.filter.filter = filter;
        opts.filter.ignoreServiceLanguage = true;
        long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

        celix_arrayList_addLong(act->trackerList,trackerId);
    }

    publisher_start(act->client);

    return 0;
}

static int pub_stop(struct publisherActivator *act, celix_bundle_context_t *ctx) {
    for (int i = 0; i < arrayList_size(act->trackerList); i++) {
        long trkId = celix_arrayList_getLong(act->trackerList, i);
        celix_bundleContext_stopTracker(ctx, trkId);
    }
    publisher_stop(act->client);

    publisher_destroy(act->client);
    celix_arrayList_destroy(act->trackerList);

    return 0;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct publisherActivator, pub_start, pub_stop)