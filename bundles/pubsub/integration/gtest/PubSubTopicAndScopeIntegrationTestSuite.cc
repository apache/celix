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

#include <gtest/gtest.h>
#include <thread>

#include "celix/FrameworkFactory.h"
#include "pubsub/publisher.h"

class PubSubTopicAndScopeIntegrationTestSuite : public ::testing::Test {
public:

    PubSubTopicAndScopeIntegrationTestSuite() {
        celix::Properties config {{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"}};
        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();

        EXPECT_GE(ctx->installBundle(PUBSUB_JSON_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_TOPMAN_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_ZMQ_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_WIRE_BUNDLE_FILE), 0);
    }


    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

class TestComponent{};

TEST_F(PubSubTopicAndScopeIntegrationTestSuite, ComponentsWithSameTopicAndDifferentScope) {
    //When I create publisher with the same topic, but different scope
    //I expect the pubsub topology manager and a PSA can handle this and the
    //publisher components will become active. 

    auto& cmp1 = ctx->getDependencyManager()->createComponent<TestComponent>();
    cmp1.createServiceDependency<pubsub_publisher>(PUBSUB_PUBLISHER_SERVICE_NAME)
            .setFilter("(topic=foo)");
    cmp1.build();
    EXPECT_EQ(cmp1.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);

    auto& cmp2 = ctx->getDependencyManager()->createComponent<TestComponent>();
    cmp2.createServiceDependency<pubsub_publisher>(PUBSUB_PUBLISHER_SERVICE_NAME)
            .setFilter("(&(topic=foo)(scope=bar))");
    cmp2.build();
    EXPECT_EQ(cmp2.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);

    auto& cmp3 = ctx->getDependencyManager()->createComponent<TestComponent>();
    cmp3.createServiceDependency<pubsub_publisher>(PUBSUB_PUBLISHER_SERVICE_NAME)
            .setFilter("(&(topic=foo)(scope=default))");
    cmp3.build();
    EXPECT_EQ(cmp3.getState(), celix::dm::ComponentState::TRACKING_OPTIONAL);
}