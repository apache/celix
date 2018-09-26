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

#ifndef PUBSUB_UTILS_H_
#define PUBSUB_UTILS_H_

#include "bundle_context.h"
#include "celix_array_list.h"
#include "celix_bundle_context.h"

#define PUBSUB_UTILS_QOS_ATTRIBUTE_KEY	    "qos"
#define PUBSUB_UTILS_QOS_TYPE_SAMPLE		"sample"	/* A.k.a. unreliable connection */
#define PUBSUB_UTILS_QOS_TYPE_CONTROL	    "control"	/* A.k.a. reliable connection */


/**
 * Returns the pubsub info from the provided filter. A pubsub filter should have a topic and can 
 * have a scope. If no topic is present the topic and scope output will be NULL.
 * If a topic is present the topic output will contain a allocated topic string and if a scope was
 * present a allocated scope string.
 * The caller is owner of the topic and scope output string.
 */
celix_status_t pubsub_getPubSubInfoFromFilter(const char* filterstr, char **topic, char **scope);

char* pubsub_getKeysBundleDir(bundle_context_pt ctx);

double pubsub_utils_matchPublisher(
        celix_bundle_context_t *ctx,
        long bundleId,
        const char *filter,
        const char *adminType,
        double sampleScore,
        double controlScore,
        double defaultScore,
        long *outSerializerSvcId);

double pubsub_utils_matchSubscriber(
        celix_bundle_context_t *ctx,
        long svcProviderBundleId,
        const celix_properties_t *svcProperties,
        const char *adminType,
        double sampleScore,
        double controlScore,
        double defaultScore,
        long *outSerializerSvcId);

bool pubsub_utils_matchEndpoint(
        celix_bundle_context_t *ctx,
        const celix_properties_t *endpoint,
        const char *adminType,
        long *outSerializerSvcId);


celix_properties_t* pubsub_utils_getTopicProperties(const celix_bundle_t *bundle, const char *topic, bool isPublisher);


#endif /* PUBSUB_UTILS_H_ */
