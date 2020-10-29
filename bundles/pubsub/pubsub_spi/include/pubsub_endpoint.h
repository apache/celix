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

#ifndef PUBSUB_ENDPOINT_H_
#define PUBSUB_ENDPOINT_H_

#include "celix_log_helper.h"
#include "celix_bundle_context.h"
#include "celix_properties.h"

#include "pubsub/publisher.h"
#include "pubsub/subscriber.h"

#include "pubsub_constants.h"

#ifdef __cplusplus
extern "C" {
#endif
//required for valid endpoint
#define PUBSUB_ENDPOINT_TOPIC_NAME      "pubsub.topic.name"
#define PUBSUB_ENDPOINT_TOPIC_SCOPE     "pubsub.topic.scope"

#define PUBSUB_ENDPOINT_UUID            "pubsub.endpoint.uuid" //required
#define PUBSUB_ENDPOINT_FRAMEWORK_UUID  "pubsub.framework.uuid" //required
#define PUBSUB_ENDPOINT_TYPE            "pubsub.endpoint.type" //PUBSUB_PUBLISHER_ENDPOINT_TYPE or PUBSUB_SUBSCRIBER_ENDPOINT_TYPE
#define PUBSUB_ENDPOINT_VISIBILITY      "pubsub.endpoint.visibility" //local, host or system. e.g. for IPC host
#define PUBSUB_ENDPOINT_ADMIN_TYPE       PUBSUB_ADMIN_TYPE_KEY
#define PUBSUB_ENDPOINT_SERIALIZER       PUBSUB_SERIALIZER_TYPE_KEY
#define PUBSUB_ENDPOINT_PROTOCOL         PUBSUB_PROTOCOL_TYPE_KEY


#define PUBSUB_PUBLISHER_ENDPOINT_TYPE      "publisher"
#define PUBSUB_SUBSCRIBER_ENDPOINT_TYPE     "subscriber"
#define PUBSUB_ENDPOINT_VISIBILITY_DEFAULT  PUBSUB_ENDPOINT_SYSTEM_VISIBILITY


celix_properties_t *
pubsubEndpoint_create(const char *fwUUID, const char *scope, const char *topic, const char *pubsubType,
                      const char *adminType, const char *serType, const char *protType, celix_properties_t *topic_props);

celix_properties_t *
pubsubEndpoint_createFromSubscriberSvc(bundle_context_t *ctx, long svcBndId, const celix_properties_t *svcProps);

celix_properties_t *
pubsubEndpoint_createFromPublisherTrackerInfo(bundle_context_t *ctx, long bundleId, const char *filter);

bool pubsubEndpoint_equals(const celix_properties_t *psEp1, const celix_properties_t *psEp2);

//check if the required properties are available for the endpoint
bool
pubsubEndpoint_isValid(const celix_properties_t *endpointProperties, bool requireAdminType, bool requireSerializerType);

/**
 * Create a key based on scope an topic.
 * Scope can be NULL.
 * Note that NULL, "topic" and "default", "topic" will result in different keys
 * @return a newly created key. caller is responsible for freeing the string array.
 */
char *pubsubEndpoint_createScopeTopicKey(const char *scope, const char *topic);

/**
 * Match a publisher for a provided bnd (using the bundleId) and service filter.
 *
 * The match function will try to find a topic properties for the requesting bundle (bundleId) using the topic
 * from the filter at META-INF/topics/pub/<topic>.properties
 *
 * If the topic properties is configured for the provided adminType (i.e. pubsub.config=ZMQ) a full match will
 * be returned. If no specific admin is configured in the topic properties the sampleScore will be returned if sample
 * qos is configured (i.e. qos=sample), the controlScore will be returned if control qos is configured
 * (i.e. qos=control) and otherwise the defaultScore will be returned.
 *
 * The match function will also search for a valid serializer. If a serializer is configured in the topic properties
 * (i.e. pubsub.serializer=json) that specific serializer will be searched. If no serializer is configured the
 * highest ranking serializer service will be returned. If no serializer can be found, the outSerializerSvcId will
 * be -1.
 *
 * The function will also returned the found topic properties and the matching serialized.
 * The caller is owner of the outTopicProperties.
 *
 * @param ctx                   The bundle context.
 * @param bundleId              The requesting bundle id.
 * @param filter                The filter of the publisher (i.e. "(&(topic=example)(scope=subsystem))")
 * @param adminType             The admin type used for the match.
 * @param sampleScore           The sample score used for the match.
 * @param controlScore          The control score used for the match.
 * @param defaultScore          The default score used for the match.
 * @param outTopicProperties    Output pointer for the read topic properties. Return can be NULL.
 * @param outSerializerSvcId    Output svc id for the matching serializer. If not found will be -1L.
 * @return                      The matching score.
 */
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
        long *outProtocolSvcId);

