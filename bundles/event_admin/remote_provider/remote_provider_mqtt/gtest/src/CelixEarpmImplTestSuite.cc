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
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <mqtt_protocol.h>
#include <mosquitto.h>
#include <uuid/uuid.h>

#include "celix_earpm_impl.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_broker_discovery.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "celix_event_handler_service.h"
#include "celix_event_admin_service.h"
#include "celix_event_constants.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "CelixEarpmTestSuiteBaseClass.h"
#include "celix_stdlib_cleanup.h"

namespace {
constexpr const char* MQTT_BROKER_ADDRESS = "127.0.0.1";
constexpr int MQTT_BROKER_PORT = 1883;
constexpr const char* FAKE_FW_UUID = "5936e9f4-c4a8-4fa8-b070-65d03a6d4d03";
}

class MqttClient {
public:
    explicit MqttClient(std::vector<std::string> subTopics) : subTopics{std::move(subTopics)} {
        mosq = std::shared_ptr<mosquitto>{mosquitto_new(nullptr, true, this), [](mosquitto* m) { mosquitto_destroy(m); }};
        EXPECT_NE(mosq, nullptr);
        auto rc = mosquitto_int_option(mosq.get(), MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        mosquitto_connect_callback_set(mosq.get(), [](mosquitto*, void* handle, int rc) {
            auto client = static_cast<MqttClient*>(handle);
            ASSERT_EQ(rc, MQTT_RC_SUCCESS);
            for (const auto& topic : client->subTopics) {
                auto ret = mosquitto_subscribe_v5(client->mosq.get(), nullptr, topic.c_str(), 0, MQTT_SUB_OPT_NO_LOCAL, nullptr);
                ASSERT_EQ(ret, MOSQ_ERR_SUCCESS);
            }
        });
        mosquitto_subscribe_v5_callback_set(mosq.get(), [](mosquitto*, void* handle, int, int, const int*, const mosquitto_property*) {
            auto client = static_cast<MqttClient*>(handle);
            std::lock_guard<std::mutex> lock(client->mutex);
            client->subscribedCnt++;
            client->subscribedCond.notify_all();
        });
        mosquitto_message_v5_callback_set(mosq.get(), [](mosquitto*, void* handle, const mosquitto_message* msg, const mosquitto_property* props) {
            auto client = static_cast<MqttClient*>(handle);
            {
                std::lock_guard<std::mutex> lock(client->mutex);
                client->receivedMsgTopics.emplace_back(msg->topic);
                client->receivedMsgCond.notify_all();
            }
            celix_autofree char* responseTopic{nullptr};
            celix_autofree void* correlationData{nullptr};
            uint16_t correlationDataSize = 0;
            mosquitto_property_read_string(props, MQTT_PROP_RESPONSE_TOPIC, &responseTopic, false);
            mosquitto_property_read_binary(props, MQTT_PROP_CORRELATION_DATA, &correlationData, &correlationDataSize, false);
            if (responseTopic != nullptr) {
                celix_autoptr(mosquitto_property) responseProps{nullptr};
                if (correlationData) {
                    auto rc = mosquitto_property_add_binary(&responseProps, MQTT_PROP_CORRELATION_DATA, correlationData, correlationDataSize);
                    ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
                }
                auto rc = mosquitto_property_add_string_pair(&responseProps, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
                ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
                rc = mosquitto_publish_v5(client->mosq.get(), nullptr, responseTopic, 0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, responseProps);
                ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
            }
        });
    }

    ~MqttClient() = default;

    void MqttClientStart() {
        for (int i = 0; i < 512; ++i) {
            if(mosquitto_connect_bind_v5(mosq.get(), MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT, 60, nullptr, nullptr) == MOSQ_ERR_SUCCESS) {
                auto rc = mosquitto_loop_start(mosq.get());
                ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
                std::unique_lock<std::mutex> lock(mutex);
                auto subscribed = subscribedCond.wait_for(lock, std::chrono::seconds{60}, [this] { return subscribedCnt >= subTopics.size(); });
                ASSERT_TRUE(subscribed);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds {100});
        }
        ADD_FAILURE() << "Failed to start mqtt client";
    }

    void MqttClientStop() {
        mosquitto_disconnect(mosq.get());
        mosquitto_loop_stop(mosq.get(), false);
    }

    bool WaitForReceivedMsg(const std::string& topic, std::chrono::milliseconds timeout = std::chrono::milliseconds{30*1000}) {
        std::unique_lock<std::mutex> lock(mutex);
        return receivedMsgCond.wait_for(lock, timeout, [&topic, this] {
            return std::find(receivedMsgTopics.rbegin(), receivedMsgTopics.rend(), topic) != receivedMsgTopics.rend();
        });
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(mutex);
        receivedMsgTopics.clear();
    }

    std::shared_ptr<mosquitto> mosq{nullptr};
    std::vector<std::string> subTopics{};
    std::mutex mutex{};
    std::condition_variable subscribedCond{};
    size_t subscribedCnt{0};
    std::condition_variable receivedMsgCond{};
    std::vector<std::string> receivedMsgTopics{};

};

class CelixEarpmImplTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    static void SetUpTestSuite() {
        mosquitto_lib_init();
        pid = fork();
        ASSERT_GE(pid, 0);
        if (pid == 0) {
            execlp("mosquitto", "mosquitto", "-p", std::to_string(MQTT_BROKER_PORT).c_str(), nullptr);
            ADD_FAILURE() << "Failed to start mosquitto";
        }
        mqttClient = new MqttClient{std::vector<std::string>{"subscribedEvent", CELIX_EARPM_TOPIC_PREFIX"#"}};
        mqttClient->MqttClientStart();
    }

    static void TearDownTestSuite() {
        mqttClient->MqttClientStop();
        delete mqttClient;
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        mosquitto_lib_cleanup();
    }

    void SetUp() override {
        mqttClient->Reset();
    }

    CelixEarpmImplTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_impl_test_cache"} { }

    ~CelixEarpmImplTestSuite() override = default;

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
        celix_properties_set(props, CELIX_EARPM_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
        celix_properties_setLong(props, CELIX_EARPM_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
        endpoint_description_t* endpoint = nullptr;
        auto status = endpointDescription_create(props, &endpoint);
        EXPECT_EQ(status, CELIX_SUCCESS);
        return endpoint;
    }

    void TestRemoteProvider(const std::function<void (celix_event_admin_remote_provider_mqtt_t*)>& testBody) {
        auto earpm = celix_earpm_create(ctx.get());

        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        auto status = celix_earpm_mqttBrokerEndpointAdded(earpm, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        auto connected = WaitForRemoteProviderConnectToBroker();
        ASSERT_TRUE(connected);

        testBody(earpm);

        status = celix_earpm_mqttBrokerEndpointRemoved(earpm, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_earpm_destroy(earpm);
    }

//    void TestPublishEvent(celix_status_t (*fp)(void* , const char* , const celix_properties_t*)) {
//        auto publisher = celix_earpm_create(publisherCtx.get());
//        celix_mqtt_broker_info_service_t brokerInfo;
//        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> props{celix_properties_create(), celix_properties_destroy};
//        celix_properties_setLong(props.get(), CELIX_FRAMEWORK_SERVICE_ID, 123);
//        celix_properties_set(props.get(), CELIX_MQTT_BROKER_ADDRESS, MQTT_BROKER_ADDRESS);
//        celix_properties_setLong(props.get(), CELIX_MQTT_BROKER_PORT, MQTT_BROKER_PORT);
//        auto status = celix_earpm_addBrokerInfoService(publisher, &brokerInfo, props.get());
//        EXPECT_EQ(status, CELIX_SUCCESS);
//
//        auto subscriber = celix_earpm_create(subscriberCtx.get());
//        status = celix_earpm_addBrokerInfoService(subscriber, &brokerInfo, props.get());
//        EXPECT_EQ(status, CELIX_SUCCESS);
//        std::atomic<bool> receivedEvent{false};
//        celix_event_admin_service_t eventAdminService;
//        eventAdminService.handle = &receivedEvent;
//        eventAdminService.postEvent = [](void* handle, const char* topic, const celix_properties_t*) {
//            auto receivedEvent = static_cast<std::atomic<bool>*>(handle);
//            *receivedEvent = true;
//            EXPECT_STREQ(topic, "topic");
//            return CELIX_SUCCESS;
//        };
//        eventAdminService.sendEvent = [](void* handle, const char* topic, const celix_properties_t*) {
//            auto receivedEvent = static_cast<std::atomic<bool>*>(handle);
//            *receivedEvent = true;
//            EXPECT_STREQ(topic, "topic");
//            return CELIX_SUCCESS;
//        };
//        status = celix_earpm_setEventAdminSvc(subscriber, &eventAdminService);
//        EXPECT_EQ(status, CELIX_SUCCESS);
//
//        celix_event_handler_service_t eventHandlerService;
//        eventHandlerService.handle = nullptr;
//        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
//        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> eventHandlerProps{celix_properties_create(), celix_properties_destroy};
//        celix_properties_setLong(eventHandlerProps.get(), CELIX_FRAMEWORK_SERVICE_ID, 234);
//        celix_properties_set(eventHandlerProps.get(), CELIX_EVENT_TOPIC, "topic");
//        status = celix_earpm_addEventHandlerService(subscriber, &eventHandlerService, eventHandlerProps.get());
//        EXPECT_EQ(status, CELIX_SUCCESS);
//
//        int maxTries = 1;
//        do {
//            std::this_thread::sleep_for(std::chrono::milliseconds{10*maxTries});
//            status = fp(publisher, "topic", nullptr);
//            EXPECT_EQ(status, CELIX_SUCCESS);
//        } while (!receivedEvent && ++maxTries < 3000);
//        EXPECT_TRUE(receivedEvent);
//
//        status = celix_earpm_removeEventHandlerService(subscriber, &eventHandlerService, eventHandlerProps.get());
//        EXPECT_EQ(status, CELIX_SUCCESS);
//        status = celix_earpm_removeBrokerInfoService(subscriber, &brokerInfo, props.get());
//        EXPECT_EQ(status, CELIX_SUCCESS);
//        celix_earpm_destroy(subscriber);
//
//        status = celix_earpm_removeBrokerInfoService(publisher, &brokerInfo, props.get());
//        EXPECT_EQ(status, CELIX_SUCCESS);
//        celix_earpm_destroy(publisher);
//    }//TODO remove

    static void AddRemoteHandlerInfoToRemoteProviderAndCheck(celix_event_admin_remote_provider_mqtt_t* earpm, const char* handlerInfo, const char* senderUUID = FAKE_FW_UUID) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", senderUUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(handlerInfo), handlerInfo, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) != 0; });//Wait for receive the handler info message
        ASSERT_TRUE(ok);
    }

    static void RemoveRemoteHandlerInfoFromRemoteProvider(celix_event_admin_remote_provider_mqtt_t*, long handlerServiceId, const char* senderUUID = FAKE_FW_UUID) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", senderUUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        char payload[128]{0};
        snprintf(payload, sizeof(payload), R"({"handlerId":%ld})", handlerServiceId);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
    }

    static void UpdateRemoteHandlerInfoToRemoteProvider(celix_event_admin_remote_provider_mqtt_t*, const char* handlers, const char* senderUUID = FAKE_FW_UUID) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", senderUUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        char payload[1024]{0};
        snprintf(payload, sizeof(payload), R"({"handlers":%s})", handlers);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
    }

    static bool WaitForRemoteProviderConnectToBroker(void) {
        //When the remote provider is connected, it will send an update handler info message
        return mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
    }

    static bool WaitFor(const std::function<bool (void)>& cond) {
        int remainTries = 3000;
        while (!cond() && remainTries > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});//Wait for condition
            remainTries--;
        }
        return remainTries > 0;
    }

    static pid_t pid;
    static MqttClient* mqttClient;
};
pid_t CelixEarpmImplTestSuite::pid{0};
MqttClient* CelixEarpmImplTestSuite::mqttClient{nullptr};

