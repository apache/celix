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

#include <string.h>
#include <limits.h>
#include <pubsub_endpoint.h>
#include <pubsub_serializer.h>
#include <pubsub_protocol.h>
#include <celix_api.h>

#include "service_reference.h"

#include "pubsub_admin.h"

#include "pubsub_utils.h"
#include "celix_constants.h"

static double getPSAScore(const char *requested_admin, const char *request_qos, const char *adminType, double sampleScore, double controlScore, double defaultScore) {
    double score;
    if (requested_admin != NULL && strncmp(requested_admin, adminType, strlen(adminType) + 1) == 0) {
        /* We got precise specification on the pubsub_admin we want */
        //Full match
        score = PUBSUB_ADMIN_FULL_MATCH_SCORE;
    } else if (requested_admin != NULL) {
        //admin type requested, but no match -> do not select this psa
        score = PUBSUB_ADMIN_NO_MATCH_SCORE;
    } else if (request_qos != NULL && strncmp(request_qos, PUBSUB_UTILS_QOS_TYPE_SAMPLE, strlen(PUBSUB_UTILS_QOS_TYPE_SAMPLE) + 1) == 0) {
        //qos match
        score = sampleScore;
    } else if (request_qos != NULL && strncmp(request_qos, PUBSUB_UTILS_QOS_TYPE_CONTROL, strlen(PUBSUB_UTILS_QOS_TYPE_CONTROL) + 1) == 0) {
        //qos match
        score = controlScore;
    } else if (request_qos != NULL) {
        //note unsupported qos -> defaultScore
        score = defaultScore;
    } else {
        //default match
        score = defaultScore;
    }
    return score;
}

static long getPSASerializer(celix_bundle_context_t *ctx, const char *requested_serializer) {
    long svcId = -1L;

    if (requested_serializer != NULL) {
        char filter[512];
        snprintf(filter, 512, "(%s=%s)", PUBSUB_SERIALIZER_TYPE_KEY, requested_serializer);

        celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
        opts.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
        opts.filter = filter;

        svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
        if (svcId == -1) {
            fprintf(stderr, "Warning cannot find serializer with requested serializer type '%s'\n", requested_serializer);
        }
    } else {
        celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
        opts.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
        opts.ignoreServiceLanguage = true;

        //note findService will automatically return the highest ranking service id
        svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    }

    return svcId;
}

struct psa_protocol_selection_data {
    const char *requested_protocol;
    long matchingSvcId;
};

void psa_protocol_selection_callback(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
    struct psa_protocol_selection_data *data = handle;
    const char *serType = celix_properties_get(props, PUBSUB_PROTOCOL_TYPE_KEY, NULL);
    if (serType == NULL) {
        fprintf(stderr, "Warning found protocol without mandatory protocol type key (%s)\n", PUBSUB_PROTOCOL_TYPE_KEY);
    } else {
        if (strncmp(data->requested_protocol, serType, 1024 * 1024) == 0) {
            data->matchingSvcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
        }
    }
}

static long getPSAProtocol(celix_bundle_context_t *ctx, const char *requested_protocol) {
    long svcId;

    if (requested_protocol != NULL) {
        struct psa_protocol_selection_data data;
        data.requested_protocol = requested_protocol;
        data.matchingSvcId = -1L;

        celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
        opts.filter.serviceName = PUBSUB_PROTOCOL_SERVICE_NAME;
        opts.filter.ignoreServiceLanguage = true;
        opts.callbackHandle = &data;
        opts.useWithProperties = psa_protocol_selection_callback;
        celix_bundleContext_useServicesWithOptions(ctx, &opts);
        svcId = data.matchingSvcId;
    } else {
        celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
        opts.serviceName = PUBSUB_PROTOCOL_SERVICE_NAME;
        opts.ignoreServiceLanguage = true;

        //note findService will automatically return the highest ranking service id
        svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    }

    return svcId;
}

double pubsubEndpoint_matchPublisher(
        celix_bundle_context_t *ctx,
        long bundleId,
        const char *filter,
        const char *adminType,
        double sampleScore,
        double controlScore,
        double defaultScore,
        bool matchProtocol,
        celix_properties_t **outTopicProperties,
        long *outSerializerSvcId,
        long *outProtocolSvcId) {

    celix_properties_t *ep = pubsubEndpoint_createFromPublisherTrackerInfo(ctx, bundleId, filter);
    const char *requested_admin         = NULL;
    const char *requested_qos            = NULL;
    if (ep != NULL) {
        requested_admin = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
        requested_qos = celix_properties_get(ep, PUBSUB_UTILS_QOS_ATTRIBUTE_KEY, NULL);
    }

    double score = getPSAScore(requested_admin, requested_qos, adminType, sampleScore, controlScore, defaultScore);

    const char *requested_serializer = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, NULL);
    long serializerSvcId = getPSASerializer(ctx, requested_serializer);

    if (serializerSvcId < 0) {
        score = PUBSUB_ADMIN_NO_MATCH_SCORE; //no serializer, no match
    }

    if (outSerializerSvcId != NULL) {
        *outSerializerSvcId = serializerSvcId;
    }

    if (matchProtocol) {
        const char *requested_protocol = celix_properties_get(ep, PUBSUB_ENDPOINT_PROTOCOL, NULL);
        long protocolSvcId = getPSAProtocol(ctx, requested_protocol);

        if (protocolSvcId < 0) {
            score = PUBSUB_ADMIN_NO_MATCH_SCORE;
        }

        if (outProtocolSvcId != NULL) {
            *outProtocolSvcId = protocolSvcId;
        }
    }

    if (outTopicProperties != NULL) {
        *outTopicProperties = ep;
    } else if (ep != NULL) {
        celix_properties_destroy(ep);
    }

    return score;
}

