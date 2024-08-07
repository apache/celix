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
#include <functional>
#include <future>
#include <mqtt_protocol.h>
#include <mosquitto.h>

#include "celix_earpm_client.h"
#include "celix_earpm_constants.h"
#include "celix_mqtt_broker_info_service.h"
#include "CelixEarpmTestSuiteBaseClass.h"
#include "celix_log_helper.h"
#include "celix_threads.h"

static constexpr const char* MQTT_BROKER_ADDRESS = "127.0.0.1";
static constexpr int MQTT_BROKER_PORT = 1883;
static constexpr const char* MQTT_BROKER_START_ARG = "-p 1883";

class CelixEarpmClientTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    static void SetUpTestSuite() {
        mosquitto_lib_init();
        pid = fork();
        ASSERT_GE(pid, 0);
        if (pid == 0) {
            execlp("mosquitto", MQTT_BROKER_START_ARG, nullptr);
            ADD_FAILURE() << "Failed to start mosquitto";
        }
        testMosq = mosquitto_new(nullptr, true, nullptr);
        ASSERT_NE(testMosq, nullptr);
        for (int i = 0; i < 60; ++i) {
            if(mosquitto_connect_bind_v5(testMosq, MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT, 60, nullptr, nullptr) == MOSQ_ERR_SUCCESS) {
                int rc = mosquitto_loop_start(testMosq);
                ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
                return;
            }
            sleep(1);
        }
        ADD_FAILURE() << "Failed to init test environment";
    }

    static void TearDownTestSuite() {
        mosquitto_disconnect(testMosq);
        mosquitto_loop_stop(testMosq, false);
        mosquitto_destroy(testMosq);
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        mosquitto_lib_cleanup();
    }

    CelixEarpmClientTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_client_test_cache"} {
        logHelper = std::shared_ptr <celix_log_helper_t>
                {celix_logHelper_create(ctx.get(), "EarpmClient"), [](celix_log_helper_t *h) { celix_logHelper_destroy(h); }};
    }

    ~CelixEarpmClientTestSuite() override = default;

    void TestClient(std::function<void (celix_earpm_client_t*)>testBody) {
        celix_earpm_client_create_options_t opts;
        memset(&opts, 0, sizeof(opts));
        opts.ctx = ctx.get();
        opts.logHelper = logHelper.get();
        opts.sessionEndTopic = "session_expiry";
        mosquitto_property_add_string_pair(&opts.sessionEndProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, fwUUID.c_str());
        opts.callbackHandle = nullptr;
        opts.receiveMsgCallback = [](void*, const char*, const char*, size_t, const mosquitto_property*){};
        opts.connectedCallback = [](void*) {};
        auto* client = celix_earpmClient_create(&opts);
        ASSERT_NE(client, nullptr);
        testBody(client);
        celix_earpmClient_destroy(client);
    }

    void TestClientWithBroker(std::function<void (celix_earpm_client_t*)>testBody, std::function<void (void*)> connectedCallback = [](void*){},
                              std::function<void (void*, const char*, const char*, size_t, const mosquitto_property*)> receiveMsgCallBack = [](void*, const char*, const char*, size_t, const mosquitto_property*){}) {
        celix_earpm_client_create_options_t opts;
        memset(&opts, 0, sizeof(opts));
        opts.ctx = ctx.get();
        opts.logHelper = logHelper.get();
        opts.sessionEndTopic = "session_expiry";
        mosquitto_property_add_string_pair(&opts.sessionEndProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, fwUUID.c_str());
        struct callback_data {
            std::function<void (void*)> connectedCallback;
            std::function<void (void*, const char*, const char*, size_t, const mosquitto_property*)> receiveMsgCallback;
        } callbackData{connectedCallback, receiveMsgCallBack};
        opts.callbackHandle = &callbackData;
        opts.connectedCallback = [](void* handle) {
            auto callbackData = static_cast<struct callback_data*>(handle);
            callbackData->connectedCallback(nullptr);
        };
        opts.receiveMsgCallback = [](void* handle, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property *props) {
            auto callbackData = static_cast<struct callback_data*>(handle);
            callbackData->receiveMsgCallback(nullptr, topic, payload, payloadSize, props);
        };
        auto* client = celix_earpmClient_create(&opts);
        ASSERT_NE(client, nullptr);

        celix_mqtt_broker_info_service_t brokerInfo;
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
        celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props.get(), CELIX_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        auto status = celix_earpmc_addBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SUCCESS);

        testBody(client);

        status = celix_earpmc_removeBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_earpmClient_destroy(client);
    }

    static pid_t pid;
    static mosquitto* testMosq;

    std::shared_ptr<celix_log_helper_t> logHelper{};
};

pid_t CelixEarpmClientTestSuite::pid{0};
mosquitto* CelixEarpmClientTestSuite::testMosq{nullptr};

TEST_F(CelixEarpmClientTestSuite, CreateClientWithoutSessionExpiryMsgTest) {
    celix_earpm_client_create_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.ctx = ctx.get();
    opts.logHelper = logHelper.get();
    opts.callbackHandle = nullptr;
    opts.receiveMsgCallback = [](void*, const char*, const char*, size_t, const mosquitto_property*) {};
    opts.connectedCallback = [](void*) {};
    auto* client = celix_earpmClient_create(&opts);
    ASSERT_NE(client, nullptr);
    celix_earpmClient_destroy(client);
}

