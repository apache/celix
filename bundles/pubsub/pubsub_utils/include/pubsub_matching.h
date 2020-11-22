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

#ifndef PUBSUB_MATCHING_H_
#define PUBSUB_MATCHING_H_

#include <celix_log_helper.h>
#include "bundle_context.h"
#include "celix_array_list.h"
#include "celix_bundle_context.h"
#ifdef __cplusplus
extern "C" {
#endif

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
double pubsub_utils_matchPublisher(
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
double pubsub_utils_matchSubscriber(
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
bool pubsub_utils_matchEndpoint(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const celix_properties_t *ep,
        const char *adminType,
        bool matchProtocol,
        long *outSerializerSvcId,
        long *outProtocolSvcId);
#ifdef __cplusplus
}
#endif

#endif /* PUBSUB_MATCHING_H_ */
