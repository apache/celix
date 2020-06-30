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

#include "gtest/gtest.h"

#include "pubsub_endpoint.h"

TEST(PubSubEndpointUtilsTestSuite, pubsubEndpoint_matchWithTopicAndScope) {
    celix_properties_t* endpoint = celix_properties_create();
    celix_properties_set(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, "topic");

    EXPECT_TRUE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", nullptr));
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topicaa", nullptr));
    EXPECT_TRUE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "default")); //Note "default" is the same as NULL scope
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "scope"));

    celix_properties_set(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, "scope");
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", nullptr));
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topicaa", nullptr));
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "default"));
    EXPECT_TRUE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "scope"));
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "scopeaa"));

    celix_properties_set(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, "default");
    EXPECT_TRUE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", nullptr)); //Note NULL is the same as "default" scope
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topicaa", nullptr));
    EXPECT_TRUE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "default"));
    EXPECT_FALSE(pubsubEndpoint_matchWithTopicAndScope(endpoint, "topic", "scope"));

    celix_properties_destroy(endpoint);
}
