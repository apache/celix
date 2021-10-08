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

#include "ICalculator.h"
#include "celix_shell_command.h"
#include "celix/FrameworkFactory.h"

class RemoteServicesIntegrationTestSuite : public ::testing::Test {
public:
    RemoteServicesIntegrationTestSuite() {
        celix::Properties clientConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {celix::FRAMEWORK_CACHE_DIR, ".clientCache"},
                //Static configuration to let the pubsub zmq operate without discovery
                {"PSA_ZMQ_STATIC_BIND_URL_FOR_test_invoke_default", "ipc:///tmp/pubsub-test-return"},
                {"PSA_ZMQ_STATIC_CONNECT_URL_FOR_test_return_default", "ipc:///tmp/pubsub-test-invoke" }
        };
        clientFw = celix::createFramework(clientConfig);
        clientCtx = clientFw->getFrameworkBundleContext();

        celix::Properties serverConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {celix::FRAMEWORK_CACHE_DIR, ".serverCache"},
                //Static configuration to let the pubsub zmq operate without discovery
                {"PSA_ZMQ_STATIC_BIND_URL_FOR_test_return_default", "ipc:///tmp/pubsub-test-invoke"},
                {"PSA_ZMQ_STATIC_CONNECT_URL_FOR_test_invoke_default", "ipc:///tmp/pubsub-test-return" }
        };
        serverFw = celix::createFramework(serverConfig);
        serverCtx = serverFw->getFrameworkBundleContext();
    }

    static void installSharedBundles(std::shared_ptr<celix::BundleContext>& ctx) {
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
    installProviderBundles();
    installConsumerBundles();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService) {
    installProviderBundles();
    installConsumerBundles();

    //If a calculator provider bundle is installed I expect a exported calculator interface
    auto count = serverCtx->useService<ICalculator>()
            .setFilter("(service.exported.interfaces=*)")
            .build();
    EXPECT_EQ(count, 1);

    //If a calculator consumer bundle is installed and also the needed remote services bundle,  I expect a import calculator interface
    count = clientCtx->useService<ICalculator>()
            .setTimeout(std::chrono::seconds{1})
            .setFilter("(service.imported=*)")
            .build();
    EXPECT_EQ(count, 1);

    /*DEBUG INFO*/
    clientCtx->useService<celix_shell_command>(CELIX_SHELL_COMMAND_SERVICE_NAME)
            .setFilter((std::string{"("}.append(CELIX_SHELL_COMMAND_NAME).append("=celix::psa_zmq)")))
            .addUseCallback([](auto& cmd) {
                cmd.executeCommand(cmd.handle, "psa_zmq", stdout, stdout);
            })
            .build();
    serverCtx->useService<celix_shell_command>(CELIX_SHELL_COMMAND_SERVICE_NAME)
            .setFilter((std::string{"("}.append(CELIX_SHELL_COMMAND_NAME).append("=celix::psa_zmq)")))
            .addUseCallback([](auto& cmd) {
                cmd.executeCommand(cmd.handle, "psa_zmq", stdout, stdout);
            })
            .build();
    std::shared_ptr<celix::PushStream<double>> stream;
    //When I call the calculator service from the client, I expect a answer
    int streamCount = 0;
    double lastValue = 0.0;
    count = clientCtx->useService<ICalculator>()
            .addUseCallback([&](auto& calc) {
                stream = calc.result();
                stream->forEach([&](double event){
                    lastValue = event;
                    streamCount++;
                });

                auto promise = calc.add(2, 4);
                promise.wait();
                EXPECT_TRUE(promise.isSuccessfullyResolved());
                if (promise.isSuccessfullyResolved()) {
                    EXPECT_EQ(6, promise.getValue());
                }
                sleep(1);
                EXPECT_GE(streamCount,0 );
                EXPECT_GE(lastValue, 0.0);
            })
            .build();
    EXPECT_EQ(count, 1);
}