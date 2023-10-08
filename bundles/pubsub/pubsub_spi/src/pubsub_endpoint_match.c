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

#include "pubsub_endpoint.h"
#include "celix_utils.h"
#include "celix_constants.h"


bool pubsubEndpoint_matchWithTopicAndScope(const celix_properties_t* endpoint, const char *topic, const char *scope) {
    const char *endpointScope = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, PUBSUB_DEFAULT_ENDPOINT_SCOPE);
    const char *endpointTopic = celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
    if (scope == NULL) {
        scope = PUBSUB_DEFAULT_ENDPOINT_SCOPE;
    }

    return celix_utils_stringEquals(topic, endpointTopic) && celix_utils_stringEquals(scope, endpointScope);
}
