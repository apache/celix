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

#ifndef CELIX_CELIXEARPMIMPLTESTSUITEBASECLASS_H
#define CELIX_CELIXEARPMIMPLTESTSUITEBASECLASS_H

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

#include "celix_stdlib_cleanup.h"
extern "C" {
#include "endpoint_description.h"
}
#include "remote_constants.h"
#include "celix_event_handler_service.h"
#include "celix_event_admin_service.h"
#include "celix_event_constants.h"
#include "celix_earpm_impl.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_broker_discovery.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "CelixEarpmTestSuiteBaseClass.h"


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

class CelixEarpmImplTestSuiteBaseClass : public CelixEarpmTestSuiteBaseClass {
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

    explicit CelixEarpmImplTestSuiteBaseClass(const char* testCache = ".earpm_impl_test_cache") : CelixEarpmTestSuiteBaseClass{testCache} { }

    ~CelixEarpmImplTestSuiteBaseClass() override = default;

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
pid_t CelixEarpmImplTestSuiteBaseClass::pid{0};
MqttClient* CelixEarpmImplTestSuiteBaseClass::mqttClient{nullptr};



#endif //CELIX_CELIXEARPMIMPLTESTSUITEBASECLASS_H
