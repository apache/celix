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
 * ps_sub_activator.c
 *
 *  \date       Sep 21, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <pubsub/subscriber.h>

#include "bundle_activator.h"

#include "pubsub_subscriber_private.h"

#define SUB_NAME "poi1;poi2"
static const char * SUB_TOPICS[] = {
        "poi1",
        "poi2",
        NULL
};

struct subscriberActivator {
    array_list_pt registrationList; //List<service_registration_pt>
    pubsub_subscriber_t *subsvc;
};

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **userData) {
    struct subscriberActivator * act = calloc(1,sizeof(struct subscriberActivator));
    *userData = act;
    arrayList_create(&(act->registrationList));
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
    struct subscriberActivator * act = (struct subscriberActivator *) userData;

    pubsub_subscriber_t *subsvc = calloc(1,sizeof(*subsvc));
    pubsub_receiver_t *sub = subscriber_create(SUB_NAME);
    subsvc->handle = sub;
    subsvc->receive = pubsub_subscriber_recv;

    act->subsvc = subsvc;

    int i;
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
        bundleContext_registerService(context, PUBSUB_SUBSCRIBER_SERVICE_NAME, subsvc, props, &reg);
        arrayList_add(act->registrationList,reg);
    }

    subscriber_start((pubsub_receiver_t *) act->subsvc->handle);

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
    struct subscriberActivator * act = (struct subscriberActivator *) userData;

    int i;
    for (i = 0; i < arrayList_size(act->registrationList); i++) {
        service_registration_pt reg = arrayList_get(act->registrationList, i);
        serviceRegistration_unregister(reg);

    }

    subscriber_stop((pubsub_receiver_t *) act->subsvc->handle);

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {

    struct subscriberActivator * act = (struct subscriberActivator *) userData;

    act->subsvc->receive = NULL;
    subscriber_destroy((pubsub_receiver_t *) act->subsvc->handle);
    act->subsvc->handle = NULL;
    free(act->subsvc);
    act->subsvc = NULL;

    arrayList_destroy(act->registrationList);
    free(act);

    return CELIX_SUCCESS;
}