TEST_F(CelixEarpmClientTestSuite, CreateClientWithSessionExpiryMsgTest) {
    celix_earpm_client_create_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.ctx = ctx.get();
    opts.logHelper = logHelper.get();
    opts.sessionEndTopic = "session_expiry";
    mosquitto_property_add_string_pair(&opts.sessionEndProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, fwUUID.c_str());
    opts.callbackHandle = nullptr;
    opts.receiveMsgCallback = [](void*, const char*, const char*, size_t, const mosquitto_property*) {};
    opts.connectedCallback = [](void*) {};
    auto* client = celix_earpmClient_create(&opts);
    ASSERT_NE(client, nullptr);
    celix_earpmClient_destroy(client);
}

TEST_F(CelixEarpmClientTestSuite, AddBrokerInfoTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_mqtt_broker_info_service_t brokerInfo;
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
        celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props.get(), CELIX_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        auto status = celix_earpmc_addBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_earpmc_removeBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, BrokerInfoSeriveIdInvalidTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_mqtt_broker_info_service_t brokerInfo;
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
        celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, -1);
        celix_properties_set(props.get(), CELIX_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        auto status = celix_earpmc_addBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

        status = celix_earpmc_removeBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);
    });
}

TEST_F(CelixEarpmClientTestSuite, BrokerInfoLostBrokerAddressTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_mqtt_broker_info_service_t brokerInfo;
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
        celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        auto status = celix_earpmc_addBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);
    });
}

TEST_F(CelixEarpmClientTestSuite, BrokerPortInvalidTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_mqtt_broker_info_service_t brokerInfo;
        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
        celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props.get(), CELIX_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, -1);
        auto status = celix_earpmc_addBrokerInfoService(client, &brokerInfo, props.get());
        EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncWithQos0MsgWhenDisconnetBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                     nullptr, CELIX_EARPM_MSG_PRI_LOW);
        EXPECT_EQ(status, ENOTCONN);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncWithQos1MsgWhenDisconnetBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                                     nullptr, CELIX_EARPM_MSG_PRI_LOW);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncWithQos2MsgWhenDisconnetBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_EXACTLY_ONCE,
                                                     nullptr, CELIX_EARPM_MSG_PRI_LOW);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncWithReplacedIfInQueueFlagTrueWhenDisconnetBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                                     nullptr, CELIX_EARPM_MSG_PRI_LOW);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE, nullptr,
                                                CELIX_EARPM_MSG_PRI_LOW);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncWithQos0MsgWhenDisconnetBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        struct timespec timeout = celixThreadCondition_getDelayedTime(30);
        auto status = celix_earpmClient_publishSync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                    nullptr, &timeout);
        EXPECT_EQ(status, ENOTCONN);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncWithQOS0MsgTest) {
    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    TestClientWithBroker([&connectedFuture](celix_earpm_client_t* client) {
        connectedFuture.get();
        struct timespec timeout = celixThreadCondition_getDelayedTime(30);
        auto status = celix_earpmClient_publishSync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                    nullptr, &timeout);
        EXPECT_EQ(status, CELIX_SUCCESS);
    },[&connectedPromise](void*){
        connectedPromise.set_value();
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishSyncWithQOS1MsgTest) {
    TestClientWithBroker([](celix_earpm_client_t* client) {
        struct timespec timeout = celixThreadCondition_getDelayedTime(30);
        auto status = celix_earpmClient_publishSync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                                    nullptr, &timeout);
        EXPECT_EQ(status, CELIX_SUCCESS);
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishMsgWhenMsgQueueFull) {
    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    std::promise<void> mqttThreadContinuePromise;
    auto mqttThreadContinueFuture = mqttThreadContinuePromise.get_future();
    TestClientWithBroker([&connectedFuture, &mqttThreadContinuePromise](celix_earpm_client_t* client) {
        connectedFuture.get();
        for (int i = 0; i < CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT; ++i) {
            auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                         nullptr, CELIX_EARPM_MSG_PRI_HIGH);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }
        //publish an asynchronous QOS0 message when msg queue is full
        auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                     nullptr, CELIX_EARPM_MSG_PRI_HIGH);
        EXPECT_EQ(status, ENOMEM);

        //publish an asynchronous QOS1 message when msg queue is full
        status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE, nullptr,
                                                CELIX_EARPM_MSG_PRI_HIGH);
        EXPECT_EQ(status, ENOMEM);

        //publish a synchronous QOS0 message when msg queue is full
        struct timespec timeout = celixThreadCondition_getDelayedTime(30);
        status = celix_earpmClient_publishSync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE, nullptr,
                                               &timeout);
        EXPECT_EQ(status, ENOMEM);

        //publish a synchronous QOS1 message when msg queue is full
        timeout = celixThreadCondition_getDelayedTime(1);
        status = celix_earpmClient_publishSync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE, nullptr,
                                               &timeout);
        EXPECT_EQ(status, ETIMEDOUT);

        mqttThreadContinuePromise.set_value();
    },[&connectedPromise, &mqttThreadContinueFuture](void*){
        connectedPromise.set_value();
        mqttThreadContinueFuture.get();
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncWithQOS0MsgAndThenClientDestroyTest) {
    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    std::promise<void> msgPublishedPromise;
    auto msgPublishedFuture = msgPublishedPromise.get_future();
    TestClientWithBroker([&connectedFuture,&msgPublishedPromise](celix_earpm_client_t* client) {
        connectedFuture.get();
        for (int i = 0; i < CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT; ++i) {
            auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                         nullptr, CELIX_EARPM_MSG_PRI_HIGH);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }
        msgPublishedPromise.set_value();
    },[&connectedPromise, &msgPublishedFuture](void*){
        connectedPromise.set_value();
        msgPublishedFuture.get();
    });
}