TEST_F(CelixEarpmImplTestSuite, CreateEarpmTest) {
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_NE(earpm, nullptr);
    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, CreateEarpmWithInvalidDefaultEventQosTest) {
    setenv(CELIX_EARPM_EVENT_QOS, std::to_string(CELIX_EARPM_QOS_MAX).c_str(), 1);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);

    setenv(CELIX_EARPM_EVENT_QOS, std::to_string(CELIX_EARPM_QOS_UNKNOWN).c_str(), 1);
    earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);
    unsetenv(CELIX_EARPM_EVENT_QOS);
}

TEST_F(CelixEarpmImplTestSuite, CreateEarpmWithInvalidControlMessageTimeoutTest) {
    setenv(CELIX_EARPM_CTRL_MSG_REQUEST_TIMEOUT, "0", 1);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);

    setenv(CELIX_EARPM_CTRL_MSG_REQUEST_TIMEOUT, "361", 1);
    earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);

    unsetenv(CELIX_EARPM_CTRL_MSG_REQUEST_TIMEOUT);
}

TEST_F(CelixEarpmImplTestSuite, CreateEarpmWithInvalidNoAckThresholdTest) {
    setenv(CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD, "0", 1);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);

    unsetenv(CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD);
}

TEST_F(CelixEarpmImplTestSuite, AddMqttBrokerEndpointTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
    auto status = celix_earpm_mqttBrokerEndpointAdded(earpm, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_earpm_mqttBrokerEndpointRemoved(earpm, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, AddEventHandlerServiceTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 234);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, AddEventHandlerServiceWithInvalidServiceIdTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, AddEventHandlerServiceWithInvalidQosTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 234);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    celix_properties_setLong(props, CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_UNKNOWN);
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

    celix_properties_setLong(props, CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_MAX);
    status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, AddEventHandlerServiceWithInvalidTopicTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 234);
    celix_properties_setLong(props, CELIX_EVENT_TOPIC, 111);//invalid topic type
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

    celix_properties_unset(props, CELIX_EVENT_TOPIC);//unset topic
    status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

    celix_array_list_t* topics = celix_arrayList_createStringArray();
    celix_properties_assignArrayList(props, CELIX_EVENT_TOPIC, topics);//empty topic list
    status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, RemoveEventHandlerServiceWithInvalidServiceIdTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    auto status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, RemoveNotExistedEventHandlerServiceTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    auto status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, SetEventAdminServiceTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_admin_service_t eventAdminService;
    eventAdminService.handle = nullptr;
    eventAdminService.postEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    eventAdminService.sendEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    auto status = celix_earpm_setEventAdminSvc(earpm, &eventAdminService);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");
        RemoveRemoteHandlerInfoFromRemoteProvider(earpm, 123);
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) == 0; });
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payLoad = R"({"handlers":[{"handlerId":123,"topics":["topic"]}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) != 0; });
        ASSERT_TRUE(ok);

        //use the update message to remove the handler
        payLoad = R"({"handlers":[]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) == 0; });
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        //The remote provider will send the handler description message when received the query message
        auto received = mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
        ASSERT_TRUE(received);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUnknownRemoteHandlerInfoControlMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"unknown",
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Unknown action");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessSessionEndMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})", FAKE_FW_UUID);
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_SESSION_END_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        int maxTries = 1;
        while (celix_earpm_currentRemoteFrameworkCount(earpm) != 0 && maxTries < 30) {
            usleep(100000*maxTries);//Wait for processing the session expiry message
            maxTries++;
        }
        ASSERT_LT(maxTries, 30);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUnknownControlMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_TOPIC_PREFIX"unknown",
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Unknown control message");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessControlMessageWithoutSenderUUIDTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, nullptr);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No sender UUID for control message");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithoutHandlerInfoTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No handler information");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithoutHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handler":{"topics":["topic"]}})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidHandlerIdTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handler":{"handlerId":"invalid","topics":["topic"]}})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handler":{"handlerId":-1,"topics":["topic"]}})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id(%ld) is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithoutTopicsTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handler":{"handlerId":123}})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidTopicsTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handler":{"handlerId":123,"topics":123}})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidTopicTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handler":{"handlerId":123,"topics":[123]}})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topic is not string.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithoutHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithInvalidHandlerIdTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlerId":"invalid"})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithInvalidHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlerId":-1})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id(%ld) is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWhenHandlerInfoNotExistedTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        RemoveRemoteHandlerInfoFromRemoteProvider(earpm, 123);
        auto ok = WaitForLogMessage("No remote framework info");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessSyncEventAckMessageWithoutCorrelationDataTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        char topic[128]{0};
        snprintf(topic, sizeof(topic), "%s%s", CELIX_EARPM_SYNC_EVENT_ACK_TOPIC_PREFIX, fwUUID.c_str());
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, topic, 0, nullptr,
                                  CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Correlation data size is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithoutHandlerInfoTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payLoad = R"({})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No handler information");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithoutHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlers":[{"topics":["topic"]}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidHandlerIdTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlers":[{"handlerId":"invalid","topics":["topic"]}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlers":[{"handlerId":-1,"topics":["topic"]}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id(%ld) is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithoutTopicsTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlers":[{"handlerId":123}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidTopicsTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlers":[{"handlerId":123,"topics":123}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidTopicTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_SENDER_UUID", FAKE_FW_UUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        const char* payload = R"({"handlers":[{"handlerId":123,"topics":[123]}]})";
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topic is not string.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAsyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        //subscribe to the async event
        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS;};
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 234);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "asyncEvent");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //add event admin service to receive the async event
        std::promise<void> receivedEventPromise;
        std::future<void> receivedEventFuture = receivedEventPromise.get_future();
        static celix_event_admin_service_t eventAdminService;
        eventAdminService.handle = &receivedEventPromise;
        eventAdminService.postEvent = [](void* handle, const char* topic, const celix_properties_t* properties) {
            EXPECT_STREQ(topic, "asyncEvent");
            EXPECT_STREQ("value", celix_properties_get(properties, "key", ""));
            auto promise = static_cast<std::promise<void>*>(handle);
            promise->set_value();
            return CELIX_SUCCESS;
        };
        eventAdminService.sendEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        status = celix_earpm_setEventAdminSvc(earpm, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //publish the async event
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        status = celix_properties_set(eventProps, "key", "value");
        ASSERT_EQ(status, CELIX_SUCCESS);
        celix_autofree char* payload = nullptr;
        status = celix_properties_saveToString(eventProps, 0, &payload);
        ASSERT_EQ(status, CELIX_SUCCESS);
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "asyncEvent",
                                       (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, nullptr);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        //wait for the async event
        auto futureStatus = receivedEventFuture.wait_for(std::chrono::seconds{30});
        EXPECT_EQ(futureStatus, std::future_status::ready);

        status = celix_earpm_setEventAdminSvc(earpm, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear the retained message from the broker
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "asyncEvent",
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, nullptr);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessSyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        //subscribe to the async event
        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS;};
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 234);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "syncEvent");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //add event admin service to receive the async event
        static celix_event_admin_service_t eventAdminService;
        eventAdminService.handle = nullptr;
        eventAdminService.postEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        eventAdminService.sendEvent = [](void*, const char* topic, const celix_properties_t* properties) {
            EXPECT_STREQ(topic, "syncEvent");
            EXPECT_STREQ("value", celix_properties_get(properties, "key", ""));
            return CELIX_SUCCESS;
        };
        status = celix_earpm_setEventAdminSvc(earpm, &eventAdminService);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //publish the async event
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        status = celix_properties_set(eventProps, "key", "value");
        ASSERT_EQ(status, CELIX_SUCCESS);
        celix_autofree char* payload = nullptr;
        status = celix_properties_saveToString(eventProps, 0, &payload);
        ASSERT_EQ(status, CELIX_SUCCESS);
        celix_autoptr(mosquitto_property) mqttProps = nullptr;
        char responseTopic[128]{0};
        snprintf(responseTopic, sizeof(responseTopic), CELIX_EARPM_SYNC_EVENT_ACK_TOPIC_PREFIX"%s", FAKE_FW_UUID);
        auto rc = mosquitto_property_add_string(&mqttProps, MQTT_PROP_RESPONSE_TOPIC, responseTopic);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        long correlationData {0};
        rc = mosquitto_property_add_binary(&mqttProps, MQTT_PROP_CORRELATION_DATA, &correlationData, sizeof(correlationData));
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "syncEvent",
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, mqttProps);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        //wait for the sync event ack that is sent by the remote provider
        auto ok = mqttClient->WaitForReceivedMsg(responseTopic);
        ASSERT_TRUE(ok);

        status = celix_earpm_setEventAdminSvc(earpm, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

        status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear the retained message from the broker
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "syncEvent",
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, nullptr);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
    });
}

