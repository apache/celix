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
#include <cstring>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <functional>
#include <future>
#include <thread>
#include <netinet/in.h>
#include <mosquitto.h>
#include <mqtt_protocol.h>
#include <uuid/uuid.h>

#include "celix_earpm_client.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "CelixEarpmTestSuiteBaseClass.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "celix_log_helper.h"
#include "celix_earpm_broker_discovery.h"

namespace {
constexpr const char *MQTT_BROKER_ADDRESS = "127.0.0.1";
constexpr int MQTT_BROKER_PORT = 1883;
}

#define CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts)                                                                        \
    memset(&opts, 0, sizeof(opts));                                                                                         \
    opts.ctx = ctx.get();                                                                                                   \
    opts.logHelper = logHelper.get();                                                                                       \
    opts.sessionEndMsgTopic = CELIX_EARPM_SESSION_END_TOPIC;                                                                \
    opts.sessionEndMsgSenderUUID = fwUUID.c_str();                                                                          \
    opts.callbackHandle = nullptr;                                                                                          \
    opts.receiveMsgCallback = [](void*, const celix_earpm_client_request_info_t*) {};                                       \
    opts.connectedCallback = [](void*) {};


class CelixEarpmClientTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    static void SetUpTestSuite() {
        mosquitto_lib_init();
        pid = fork();
        ASSERT_GE(pid, 0);
        if (pid == 0) {
            execlp("mosquitto", "mosquitto", "-p", std::to_string(MQTT_BROKER_PORT).c_str(), nullptr);
            ADD_FAILURE() << "Failed to start mosquitto";
        }
        testMosqClient = mosquitto_new(nullptr, true, nullptr);
        ASSERT_NE(testMosqClient, nullptr);
        auto rc = mosquitto_int_option(testMosqClient, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        for (int i = 0; i < 6000; ++i) {
            if(mosquitto_connect_bind_v5(testMosqClient, MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT, 60, nullptr, nullptr) == MOSQ_ERR_SUCCESS) {
                rc = mosquitto_loop_start(testMosqClient);
                ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ADD_FAILURE() << "Failed to init test environment";
    }

    static void TearDownTestSuite() {
        mosquitto_disconnect(testMosqClient);
        mosquitto_loop_stop(testMosqClient, false);
        mosquitto_destroy(testMosqClient);
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        mosquitto_lib_cleanup();
    }

    CelixEarpmClientTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_client_test_cache"} {
        logHelper = std::shared_ptr <celix_log_helper_t>
                {celix_logHelper_create(ctx.get(), "EarpmClient"), [](celix_log_helper_t *h) { celix_logHelper_destroy(h); }};
    }

    ~CelixEarpmClientTestSuite() override = default;

    static endpoint_description_t* CreateMqttBrokerEndpoint(void) {
        auto props = celix_properties_create();
        EXPECT_NE(props, nullptr);
        celix_properties_setLong(props, CELIX_RSA_ENDPOINT_SERVICE_ID, INT32_MAX);
        celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_NAME, CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME);
        celix_properties_set(props, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76");
        uuid_t uid;
        uuid_generate(uid);
        char endpointUUID[37];
        uuid_unparse_lower(uid, endpointUUID);
        celix_properties_set(props, CELIX_RSA_ENDPOINT_ID, endpointUUID);
        celix_properties_setBool(props, CELIX_RSA_SERVICE_IMPORTED, true);
        celix_properties_set(props, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE);
        endpoint_description_t* endpoint = nullptr;
        auto status = endpointDescription_create(props, &endpoint);
        EXPECT_EQ(status, CELIX_SUCCESS);
        return endpoint;
    }

    void TestAddMqttBrokerEndpoint(const std::function<void (endpoint_description_t*)>& modifyEndpoint) {
        std::promise<void> clientConnectedPromise;
        auto clientConnectedFuture = clientConnectedPromise.get_future();

        celix_earpm_client_create_options_t opts;
        CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);
        opts.callbackHandle = &clientConnectedPromise;
        opts.connectedCallback = [](void* handle){
            auto* promise = static_cast<std::promise<void>*>(handle);
            try {
                promise->set_value();
            } catch (...) {
                //ignore
            }
        };
        auto* client = celix_earpmClient_create(&opts);
        ASSERT_NE(client, nullptr);

        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();

        modifyEndpoint(endpoint);

        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(status, CELIX_SUCCESS);
        auto result = clientConnectedFuture.wait_for(std::chrono::seconds(30));
        ASSERT_EQ(result, std::future_status::ready);
        status = celix_earpmClient_mqttBrokerEndpointRemoved(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_earpmClient_destroy(client);
    }

    void TestClient(const std::function<void (celix_earpm_client_t*)>& testBody) {
        celix_earpm_client_create_options_t opts;
        CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);
        auto* client = celix_earpmClient_create(&opts);
        ASSERT_NE(client, nullptr);
        testBody(client);
        celix_earpmClient_destroy(client);
    }

