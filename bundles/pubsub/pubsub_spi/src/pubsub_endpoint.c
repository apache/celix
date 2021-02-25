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
 * endpoint_description.c
 *
 *  \date       25 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <celix_api.h>
#include <assert.h>

#include "celix_errno.h"
#include "celix_log.h"

#include "pubsub_endpoint.h"
#include "celix_constants.h"

#include "pubsub_utils.h"


static void pubsubEndpoint_setFields(celix_properties_t *psEp, const char* fwUUID, const char* scope, const char* topic, const char *pubsubType, const char *adminType, const char *serType, const char *protType, const celix_properties_t *topic_props);

static void pubsubEndpoint_setFields(celix_properties_t *ep, const char* fwUUID, const char* scope, const char* topic, const char *pubsubType, const char *adminType, const char *serType, const char *protType, const celix_properties_t *topic_props) {
    assert(ep != NULL);

    //copy topic properties
    if (topic_props != NULL) {
        const char *key = NULL;
        CELIX_PROPERTIES_FOR_EACH((celix_properties_t *) topic_props, key) {
            celix_properties_set(ep, key, celix_properties_get(topic_props, key, NULL));
        }
    }


    char endpointUuid[37];
    uuid_t endpointUid;
    uuid_generate(endpointUid);
    uuid_unparse(endpointUid, endpointUuid);
    celix_properties_set(ep, PUBSUB_ENDPOINT_UUID, endpointUuid);

    if (fwUUID != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_FRAMEWORK_UUID, fwUUID);
    }

    if (scope != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_TOPIC_SCOPE, scope);
    } else {
        celix_properties_set(ep, PUBSUB_ENDPOINT_TOPIC_SCOPE, PUBSUB_DEFAULT_ENDPOINT_SCOPE);
    }

    if (topic != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_TOPIC_NAME, topic);
    }

    if (pubsubType != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_TYPE, pubsubType);
    }

    if (adminType != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, adminType);
    }

    if (serType != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_SERIALIZER, serType);
    }

    if (protType != NULL) {
        celix_properties_set(ep, PUBSUB_ENDPOINT_PROTOCOL, protType);
    }
}

celix_properties_t* pubsubEndpoint_create(
        const char* fwUUID,
        const char* scope,
        const char* topic,
        const char* pubsubType,
        const char* adminType,
        const char *serType,
        const char *protType,
        celix_properties_t *topic_props) {
    celix_properties_t *ep = celix_properties_create();
    pubsubEndpoint_setFields(ep, fwUUID, scope, topic, pubsubType, adminType, serType, protType, topic_props);
    if (!pubsubEndpoint_isValid(ep, true, true)) {
        celix_properties_destroy(ep);
        ep = NULL;
    }
    return ep;
}


struct retrieve_topic_properties_data {
    celix_properties_t *props;
    const char *scope;
    const char *topic;
    bool isPublisher;
};

static void retrieveTopicProperties(void *handle, const celix_bundle_t *bnd) {
    struct retrieve_topic_properties_data *data = handle;
    data->props = pubsub_utils_getTopicProperties(bnd, data->scope, data->topic, data->isPublisher);
}

celix_properties_t* pubsubEndpoint_createFromSubscriberSvc(bundle_context_t* ctx, long bundleId, const celix_properties_t *svcProps) {
    celix_properties_t *ep = celix_properties_create();

    const char* fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    const char* scope = celix_properties_get(svcProps,  PUBSUB_SUBSCRIBER_SCOPE, NULL);
    const char* topic = celix_properties_get(svcProps,  PUBSUB_SUBSCRIBER_TOPIC, NULL);

    struct retrieve_topic_properties_data data;
    data.props = NULL;
    data.isPublisher = false;
    data.scope = scope;
    data.topic = topic;
    celix_bundleContext_useBundle(ctx, bundleId, &data, retrieveTopicProperties);

    const char *pubsubType = PUBSUB_SUBSCRIBER_ENDPOINT_TYPE;

    pubsubEndpoint_setFields(ep, fwUUID, scope, topic, pubsubType, NULL, NULL, NULL, data.props);

    if (data.props != NULL) {
        celix_properties_destroy(data.props); //Can be deleted since setFields invokes properties_copy
    }

    if (!pubsubEndpoint_isValid(ep, false, false)) {
        celix_properties_destroy(ep);
        ep = NULL;
    }
    return ep;
}


celix_properties_t* pubsubEndpoint_createFromPublisherTrackerInfo(bundle_context_t *ctx, long bundleId, const char *filter) {
    celix_properties_t *ep = celix_properties_create();

    const char* fwUUID=NULL;
    bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUUID);
    assert(fwUUID != NULL);

    char* topic = NULL;
    char* scopeFromFilter = NULL;
    pubsub_getPubSubInfoFromFilter(filter, &scopeFromFilter, &topic);
    const char *scope = scopeFromFilter;

    struct retrieve_topic_properties_data data;
    data.props = NULL;
    data.isPublisher = true;
    data.scope = scope;
    data.topic = topic;
    celix_bundleContext_useBundle(ctx, bundleId, &data, retrieveTopicProperties);

    pubsubEndpoint_setFields(ep, fwUUID, scope, topic, PUBSUB_PUBLISHER_ENDPOINT_TYPE, NULL, NULL, NULL, data.props);
    celix_properties_destroy(data.props); //safe to delete, properties are copied in pubsubEndpoint_setFields

    if (!pubsubEndpoint_isValid(ep, false, false)) {
        celix_properties_destroy(ep);
        ep = NULL;
    }

    free(topic);
    if (scope != NULL) {
        free(scopeFromFilter);
    }

    return ep;
}


bool pubsubEndpoint_equals(const celix_properties_t *psEp1, const celix_properties_t *psEp2) {
    if (psEp1 && psEp2) {
        const char *uuid1 = celix_properties_get(psEp1, PUBSUB_ENDPOINT_UUID, "entry1");
        const char *uuid2 = celix_properties_get(psEp1, PUBSUB_ENDPOINT_UUID, "entry1");
        return strcmp(uuid1, uuid2) == 0;
    } else {
        return false;
    }
}

char* pubsubEndpoint_createScopeTopicKey(const char* scope, const char* topic) {
    char *result = NULL;
    if (scope != NULL) {
        asprintf(&result, "%s:%s", scope, topic);
    } else {
        //NOTE scope == NULL, equal to scope="default"
        asprintf(&result, "%s", topic);
    }
    return result;
}

static bool checkProp(const celix_properties_t *props, const char *key) {
    const char *val = celix_properties_get(props, key, NULL);
    if (val == NULL) {
        fprintf(stderr, "[Error] Missing mandatory entry for endpoint. Missing key is '%s'\n", key);
    }
    return val != NULL;
}


bool pubsubEndpoint_isValid(const celix_properties_t *props, bool requireAdminType, bool requireSerializerType) {
    bool p1 = checkProp(props, PUBSUB_ENDPOINT_UUID);
    bool p2 = checkProp(props, PUBSUB_ENDPOINT_FRAMEWORK_UUID);
    bool p3 = checkProp(props, PUBSUB_ENDPOINT_TYPE);
    bool p4 = true;
    if (requireAdminType) {
        p4 = checkProp(props, PUBSUB_ENDPOINT_ADMIN_TYPE);
    }
    bool p5 = true;
    if (requireSerializerType) {
        p5 = checkProp(props, PUBSUB_ENDPOINT_SERIALIZER);
    }
    bool p6 = checkProp(props, PUBSUB_ENDPOINT_TOPIC_NAME);
    return p1 && p2 && p3 && p4 && p5 && p6;
}