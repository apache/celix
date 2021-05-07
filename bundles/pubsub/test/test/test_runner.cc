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

#include "celix_api.h"
#include <unistd.h>
#include "receive_count_service.h"

class PubSubIntegrationTestSuite : public ::testing::Test {
public:
    PubSubIntegrationTestSuite() {
        celixLauncher_launch("config.properties", &fw);
        ctx = celix_framework_getFrameworkContext(fw);
    }

    ~PubSubIntegrationTestSuite() override {
        celixLauncher_stop(fw);
        celixLauncher_waitForShutdown(fw);
        celixLauncher_destroy(fw);
    }

    PubSubIntegrationTestSuite(const PubSubIntegrationTestSuite&) = delete;
    PubSubIntegrationTestSuite(PubSubIntegrationTestSuite&&) = delete;
    PubSubIntegrationTestSuite& operator=(const PubSubIntegrationTestSuite&) = delete;
    PubSubIntegrationTestSuite& operator=(PubSubIntegrationTestSuite&&) = delete;

    celix_framework_t* fw = nullptr;
    celix_bundle_context_t* ctx = nullptr;
};

class PubSubIntegrationWithEnvironmentTestSuite : public ::testing::Test {
public:
    PubSubIntegrationWithEnvironmentTestSuite() {
        setenv("PSA_TCP_STATIC_BIND_URL_FOR_ping", "tcp://localhost:9001", 1);
        setenv("PSA_TCP_STATIC_CONNECT_URL_FOR_ping", "tcp://localhost:9001", 1);
        setenv("PSA_UDPMC_STATIC_BIND_PORT_FOR_ping", "9001", 1);
        setenv("PSA_UDPMC_STATIC_CONNECT_URLS_FOR_ping", "224.100.0.1:9001", 1);
        setenv("PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES_FOR_ping", "127.0.0.1:9001", 1);
        setenv("CELIX_HTTP_ADMIN_LISTENING_PORTS", "9001", 1);
        setenv("PSA_ZMQ_STATIC_BIND_URL_FOR_ping", "ipc:///tmp/pubsub-envtest", 1);
        setenv("PSA_ZMQ_STATIC_CONNECT_URL_FOR_ping", "ipc:///tmp/pubsub-envtest", 1);

        celixLauncher_launch("config.properties", &fw);
        ctx = celix_framework_getFrameworkContext(fw);
    }

    ~PubSubIntegrationWithEnvironmentTestSuite() override {
        celixLauncher_stop(fw);
        celixLauncher_waitForShutdown(fw);
        celixLauncher_destroy(fw);

        unsetenv("PSA_TCP_STATIC_BIND_URL_FOR_ping");
        unsetenv("PSA_TCP_STATIC_CONNECT_URL_FOR_ping");
        unsetenv("PSA_UDPMC_STATIC_BIND_PORT_FOR_ping");
        unsetenv("PSA_UDPMC_STATIC_CONNECT_URLS_FOR_ping");
        unsetenv("PUBSUB_WEBSOCKET_STATIC_CONNECT_SOCKET_ADDRESSES_FOR_ping");
        unsetenv("CELIX_HTTP_ADMIN_LISTENING_PORTS");
        unsetenv("PSA_ZMQ_STATIC_BIND_URL_FOR_ping");
        unsetenv("PSA_ZMQ_STATIC_CONNECT_URL_FOR_ping");
    }

    PubSubIntegrationWithEnvironmentTestSuite(const PubSubIntegrationWithEnvironmentTestSuite&) = delete;
    PubSubIntegrationWithEnvironmentTestSuite(PubSubIntegrationWithEnvironmentTestSuite&&) = delete;
    PubSubIntegrationWithEnvironmentTestSuite& operator=(const PubSubIntegrationWithEnvironmentTestSuite&) = delete;
    PubSubIntegrationWithEnvironmentTestSuite& operator=(PubSubIntegrationWithEnvironmentTestSuite&&) = delete;

    celix_framework_t* fw = nullptr;
    celix_bundle_context_t* ctx = nullptr;
};

void receiveTest(celix_bundle_context_t *ctx) {
    constexpr int TRIES = 50;
    constexpr int TIMEOUT = 250000;
    constexpr int MSG_COUNT = 100;

    int count = 0;

    for (int i = 0; i < TRIES; ++i) {
        count = 0;
        celix_bundleContext_useService(ctx, CELIX_RECEIVE_COUNT_SERVICE_NAME, &count, [](void *handle, void *svc) {
            auto *count_ptr = static_cast<int *>(handle);
            auto *count = static_cast<celix_receive_count_service_t *>(svc);
            *count_ptr = count->receiveCount(count->handle);
        });
        printf("Current msg count is %i, waiting for at least %i\n", count, MSG_COUNT);
        if (count >= MSG_COUNT) {
            break;
        }
        usleep(TIMEOUT);
    }
    EXPECT_GE(count, MSG_COUNT);
}

TEST_F(PubSubIntegrationTestSuite, recvTest) {
    receiveTest(ctx);
}


TEST_F(PubSubIntegrationWithEnvironmentTestSuite, recvTest) {
    receiveTest(ctx);
}
