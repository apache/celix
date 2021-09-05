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

#include "celix/FrameworkFactory.h"
#include "celix/rsa/EndpointDescription.h"
#include "celix/rsa/IConfiguredDiscoveryManager.h"

class RsaConfiguredDiscoveryTestSuite : public ::testing::Test {
public:
    RsaConfiguredDiscoveryTestSuite() {
        celix::Properties config{
                {celix::rsa::CONFIGURED_DISCOVERY_DISCOVERY_FILES, std::string{RSA_CONFIGURED_DISCOVERY_DISCOVERY_FILE}.append("  ,  garbage_path, garbage_path2  ")},
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"}
        };
        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

TEST_F(RsaConfiguredDiscoveryTestSuite, startStopStartStopBundle) {
    auto bndId = ctx->installBundle(RSA_CONFIGURED_DISCOVERY_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);
    ctx->stopBundle(bndId);
    ctx->startBundle(bndId);
}

TEST_F(RsaConfiguredDiscoveryTestSuite, discoverConfiguredEndpoints) {
    auto bndId = ctx->installBundle(RSA_CONFIGURED_DISCOVERY_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    //When I start the RSA Configured Discovery bundle with a endpoint discovery file with 2 endpoints
    //there will be 2 Endpoint services in the framework.
    auto count = ctx->useServices<celix::rsa::EndpointDescription>()
            .addUseCallback([](auto& endpoint) {
                EXPECT_NE(endpoint.getId(), "");
                EXPECT_NE(endpoint.getConfigurationTypes(), std::vector<std::string>{});
                EXPECT_NE(endpoint.getInterface(), "");
                EXPECT_NE(endpoint.getFrameworkUUID(), "");
                EXPECT_NE(endpoint.getProperties().get("endpoint.scope"), ""); //note async specific
                EXPECT_NE(endpoint.getProperties().get("endpoint.topic"), ""); //note async specific
                EXPECT_NE(endpoint.getProperties().get("endpoint.anykey"), ""); //note test specific
            })
            .build();
    EXPECT_EQ(count, 2);
}

TEST_F(RsaConfiguredDiscoveryTestSuite, removeConfiguredEndpointFile) {
    //When I install a configured discovery bundle with also a configured discovery file, I will find
    //2 EndpointDescriptions.
    //If I then remove the configured discovery file using the IConfiguredDiscoveryManager service, I will find
    //0 EndpointDescriptions.

    auto bndId = ctx->installBundle(RSA_CONFIGURED_DISCOVERY_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    auto count = ctx->useServices<celix::rsa::EndpointDescription>().build();
    EXPECT_EQ(count, 2);

    count = ctx->useService<celix::rsa::IConfiguredDiscoveryManager>()
                .addUseCallback([](auto& svc) {
                    auto files = svc.getConfiguredDiscoveryFiles();
                    ASSERT_EQ(files.size(), 1);
                    EXPECT_EQ(files[0], RSA_CONFIGURED_DISCOVERY_DISCOVERY_FILE);
                    svc.removeConfiguredDiscoveryFile(RSA_CONFIGURED_DISCOVERY_DISCOVERY_FILE);
                })
                .build();
    EXPECT_EQ(count, 1);


    count = ctx->useServices<celix::rsa::EndpointDescription>().build();
    EXPECT_EQ(count, 0);
}