    void TestClient(const std::function<void (celix_earpm_client_t*)>& testBody, const std::function<void (void)>& connectedCallback,
                              const std::function<void (const celix_earpm_client_request_info_t*)>& receiveMsgCallBack = [](const celix_earpm_client_request_info_t*){}) {
        celix_earpm_client_create_options_t opts;
        CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);
        struct callback_data {
            std::function<void (void)> connectedCallback;
            std::function<void (const celix_earpm_client_request_info_t*)> receiveMsgCallback;
        } callbackData{connectedCallback, receiveMsgCallBack};
        opts.callbackHandle = &callbackData;
        opts.connectedCallback = [](void* handle) {
            auto callbackData = static_cast<struct callback_data*>(handle);
            callbackData->connectedCallback();
        };
        opts.receiveMsgCallback = [](void* handle, const celix_earpm_client_request_info_t* requestInfo) {
            auto callbackData = static_cast<struct callback_data*>(handle);
            callbackData->receiveMsgCallback(requestInfo);
        };
        auto* client = celix_earpmClient_create(&opts);
        ASSERT_NE(client, nullptr);

        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(status, CELIX_SUCCESS);

        testBody(client);

        status = celix_earpmClient_mqttBrokerEndpointRemoved(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_earpmClient_destroy(client);
    }

    void TestClientAfterConnectedBroker(const std::function<void (celix_earpm_client_t*)>& testBody,
                                        const std::function<void (const celix_earpm_client_request_info_t*)>& receiveMsgCallBack = [](const celix_earpm_client_request_info_t*){}) {
        std::promise<void> clientConnectedPromise;
        auto clientConnectedFuture = clientConnectedPromise.get_future();
        TestClient([&clientConnectedFuture, testBody](celix_earpm_client_t* client) {
            auto result = clientConnectedFuture.wait_for(std::chrono::seconds(30));
            ASSERT_EQ(result, std::future_status::ready);
            testBody(client);
        }, [&clientConnectedPromise](void){
            try {
                clientConnectedPromise.set_value();
            } catch (...) {
                //ignore
            }
        }, receiveMsgCallBack);
    }

    static pid_t pid;
    static mosquitto* testMosqClient;

    std::shared_ptr<celix_log_helper_t> logHelper{};
};

pid_t CelixEarpmClientTestSuite::pid{0};
mosquitto* CelixEarpmClientTestSuite::testMosqClient{nullptr};

TEST_F(CelixEarpmClientTestSuite, CreateClientTest) {
    celix_earpm_client_create_options_t opts;
    CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);
    auto* client = celix_earpmClient_create(&opts);
    ASSERT_NE(client, nullptr);
    celix_earpmClient_destroy(client);
}

TEST_F(CelixEarpmClientTestSuite, CreateClientWithInvalidMsgQueueSizeTest) {
    celix_earpm_client_create_options_t opts;
    CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);

    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(CELIX_EARPM_MSG_QUEUE_MAX_SIZE+1).c_str(), 1);
    auto client = celix_earpmClient_create(&opts);
    ASSERT_EQ(client, nullptr);

    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(0).c_str(), 1);
    client = celix_earpmClient_create(&opts);
    ASSERT_EQ(client, nullptr);

    unsetenv(CELIX_EARPM_MSG_QUEUE_CAPACITY);
}

TEST_F(CelixEarpmClientTestSuite, CreateClientWithInvalidParallelMsgSizeTest) {
    celix_earpm_client_create_options_t opts;
    CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);

    const int msgQueueCapacity = 256;
    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(msgQueueCapacity).c_str(), 1);
    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, std::to_string(msgQueueCapacity+1).c_str(), 1);
    auto client = celix_earpmClient_create(&opts);
    ASSERT_EQ(client, nullptr);

    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, std::to_string(0).c_str(), 1);
    client = celix_earpmClient_create(&opts);
    ASSERT_EQ(client, nullptr);

    unsetenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY);
    unsetenv(CELIX_EARPM_MSG_QUEUE_CAPACITY);
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerSpecifiedIpEndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerSpecifiedHostNameEndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "localhost");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerDynamicIpEndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "127.0.0.1,::1");
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerDynamicIpv4EndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "127.0.0.1,::1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, AF_INET);
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerDynamicIpv6EndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "127.0.0.1,::1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, AF_INET6);
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerSpecifiedHostNameAndSockDomainEndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "localhost");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, AF_INET);
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, AddMqttBrokerSpecifiedInterfaceEndpointTest) {
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "");
        celix_properties_set(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, "lo");
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, MQTT_BROKER_PORT);
    });
}

