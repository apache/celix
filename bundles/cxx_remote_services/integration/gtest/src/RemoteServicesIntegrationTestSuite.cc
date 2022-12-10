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
        auto pubsubTestReturnIpc = std::string{"ipc:///tmp/pubsub-test-return"};
        auto pubsubTestInvokeIpc = std::string{"ipc:///tmp/pubsub-test-invoke"};

        celix::Properties clientConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"},
                {celix::FRAMEWORK_CACHE_DIR, ".clientCache"},
                //Static configuration to let the pubsub zmq operate without discovery
                {"PSA_ZMQ_STATIC_BIND_URL_FOR_test_invoke_default", pubsubTestInvokeIpc},
                {"PSA_ZMQ_STATIC_CONNECT_URL_FOR_test_return_default", pubsubTestReturnIpc }
        };
        clientFw = celix::createFramework(clientConfig);
        clientCtx = clientFw->getFrameworkBundleContext();

        celix::Properties serverConfig{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"},
                {celix::FRAMEWORK_CACHE_DIR, ".serverCache"},
                //Static configuration to let the pubsub zmq operate without discovery
                {"PSA_ZMQ_STATIC_BIND_URL_FOR_test_return_default",  pubsubTestReturnIpc },
                {"PSA_ZMQ_STATIC_CONNECT_URL_FOR_test_invoke_default", pubsubTestInvokeIpc }
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

    void printTopicSendersAndReceivers(celix::BundleContext& ctx) {
        ctx.useService<celix_shell_command>(CELIX_SHELL_COMMAND_SERVICE_NAME)
                .setFilter((std::string{"("}.append(CELIX_SHELL_COMMAND_NAME).append("=celix::psa_zmq)")))
                .addUseCallback([](auto& cmd) {
                    cmd.executeCommand(cmd.handle, "psa_zmq", stdout, stdout);
                })
                .build();
    }

    void invokeRemoteCalcService() {
        installProviderBundles();
        installConsumerBundles();

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
        std::atomic<double> lastValue = 0.0;
        std::atomic<bool> promiseSuccessful = false;
        count = clientCtx->useService<ICalculator>()
                .addUseCallback([&](auto& calc) {
                    //NOTE inside the use callback components using suspend strategy cannot be updated, because
                    //the use called on the event thread. So use locking instead.

                    //testing remote stream
                    auto stream = calc.result();
                    auto streamEnded = stream->forEach([&](double event){
                        lastValue = event;
                        streamCount++;
                    });

                    auto start = std::chrono::system_clock::now();
                    auto now = std::chrono::system_clock::now();
                    int iter = 1;
                    while (streamCount <= 0 && (now - start) < std::chrono::seconds{5}) {
                        clientCtx->logInfo("Checking stream for iteration %i.", iter++);
                        std::this_thread::sleep_for(std::chrono::milliseconds{10});
                        now = std::chrono::system_clock::now();
                    }

                    //testing remote promise
                    start = std::chrono::system_clock::now();
                    now = std::chrono::system_clock::now();
                    iter = 1;
                    while (!promiseSuccessful && (now - start) < std::chrono::seconds{5}) {
                        clientCtx->logInfo("Checking promise for iteration %i", iter++);
                        auto promise = calc.add(2, 4).onFailure([ctx = clientCtx](const auto &e) {
                            ctx->logError("Got remote promise exception: %s", e.what());
                        });
                        promise.wait();
                        promiseSuccessful = promise.isSuccessfullyResolved();
                        if (promiseSuccessful) {
                            EXPECT_EQ(6, promise.getValue());
                        }
                        now = std::chrono::system_clock::now();
                    }
                })
                .build();
        EXPECT_EQ(count, 1);
        EXPECT_GE(streamCount, 1);
        EXPECT_GE(lastValue, 0.0);
        EXPECT_TRUE(promiseSuccessful);

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

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService1) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService2) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService3) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService4) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService5) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService6) {
    invokeRemoteCalcService();

}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService7) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService8) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService9) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService10) {
    invokeRemoteCalcService();
}


TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService11) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService12) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService13) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService14) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService15) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService16) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService17) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService18) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService19) {
    invokeRemoteCalcService();
}

TEST_F(RemoteServicesIntegrationTestSuite, InvokeRemoteCalcService20) {
    invokeRemoteCalcService();
}