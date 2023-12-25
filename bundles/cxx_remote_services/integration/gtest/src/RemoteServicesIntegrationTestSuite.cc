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
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"},
                {celix::FRAMEWORK_CACHE_DIR, ".clientCache"},
        };
        clientFw = celix::createFramework(clientConfig);
        clientCtx = clientFw->getFrameworkBundleContext();

        celix::Properties serverConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"},
                {celix::FRAMEWORK_CACHE_DIR, ".serverCache"},
        };
        serverFw = celix::createFramework(serverConfig);
        serverCtx = serverFw->getFrameworkBundleContext();
    }

    static void installSharedBundles(std::shared_ptr<celix::BundleContext>& ctx) {
        auto sharedBundles = {
                RS_DISCOVERY_BUNDLE_LOC,
                RS_FACTORY_BUNDLE_LOC ,
                RS_RSA_BUNDLE_LOC};
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

    static void printTopicSendersAndReceivers(celix::BundleContext& ctx) {
        ctx.useService<celix_shell_command>(CELIX_SHELL_COMMAND_SERVICE_NAME)
                .setFilter((std::string{"("}.append(CELIX_SHELL_COMMAND_NAME).append("=celix::psa_zmq)")))
                .addUseCallback([](auto& cmd) {
                    cmd.executeCommand(cmd.handle, "psa_zmq", stdout, stdout);
                })
                .build();
    }

    void invokeRemoteCalcService() {
        //If a calculator provider bundle is installed I expect a exported calculator interface
        auto count = serverCtx->useService<ICalculator>()
                .setFilter("(service.exported.interfaces=*)")
                .build();
        EXPECT_EQ(count, 1);

        //If a calculator consumer bundle is installed and also the needed remote services bundle, I expect an import calculator interface
        count = clientCtx->useService<ICalculator>()
                .setTimeout(std::chrono::seconds{1})
                .setFilter("(service.imported=*)")
                .build();
        EXPECT_EQ(count, 1);

        //When I call the calculator service from the client, I expect an answer
        std::atomic<int> streamCount = 0;
        std::atomic<double> lastStreamValue = 0.0;
        std::atomic<bool> promiseSuccessful = false;
        std::atomic<double> promiseValue = 0.0;

        /*
         * Testing the remote service in a while loop till it is successful or 10 seconds has passed.
         * Note that because mq does not guarantee a connection when used, it is possible - and likely -
         * that the first remote test iteration fails due to not yet completely connected mq.
         */
        auto start = std::chrono::system_clock::now();
        auto now = std::chrono::system_clock::now();
        int iter = 1;
        while ((streamCount == 0 || !promiseSuccessful) && (now - start) < std::chrono::seconds{10}) {
            clientCtx->logInfo("Testing remote C++ iteration %i.", iter++);
            count = clientCtx->useService<ICalculator>()
                    .addUseCallback([&](auto& calc) {
                        //testing remote stream
                        auto stream = calc.result();
                        auto streamEnded = stream->forEach([&](double event){
                            lastStreamValue = event;
                            streamCount++;
                        });
                        streamEnded.onResolve([ctx = clientCtx, i = iter]() {
                            ctx->logInfo("Stream for iteration %i closed", i);
                        });

                        //testing remote promise
                        auto promise = calc.add(2, 4).onFailure([ctx = clientCtx, i = iter](const auto& e) {
                            ctx->logInfo("Got remote promise exception for iteration %i: %s", i, e.what());
                        });
                        promise.wait();
                        promiseSuccessful = promise.isSuccessfullyResolved();
                        if (promiseSuccessful) {
                            promiseValue = promise.getValue();
                        }
                    })
                    .build();
            now = std::chrono::system_clock::now();
        }
        EXPECT_EQ(count, 1);
        EXPECT_GE(streamCount, 1);
        EXPECT_GE(lastStreamValue, 0.0);
        EXPECT_TRUE(promiseSuccessful);
        EXPECT_EQ(6, promiseValue);

        if (streamCount == 0 || !promiseSuccessful) {
            //extra debug info
            printTopicSendersAndReceivers(*clientCtx);
            printTopicSendersAndReceivers(*serverCtx);
        }
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
    invokeRemoteCalcService();
}