TEST_F(CelixEarpmImplTestSuite, PostEventTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["subscribedEvent"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        celix_properties_set(eventProps, "key", "value");
        auto status = celix_earpm_postEvent(earpm, "subscribedEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto notFoundSubscriber = WaitForLogMessage("No remote handler subscribe", 10);
        EXPECT_FALSE(notFoundSubscriber);
    });
}

TEST_F(CelixEarpmImplTestSuite, PostEventToSubscribeAnyTopicHandlerTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["*"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        celix_properties_set(eventProps, "key", "value");
        auto status = celix_earpm_postEvent(earpm, "subscribedEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto notFoundSubscriber = WaitForLogMessage("No remote handler subscribe", 10);
        EXPECT_FALSE(notFoundSubscriber);
    });
}

TEST_F(CelixEarpmImplTestSuite, PostEventToSubscribeTopicPatternHandlerTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["test/*"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        celix_properties_set(eventProps, "key", "value");
        auto status = celix_earpm_postEvent(earpm, "test/Event", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto notFoundSubscriber = WaitForLogMessage("No remote handler subscribe", 10);
        EXPECT_FALSE(notFoundSubscriber);
    });
}

TEST_F(CelixEarpmImplTestSuite, PostEventButNoSubscriberTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_status_t status = celix_earpm_postEvent(earpm, "unsubscribedEvent", nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto ok = WaitForLogMessage("No remote handler subscribe");
        EXPECT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, PostEventButEventPropertiesNotMatchTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"s({"handler":{"handlerId":123,"topics":["subscribedEvent"], "filter":"(key=value1)"}})s");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        celix_properties_set(eventProps, "key", "value");
        auto status = celix_earpm_postEvent(earpm, "subscribedEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto ok = WaitForLogMessage("No remote handler subscribe");
        EXPECT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, PostEventWithInvalidQosTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["subscribedEvent"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_UNKNOWN);
        auto status = celix_earpm_postEvent(earpm, "subscribedEvent", eventProps);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_MAX);
        status = celix_earpm_postEvent(earpm, "subscribedEvent", eventProps);
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["subscribedEvent"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        ASSERT_NE(eventProps, nullptr);
        celix_properties_set(eventProps, "key", "value");
        auto status = celix_earpm_sendEvent(earpm, "subscribedEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto ok = WaitForLogMessage("No remote handler subscribe", 10);
        EXPECT_FALSE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventButNoSubscriberTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_status_t status = celix_earpm_sendEvent(earpm, "unsubscribedEvent", nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto ok = WaitForLogMessage("No remote handler subscribe");
        EXPECT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventButNoAckTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["noAckEvent"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        auto status = celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
        ASSERT_EQ(status, CELIX_SUCCESS);
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, ETIMEDOUT);
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventButAlwaysNoAckTest) {
    setenv(CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD, "1", 1);
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["noAckEvent"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        auto status = celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
        ASSERT_EQ(status, CELIX_SUCCESS);
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, ETIMEDOUT);
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, ETIMEDOUT);
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        auto ok = WaitForLogMessage("No waiting for the ack");
        EXPECT_TRUE(ok);
    });
    unsetenv(CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD);
}