typedef struct pubsub_match_retrieve_topic_properties_data {
    const char *topic;
    const char *scope;
    bool isPublisher;

    celix_properties_t *outEndpoint;
} pubsub_get_topic_properties_data_t;

static void getTopicPropertiesCallback(void *handle, const celix_bundle_t *bnd) {
    pubsub_get_topic_properties_data_t *data = handle;
    data->outEndpoint = pubsub_utils_getTopicProperties(bnd, data->scope, data->topic, data->isPublisher);
}

double pubsubEndpoint_matchSubscriber(
        celix_bundle_context_t *ctx,
        const long svcProviderBundleId,
        const celix_properties_t *svcProperties,
        const char *adminType,
        double sampleScore,
        double controlScore,
        double defaultScore,
        bool matchProtocol,
        celix_properties_t **outTopicProperties,
        long *outSerializerSvcId,
        long *outProtocolSvcId) {

    pubsub_get_topic_properties_data_t data;
    data.isPublisher = false;
    data.scope = celix_properties_get(svcProperties, PUBSUB_SUBSCRIBER_SCOPE, NULL);
    data.topic = celix_properties_get(svcProperties, PUBSUB_SUBSCRIBER_TOPIC, NULL);
    data.outEndpoint = NULL;
    celix_bundleContext_useBundle(ctx, svcProviderBundleId, &data, getTopicPropertiesCallback);

    celix_properties_t *ep = data.outEndpoint;
    const char *requested_admin         = NULL;
    const char *requested_qos            = NULL;
    const char *requested_serializer     = NULL;
    const char *requested_protocol = NULL;
    if (ep != NULL) {
        requested_admin = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
        requested_qos = celix_properties_get(ep, PUBSUB_UTILS_QOS_ATTRIBUTE_KEY, NULL);
        requested_serializer = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, NULL);
        if (matchProtocol) {
            requested_protocol = celix_properties_get(ep, PUBSUB_ENDPOINT_PROTOCOL, NULL);
        }
    }

    double score = getPSAScore(requested_admin, requested_qos, adminType, sampleScore, controlScore, defaultScore);

    long serializerSvcId = getPSASerializer(ctx, requested_serializer);
    if (serializerSvcId < 0) {
        score = PUBSUB_ADMIN_NO_MATCH_SCORE; //no serializer, no match
    }

    if (outSerializerSvcId != NULL) {
        *outSerializerSvcId = serializerSvcId;
    }

    if (matchProtocol) {
        long protocolSvcId = getPSAProtocol(ctx, requested_protocol);
        if (protocolSvcId < 0) {
            score = PUBSUB_ADMIN_NO_MATCH_SCORE; //no protocol, no match
        }

        if (outProtocolSvcId != NULL) {
            *outProtocolSvcId = protocolSvcId;
        }
    }

    if (outTopicProperties != NULL) {
        *outTopicProperties = ep;
    } else if (ep != NULL) {
        celix_properties_destroy(ep);
    }

    return score;
}

bool pubsubEndpoint_match(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const celix_properties_t *ep,
        const char *adminType,
        bool matchProtocol,
        long *outSerializerSvcId,
        long *outProtocolSvcId) {

    bool psaMatch = false;
    const char *configured_admin = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
    if (configured_admin != NULL) {
        psaMatch = strncmp(configured_admin, adminType, strlen(adminType) + 1) == 0;
    }

    bool serMatch = false;
    long serializerSvcId = -1L;
    if (psaMatch) {
        const char *configured_serializer = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, NULL);
        serializerSvcId = getPSASerializer(ctx, configured_serializer);
        serMatch = serializerSvcId >= 0;

        if(!serMatch) {
            celix_logHelper_log(logHelper, CELIX_LOG_LEVEL_ERROR, "Matching endpoint for technology %s but couldn't get serializer %s", configured_admin, configured_serializer);
        }
    }

    bool match = psaMatch && serMatch;

    if (matchProtocol) {
        bool protMatch = false;
        long protocolSvcId = -1L;
        if (psaMatch) {
            const char *configured_protocol = celix_properties_get(ep, PUBSUB_ENDPOINT_PROTOCOL, NULL);
            protocolSvcId = getPSAProtocol(ctx, configured_protocol);
            protMatch = protocolSvcId >= 0;

            if(!protMatch) {
                celix_logHelper_log(logHelper, CELIX_LOG_LEVEL_ERROR, "Matching endpoint for technology %s but couldn't get protocol %s", configured_admin, configured_protocol);
            }
        }
        match = match && protMatch;

        if (outProtocolSvcId != NULL) {
            *outProtocolSvcId = protocolSvcId;
        }
    }

    if (outSerializerSvcId != NULL) {
        *outSerializerSvcId = serializerSvcId;
    }

    return match;
}

bool pubsubEndpoint_matchWithTopicAndScope(const celix_properties_t* endpoint, const char *topic, const char *scope) {
    const char *endpointScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, PUBSUB_DEFAULT_ENDPOINT_SCOPE);
    const char *endpointTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    if (scope == NULL) {
        scope = PUBSUB_DEFAULT_ENDPOINT_SCOPE;
    }

    return celix_utils_stringEquals(topic, endpointTopic) && celix_utils_stringEquals(scope, endpointScope);
}
