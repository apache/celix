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

class RemoteServicesIntegrationTestSuite : public ::testing::Test {
public:
    RemoteServicesIntegrationTestSuite() {
        celix::Properties clientConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {celix::FRAMEWORK_CACHE_DIR, ".clientCache"}
        };
        clientFw = celix::createFramework(clientConfig);
        clientCtx = clientFw->getFrameworkBundleContext();

        celix::Properties serverConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {celix::FRAMEWORK_CACHE_DIR, ".serverCache"}
        };
        serverFw = celix::createFramework(serverConfig);
        serverCtx = serverFw->getFrameworkBundleContext();
    }

    void installSharedBundles(std::shared_ptr<celix::BundleContext>& ctx) {
        auto sharedBundles = {
                PS_SER_BUNDLE_LOC,
                PS_PSTM_BUNDLE_LOC,
                PS_PSA_BUNDLE_LOC,
                PS_WIRE_BUNDLE_LOC,
                RS_DISCOVERY_BUNDLE_LOC,
                RS_RSA_BUNDLE_LOC,
                RS_FACTORY_BUNDLE_LOC };
        for (const auto& bndLoc : sharedBundles) {
            auto bndId = ctx->installBundle(bndLoc);
            EXPECT_GE(bndId, 0);
        }
    }

    void installProviderBundles() {
        installSharedBundles(serverCtx);
        auto bndId = serverCtx->installBundle(RS_PROVIDER_BUNDLE_LOC);
        EXPECT_GE(bndId, 0);
    }

    void installConsumerBundles() {
        installSharedBundles(clientCtx);
        auto bndId = clientCtx->installBundle(RS_CONSUMER_BUNDLE_LOC);
        EXPECT_GE(bndId, 0);
    }

    std::shared_ptr<celix::Framework> clientFw{};
    std::shared_ptr<celix::BundleContext> clientCtx{};
    std::shared_ptr<celix::Framework> serverFw{};
    std::shared_ptr<celix::BundleContext> serverCtx{};
};

TEST_F(RemoteServicesIntegrationTestSuite, StartStopFrameworks) {
    installConsumerBundles();
    installProviderBundles();
}