TEST_F(CelixEarpmImplTestSuite, SendEventAndThenAllRemoteHandlerRemovedTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["noAckEvent"]}})");
        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        auto status = celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
        ASSERT_EQ(status, CELIX_SUCCESS);
        std::future<void> futureResult = std::async(std::launch::async, [earpm] {
            std::this_thread::sleep_for(std::chrono::milliseconds{30});//Wait for the remote provider to send event
            RemoveRemoteHandlerInfoFromRemoteProvider(earpm, 123);
        });
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        futureResult.wait();
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventAndThenRemoteHandlerRemovedTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        UpdateRemoteHandlerInfoToRemoteProvider(earpm, R"([{"handlerId":123,"topics":["noAckEvent"]}, {"handlerId":124,"topics":["topic"]}])");
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) != 0; });
        ASSERT_TRUE(ok);

        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        auto status = celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
        ASSERT_EQ(status, CELIX_SUCCESS);
        std::future<void> futureResult = std::async(std::launch::async, [earpm] {
            std::this_thread::sleep_for(std::chrono::milliseconds{30});//Wait for the remote provider to send event
            RemoveRemoteHandlerInfoFromRemoteProvider(earpm, 123);
        });
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        futureResult.wait();
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventAndThenAllRemoteHandlerRemovedByUpdateMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        UpdateRemoteHandlerInfoToRemoteProvider(earpm, R"([{"handlerId":123,"topics":["noAckEvent"]}, {"handlerId":124,"topics":["topic"]}])");
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) != 0; });
        ASSERT_TRUE(ok);

        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        auto status = celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
        ASSERT_EQ(status, CELIX_SUCCESS);
        std::future<void> futureResult = std::async(std::launch::async, [earpm] {
            std::this_thread::sleep_for(std::chrono::milliseconds{30});//Wait for the remote provider to send event
            UpdateRemoteHandlerInfoToRemoteProvider(earpm, R"([])");
        });
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        futureResult.wait();
    });
}