TEST_F(CelixEarpmClientTestSuite, MqttBrokerEndpointWithInvalidPortTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, -1);
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, MqttBrokerDynamicIpEndpointWithInvalidPortTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, -1);
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, MQTT_BROKER_ADDRESS);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, MqttBrokerEndpointWithoutAddressTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, MqttBrokerEndpointWithEmptyAddressTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, MQTT_BROKER_PORT);
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "");
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, MqttBrokerEndpointWithInvalidSocketDomainTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, AF_INET6);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        EXPECT_EQ(status, EINVAL);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncQos0MsgWhenUnconnetToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, ENOTCONN);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncQos1MsgWhenUnconnetToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncQos2MsgWhenUnconnetToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_EXACTLY_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncLowPriorityMsgWhenMsgQueueHungryTest) {
    const int msgQueueCapacity = 4;
    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(msgQueueCapacity).c_str(), 1);
    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, "1", 1);
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
        for (int i = 0; i < msgQueueCapacity*70/100; ++i) {
            auto status = celix_earpmClient_publishAsync(client, &requestInfo);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }

        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, ENOMEM);
    });
    unsetenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY);
    unsetenv(CELIX_EARPM_MSG_QUEUE_CAPACITY);
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncMiddlePriorityMsgWhenMsgQueueHungryTest) {
    const int msgQueueCapacity = 4;
    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(msgQueueCapacity).c_str(), 1);
    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, "1", 1);
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
        for (int i = 0; i < msgQueueCapacity*85/100; ++i) {
            auto status = celix_earpmClient_publishAsync(client, &requestInfo);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }
        requestInfo.pri = CELIX_EARPM_MSG_PRI_MIDDLE;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, ENOMEM);
    });
    unsetenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY);
    unsetenv(CELIX_EARPM_MSG_QUEUE_CAPACITY);
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncHighPriorityMsgWhenMsgQueueHungryTest) {
    const int msgQueueCapacity = 4;
    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(msgQueueCapacity).c_str(), 1);
    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, "1", 1);
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
        for (int i = 0; i < msgQueueCapacity; ++i) {
            auto status = celix_earpmClient_publishAsync(client, &requestInfo);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }

        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, ENOMEM);
    });
    unsetenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY);
    unsetenv(CELIX_EARPM_MSG_QUEUE_CAPACITY);
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncQos0MsgWhenUnconnetToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 30;
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, ENOTCONN);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncQos1MsgWhenUnconnetToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 0.01;
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, ETIMEDOUT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncQos2MsgWhenUnconnetToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_EXACTLY_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 0.01;
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, ETIMEDOUT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncQOS0MsgTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 30;
        requestInfo.responseTopic = "test/response/topic";
        long correlationData = 0;
        requestInfo.correlationData = &correlationData;
        requestInfo.correlationDataSize = sizeof(correlationData);
        requestInfo.senderUUID = "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76";
        requestInfo.version = "1.0.0";
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncQOS1MsgTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 30;
        requestInfo.responseTopic = "test/response/topic";
        long correlationData = 0;
        requestInfo.correlationData = &correlationData;
        requestInfo.correlationDataSize = sizeof(correlationData);
        requestInfo.senderUUID = "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76";
        requestInfo.version = "1.0.0";
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncQOS2MsgTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_EXACTLY_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 30;
        requestInfo.responseTopic = "test/response/topic";
        long correlationData = 0;
        requestInfo.correlationData = &correlationData;
        requestInfo.correlationDataSize = sizeof(correlationData);
        requestInfo.senderUUID = "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76";
        requestInfo.version = "1.0.0";
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncMsgWithInvalidTopicTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = nullptr;
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncMsgWithInvalidQosTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.qos = CELIX_EARPM_QOS_UNKNOWN;
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
        requestInfo.qos = CELIX_EARPM_QOS_MAX;
        status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncMsgWithInvalidPriorityTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.pri = (celix_earpm_client_message_priority_e)(CELIX_EARPM_MSG_PRI_LOW - 1);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
        requestInfo.pri = (celix_earpm_client_message_priority_e)(CELIX_EARPM_MSG_PRI_HIGH + 1);
        status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncMsgWhenMsgQueueHungryTest) {
    const int msgQueueCapacity = 4;
    setenv(CELIX_EARPM_MSG_QUEUE_CAPACITY, std::to_string(msgQueueCapacity).c_str(), 1);
    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, "1", 1);

    std::promise<void> clientConnectedPromise;
    auto clientConnectedFuture = clientConnectedPromise.get_future();
    std::promise<void> mqttThreadContinuePromise;
    auto mqttThreadContinueFuture = mqttThreadContinuePromise.get_future();

    TestClient([&clientConnectedFuture, &mqttThreadContinuePromise](celix_earpm_client_t* client) {
        //wait for the client connected
        auto result = clientConnectedFuture.wait_for(std::chrono::seconds(30));
        ASSERT_EQ(result, std::future_status::ready);

        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
        for (int i = 0; i < msgQueueCapacity*70/100; ++i) {
            auto status = celix_earpmClient_publishAsync(client, &requestInfo);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }

        //QOS0 msg
        requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 0.01;
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, ENOMEM);

        //QOS1 msg
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.expiryInterval = 0.01;
        status = celix_earpmClient_publishSync(client,&requestInfo);
        EXPECT_EQ(status, ETIMEDOUT);

        //QOS2 msg
        requestInfo.qos = CELIX_EARPM_QOS_EXACTLY_ONCE;
        requestInfo.expiryInterval = 0.01;
        status = celix_earpmClient_publishSync(client, &requestInfo);
        EXPECT_EQ(status, ETIMEDOUT);

        mqttThreadContinuePromise.set_value();// notify mqtt thread continue
    },[&clientConnectedPromise, &mqttThreadContinueFuture](){
        clientConnectedPromise.set_value();// notify client connected

        //wait for mqtt thread continue
        auto result = mqttThreadContinueFuture.wait_for(std::chrono::seconds(30));
        ASSERT_EQ(result, std::future_status::ready);
    });
    unsetenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY);
    unsetenv(CELIX_EARPM_MSG_QUEUE_CAPACITY);
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncQos0MsgTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76";
        requestInfo.version = "1.0.0";
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncQos1MsgTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76";
        requestInfo.version = "1.0.0";
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncQos2MsgTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_EXACTLY_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76";
        requestInfo.version = "1.0.0";
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncMsgWithInvalidTopicTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = nullptr;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncMsgWithInvalidQosTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.qos = CELIX_EARPM_QOS_UNKNOWN;
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
        requestInfo.qos = CELIX_EARPM_QOS_MAX;
        status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncMsgWithInvalidPriorityTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.pri = (celix_earpm_client_message_priority_e)(CELIX_EARPM_MSG_PRI_LOW - 1);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
        requestInfo.pri = (celix_earpm_client_message_priority_e)(CELIX_EARPM_MSG_PRI_HIGH + 1);
        status = celix_earpmClient_publishAsync(client, &requestInfo);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishMsgBeforeConnectedTest) {
    TestClient([](celix_earpm_client_t* client) {
        std::future<void> future = std::async(std::launch::async, [client](){
            celix_earpm_client_request_info_t requestInfo;
            memset(&requestInfo, 0, sizeof(requestInfo));
            requestInfo.topic = "test/topic";
            requestInfo.payload = "test";
            requestInfo.payloadSize = 4;
            requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
            requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
            requestInfo.expiryInterval = 30;
            auto status = celix_earpmClient_publishSync(client, &requestInfo);
            EXPECT_EQ(status, CELIX_SUCCESS);
        });

        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        future.get();//wait for the published msg complete

        status = celix_earpmClient_mqttBrokerEndpointRemoved(client, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}


TEST_F(CelixEarpmClientTestSuite, PublishAsyncMsgAndThenClientDestroyTest) {
    std::promise<void> clientConnectedPromise;
    auto clientConnectedFuture = clientConnectedPromise.get_future();
    std::promise<void> msgPublishedPromise;
    auto msgPublishedFuture = msgPublishedPromise.get_future();
    TestClient([&clientConnectedFuture,&msgPublishedPromise](celix_earpm_client_t* client) {
        //wait for the client connected
        auto result = clientConnectedFuture.wait_for(std::chrono::seconds(30));
        ASSERT_EQ(result, std::future_status::ready);

        celix_earpm_client_request_info_t requestInfo;
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test";
        requestInfo.payloadSize = 4;
        requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
        for (int i = 0; i < CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT; ++i) {
            auto status = celix_earpmClient_publishAsync(client, &requestInfo);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }
        msgPublishedPromise.set_value();// notify the message published
    },[&clientConnectedPromise, &msgPublishedFuture](void){
        clientConnectedPromise.set_value();// notify client connected

        msgPublishedFuture.get();//wait for the message published
    });
}

TEST_F(CelixEarpmClientTestSuite, SubscribeAndUnsubscribeWhenUnconnectedToBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_earpmClient_unsubscribe(client, "test/topic");
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, SubscribeMessageWithSpecialTopicTest) {
    std::promise<void> receivedPromise;
    auto receivedFuture = receivedPromise.get_future();

    TestClientAfterConnectedBroker([&receivedFuture](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/topic1", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);
        int rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/topic1", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                      true, nullptr);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto result = receivedFuture.wait_for(std::chrono::seconds(30));
        EXPECT_EQ(result, std::future_status::ready);
        status = celix_earpmClient_unsubscribe(client, "test/topic1");
        EXPECT_EQ(status, CELIX_SUCCESS);
    }, [&receivedPromise](const celix_earpm_client_request_info_t* requestInfo){
        EXPECT_STREQ(requestInfo->topic, "test/topic1");
        EXPECT_STREQ(requestInfo->payload, "test");
        try {
            receivedPromise.set_value();
        } catch (...) {
            //ignore
        }
    });
}

TEST_F(CelixEarpmClientTestSuite, SubscribeMessageWithPatternTopicTest) {
    std::promise<void> receivedPromise;
    auto receivedFuture = receivedPromise.get_future();

    TestClientAfterConnectedBroker([&receivedFuture](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test1/*", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);
        int rc = mosquitto_publish_v5(testMosqClient, nullptr, "test1/topic1", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                      true, nullptr);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto result = receivedFuture.wait_for(std::chrono::seconds(30));
        EXPECT_EQ(result, std::future_status::ready);

        status = celix_earpmClient_unsubscribe(client, "test1/*");
        EXPECT_EQ(status, CELIX_SUCCESS);
    }, [&receivedPromise](const celix_earpm_client_request_info_t* requestInfo){
        EXPECT_STREQ(requestInfo->topic, "test1/topic1");
        EXPECT_STREQ(requestInfo->payload, "test");
        try {
            receivedPromise.set_value();
        } catch (...) {
            //ignore
        }
    });
}

TEST_F(CelixEarpmClientTestSuite, SubscribeBeforeConnectedToBrokerTest) {
    std::promise<void> receivedPromise;
    auto receivedFuture = receivedPromise.get_future();
    celix_earpm_client_create_options_t opts;
    CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);
    opts.callbackHandle = &receivedPromise;
    opts.connectedCallback = [](void*) { };
    opts.receiveMsgCallback = [](void* handle, const celix_earpm_client_request_info_t* requestInfo) {
        auto receivedPromise = static_cast<std::promise<void>*>(handle);
        EXPECT_STREQ(requestInfo->topic, "test/topic2");
        EXPECT_STREQ(requestInfo->payload, "test");
        try {
            receivedPromise->set_value();
        } catch (...) {
            //ignore
        }
    };
    auto* client = celix_earpmClient_create(&opts);
    ASSERT_NE(client, nullptr);

    auto status = celix_earpmClient_subscribe(client, "test/topic2", CELIX_EARPM_QOS_AT_LEAST_ONCE);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
    celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
    celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
    status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
    ASSERT_EQ(status, CELIX_SUCCESS);

    int rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/topic2", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                  true, nullptr);
    EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);

    auto result = receivedFuture.wait_for(std::chrono::seconds(30));
    EXPECT_EQ(result, std::future_status::ready);

    status = celix_earpmClient_mqttBrokerEndpointRemoved(client, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpmClient_destroy(client);
}

TEST_F(CelixEarpmClientTestSuite, UnsubscribeBeforeConnectedToBrokerTest) {
    std::promise<void> receivedPromise;
    auto receivedFuture = receivedPromise.get_future();
    celix_earpm_client_create_options_t opts;
    CELIX_EARPM_CLIENT_CREATE_OPTIONS_INIT(opts);
    opts.callbackHandle = &receivedPromise;
    opts.connectedCallback = [](void*) { };
    opts.receiveMsgCallback = [](void* handle, const celix_earpm_client_request_info_t*) {
        auto receivedPromise = static_cast<std::promise<void>*>(handle);
        receivedPromise->set_value();
    };
    auto* client = celix_earpmClient_create(&opts);
    ASSERT_NE(client, nullptr);

    auto status = celix_earpmClient_subscribe(client, "test/topic2", CELIX_EARPM_QOS_AT_LEAST_ONCE);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_earpmClient_unsubscribe(client, "test/topic2");
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
    celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
    celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
    status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
    ASSERT_EQ(status, CELIX_SUCCESS);

    int rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/topic2", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                  true, nullptr);
    EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);

    auto result = receivedFuture.wait_for(std::chrono::milliseconds(10));
    EXPECT_EQ(result, std::future_status::timeout);

    status = celix_earpmClient_mqttBrokerEndpointRemoved(client, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpmClient_destroy(client);
}

TEST_F(CelixEarpmClientTestSuite, SubscribeWithInvalidTopicTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        std::string topic(1024 + 1, 'a');
        status = celix_earpmClient_subscribe(client, topic.c_str(), CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpmClient_subscribe(client, "$a", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpmClient_subscribe(client, "+a", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpmClient_subscribe(client, "#a", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, UnsubscribeWithInvalidTopicTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_unsubscribe(client, "");
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        std::string topic(1024 + 1, 'a');
        status = celix_earpmClient_unsubscribe(client, topic.c_str());
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpmClient_unsubscribe(client, "$a");
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpmClient_unsubscribe(client, "+a");
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpmClient_unsubscribe(client, "#a");
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmClientTestSuite, ReceiveMessageTest) {
    std::promise<void> receivedPromise;
    auto receivedFuture = receivedPromise.get_future();

    TestClientAfterConnectedBroker([&receivedFuture](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/message", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_autoptr(mosquitto_property) props = nullptr;
        auto rc = mosquitto_property_add_int32(&props, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, 10);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_property_add_string(&props, MQTT_PROP_RESPONSE_TOPIC, "test/response");
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_property_add_binary(&props, MQTT_PROP_CORRELATION_DATA, "1234", 4);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_property_add_string_pair(&props, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76");
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_property_add_string_pair(&props, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_MSG_VERSION", "1.0.0");
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/message", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                      true, props);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto result = receivedFuture.wait_for(std::chrono::seconds(30));
        EXPECT_EQ(result, std::future_status::ready);
        status = celix_earpmClient_unsubscribe(client, "test/message");
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear retained message
        rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/message", 0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                  true, props);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
    }, [&receivedPromise](const celix_earpm_client_request_info_t* requestInfo){
        EXPECT_STREQ(requestInfo->topic, "test/message");
        if (requestInfo->payload != nullptr) {
            EXPECT_STREQ(requestInfo->payload, "test");
            EXPECT_EQ(requestInfo->payloadSize, sizeof("test"));
        }
        EXPECT_EQ(requestInfo->qos, CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(requestInfo->pri, CELIX_EARPM_MSG_PRI_LOW);
        EXPECT_EQ(requestInfo->expiryInterval, 10);
        EXPECT_STREQ(requestInfo->responseTopic, "test/response");
        EXPECT_EQ(requestInfo->correlationDataSize, 4);
        EXPECT_EQ(memcmp(requestInfo->correlationData, "1234", 4), 0);
        EXPECT_STREQ(requestInfo->senderUUID, "9d5b0a58-1e6f-4c4f-bd00-7298914b1a76");
        EXPECT_STREQ(requestInfo->version, "1.0.0");
        try {
            receivedPromise.set_value();
        } catch (...) {
            //ignore
        }
    });
}

TEST_F(CelixEarpmClientTestSuite, ReconnectBrokerTest) {
    pid_t pid1 = fork();
    ASSERT_GE(pid1, 0);
    if (pid1 == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));//let the parent run first
        execlp("mosquitto", "mosquitto1", "-p","1884", nullptr);
        ADD_FAILURE() << "Failed to start mosquitto";
    }
    TestAddMqttBrokerEndpoint([](endpoint_description_t* endpoint){
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1884);
    });

    kill(pid1, SIGKILL);
    waitpid(pid1, nullptr, 0);
}