/**
 * Match a subscriber for a provided bnd (using the bundleId) and provided service properties.
 *
 * The match function will try to find a topic properties for the requesting bundle (bundleId) - using topic in the
 * provided service properties - at META-INF/topics/sub/<topic>.properties
 *
 * If the topic properties is configured for the provided adminType (i.e. pubsub.config=ZMQ) a full match will
 * be returned. If no specific admin is configured in the topic properties the sampleScore will be returned if sample
 * qos is configured (i.e. qos=sample), the controlScore will be returned if control qos is configured
 * (i.e. qos=control) and otherwise the defaultScore will be returned.
 *
 * The match function will also search for a valid serializer. If a serializer is configured in the topic properties
 * (i.e. pubsub.serializer=json) that specific serializer will be searched. If no serializer is configured the
 * highest ranking serializer service will be returned. If no serializer can be found, the outSerializerSvcId will
 * be -1.
 *
 * The function will also returned the found topic properties and the matching serialized.
 * The caller is owner of the outTopicProperties.
 *
 * @param ctx                   The bundle context.
 * @param bundleId              The requesting bundle id.
 * @param svcProperties         The service properties of the registered subscriber service.
 * @param adminType             The admin type used for the match.
 * @param sampleScore           The sample score used for the match.
 * @param controlScore          The control score used for the match.
 * @param defaultScore          The default score used for the match.
 * @param outTopicProperties    Output pointer for the read topic properties. Return can be NULL.
 * @param outSerializerSvcId    Output svc id for the matching serializer. If not found will be -1L.
 * @return                      The matching score.
 */
double pubsubEndpoint_matchSubscriber(
        celix_bundle_context_t *ctx,
        long svcProviderBundleId,
        const celix_properties_t *svcProperties,
        const char *adminType,
        double sampleScore,
        double controlScore,
        double defaultScore,
        bool matchProtocol,
        celix_properties_t **outTopicProperties,
        long *outSerializerSvcId,
        long *outProtocolSvcId);

/**
 * Match an endpoint (subscriber or publisher endpoint) for the provided admin type.
 *
 * Also tries to found the matching serializer configured in the endpoint.
 *
 * @param ctx                   The bundle context.
 * @param endpoint              The endpoint to match.
 * @param adminType             The admin type (i.e. UDPMC)
 * @param outSerializerSvcId    The found serialized svc id based on the endpoint or -1 if no serializer is
 *                              configured/found.
 * @return                      true if there is a match.
 */
bool pubsubEndpoint_match(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const celix_properties_t *endpoint,
        const char *adminType,
        bool matchProtocol,
        long *outSerializerSvcId,
        long *outProtocolSvcId);

/**
 * Match an endpoint with a topic & scope.
 * @param endpoint The endpoints (mandatory)
 * @param topic The topic (mandatory)
 * @param scope The scope (can be NULL)
 * @return true if the endpoint is for the provide topic and scope);
 */
bool pubsubEndpoint_matchWithTopicAndScope(const celix_properties_t* endpoint, const char *topic, const char *scope);


#ifdef __cplusplus
}
#endif
#endif /* PUBSUB_ENDPOINT_H_ */