TEST_F(CelixEarpmImplTestSuite, SendEventAndThenRemoteHandlerRemovedByUpdateMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        UpdateRemoteHandlerInfoToRemoteProvider(earpm, R"([{"handlerId":123,"topics":["noAckEvent"]}, {"handlerId":124,"topics":["topic"]}])");
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) != 0; });
        ASSERT_TRUE(ok);

        celix_autoptr(celix_properties_t) eventProps = celix_properties_create();
        auto status = celix_properties_setLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
        ASSERT_EQ(status, CELIX_SUCCESS);
        std::future<void> futureResult = std::async(std::launch::async, [earpm] {
            std::this_thread::sleep_for(std::chrono::milliseconds{30});//Wait for the remote provider to send event
            UpdateRemoteHandlerInfoToRemoteProvider(earpm, R"([{"handlerId":124,"topics":["topic"]}])");
        });
        status = celix_earpm_sendEvent(earpm, "noAckEvent", eventProps);
        EXPECT_EQ(status, CELIX_SUCCESS);
        futureResult.wait();
    });
}


class CelixEarpmImplTestSuite2 : public CelixEarpmImplTestSuite {
public:
    static void SetUpTestSuite() {
        mosquitto_lib_init();
    }

    static void TearDownTestSuite() {
        mosquitto_lib_cleanup();
    }