TEST_F(CelixEarpmClientTestSuite, PublishAsyncWithQOS1MsgAndThenClientDestroyTest) {
    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    std::promise<void> msgPublishedPromise;
    auto msgPublishedFuture = msgPublishedPromise.get_future();
    TestClientWithBroker([&connectedFuture,&msgPublishedPromise](celix_earpm_client_t* client) {
        connectedFuture.get();
        for (int i = 0; i < CELIX_EARPM_MSG_QUEUE_CAPACITY_DEFAULT; ++i) {
            auto status = celix_earpmClient_publishAsync(client, "test/topic", "test", 4, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                                         nullptr, CELIX_EARPM_MSG_PRI_HIGH);
            EXPECT_EQ(status, CELIX_SUCCESS);
        }
        msgPublishedPromise.set_value();
    },[&connectedPromise, &msgPublishedFuture](void*){
        connectedPromise.set_value();
        msgPublishedFuture.get();
    });
}

TEST_F(CelixEarpmClientTestSuite, SubcribeAndUnsubcribeAfterConnectedToBrokerTest) {
    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    TestClientWithBroker([&connectedFuture](celix_earpm_client_t* client) {
        connectedFuture.get();
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_earpmClient_unsubscribe(client, "test/topic");
        EXPECT_EQ(status, CELIX_SUCCESS);
    },[&connectedPromise](void*){
        connectedPromise.set_value();
    });
}

TEST_F(CelixEarpmClientTestSuite, SubcribeAndUnsubcribeBeforeConnectedToBrokerTest) {
    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    celix_earpm_client_create_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.ctx = ctx.get();
    opts.logHelper = logHelper.get();
    opts.sessionEndTopic = "session_expiry";
    mosquitto_property_add_string_pair(&opts.sessionEndProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, fwUUID.c_str());
    opts.callbackHandle = &connectedPromise;
    opts.connectedCallback = [](void* handle) {
        auto connectedPromise = static_cast<std::promise<void>*>(handle);
        connectedPromise->set_value();
    };
    opts.receiveMsgCallback = [](void*, const char*, const char*, size_t, const mosquitto_property*) {};
    auto* client = celix_earpmClient_create(&opts);
    ASSERT_NE(client, nullptr);

    auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_LEAST_ONCE);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_earpmClient_unsubscribe(client, "test/topic");
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_mqtt_broker_info_service_t brokerInfo;
    std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
    celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, 123);
    celix_properties_set(props.get(), CELIX_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
    celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
    status = celix_earpmc_addBrokerInfoService(client, &brokerInfo, props.get());
    EXPECT_EQ(status, CELIX_SUCCESS);

    connectedFuture.get();

    status = celix_earpmc_removeBrokerInfoService(client, &brokerInfo, props.get());
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpmClient_destroy(client);
}

TEST_F(CelixEarpmClientTestSuite, SubscribeWithInvalidTopic) {
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

TEST_F(CelixEarpmClientTestSuite, SubscribeMessageWithSpecialTopicTest) {
    TestClientWithBroker([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);
        usleep(100000);//wait for subscribing to be processed
        int rc = mosquitto_publish_v5(testMosq, nullptr, "test/topic", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE, false, nullptr);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        usleep(100000);//wait for receiving the message
    },[](void*){},[](void*, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property*) {
        EXPECT_STREQ(topic, "test/topic");
        EXPECT_STREQ(payload, "test");
        EXPECT_EQ(payloadSize, sizeof("test"));
    });
}

TEST_F(CelixEarpmClientTestSuite, SubscribeMessageWithPatternTopicTest) {
    TestClientWithBroker([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/*", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);
        usleep(100000);//wait for subscribing to be processed
        int rc = mosquitto_publish_v5(testMosq, nullptr, "test/topic1", sizeof("test"), "test", CELIX_EARPM_QOS_AT_LEAST_ONCE, false, nullptr);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        usleep(100000);//wait for receiving the message
    },[](void*){},[](void*, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property*) {
        EXPECT_STREQ(topic, "test/topic1");
        EXPECT_STREQ(payload, "test");
        EXPECT_EQ(payloadSize, sizeof("test"));
    });
}