    void SetUp() override {
        pid = fork();
        ASSERT_GE(pid, 0);
        if (pid == 0) {
            execlp("mosquitto", "mosquitto", "-p", std::to_string(MQTT_BROKER_PORT).c_str(), nullptr);
            ADD_FAILURE() << "Failed to start mosquitto";
        }
        mqttClient = new MqttClient{std::vector<std::string>{"subscribedEvent", CELIX_EARPM_TOPIC_PREFIX"#"}};
        mqttClient->MqttClientStart();
    }

    void TearDown() override {
        mqttClient->MqttClientStop();
        delete mqttClient;
        mqttClient = nullptr;
        if (pid > 0) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
    }
    static void RestartBroker() {
        if (pid > 0) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
        pid = fork();
        ASSERT_GE(pid, 0);
        if (pid == 0) {
            execlp("mosquitto", "mosquitto", "-p", std::to_string(MQTT_BROKER_PORT).c_str(), nullptr);
            ADD_FAILURE() << "Failed to restart mosquitto";
        }
    }
};

TEST_F(CelixEarpmImplTestSuite2, BrokerRestartAndRemoteFrameworkExpiredTest) {
    setenv(CELIX_EARPM_CTRL_MSG_REQUEST_TIMEOUT, "1", 1);
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["subscribedEvent"]}})");
        RestartBroker();
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCount(earpm) == 0; });
        ASSERT_TRUE(ok);
    });
    unsetenv(CELIX_EARPM_CTRL_MSG_REQUEST_TIMEOUT);
}

