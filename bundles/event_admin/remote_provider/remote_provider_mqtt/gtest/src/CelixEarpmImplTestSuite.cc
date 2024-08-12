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
#include "celix_event_handler_service.h"
#include "celix_event_admin_service.h"
#include "celix_event_constants.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "CelixEarpmTestSuiteBaseClass.h"
#include "celix_log_helper.h"

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
        mosquitto_message_callback_set(mosq.get(), [](mosquitto*, void* handle, const mosquitto_message* msg) {
            auto client = static_cast<MqttClient*>(handle);
            std::lock_guard<std::mutex> lock(client->mutex);
            client->receivedMsgTopics.emplace_back(msg->topic);
            client->receivedMsgCond.notify_all();
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
            auto it = std::find(receivedMsgTopics.begin(), receivedMsgTopics.end(), topic);
            if (it != receivedMsgTopics.end()) {
                receivedMsgTopics.erase(it);
                return true;
            }
            return false;
        });
    }

    void reset() {
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
        mqttClient = new MqttClient{std::vector<std::string>{"subTopic", CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"#"}};
        mqttClient->MqttClientStart();
    }

    static void TearDownTestSuite() {
        mqttClient->MqttClientStop();
        delete mqttClient;
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        mosquitto_lib_cleanup();
    }

    CelixEarpmImplTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_impl_test_cache"} {
        publisherCtx = ctx;
        mqttClient->reset();
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, "subscriber_test_cache");
        celix_properties_set(props, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "trace");
        auto fwPtr = celix_frameworkFactory_createFramework(props);
        subscriberFw = std::shared_ptr < celix_framework_t >
                {fwPtr, [](celix_framework_t *f) { celix_frameworkFactory_destroyFramework(f); }};
        subscriberCtx = std::shared_ptr < celix_bundle_context_t >
                {celix_framework_getFrameworkContext(subscriberFw.get()), [](celix_bundle_context_t *) {/*nop*/}};
    }

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

    void TestRemoteProvider(std::function<void (celix_event_admin_remote_provider_mqtt_t*)>testBody) {
        auto earpm = celix_earpm_create(ctx.get());

        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        auto status = celix_earpm_mqttBrokerEndpointAdded(earpm, endpoint, nullptr);
        EXPECT_EQ(status, CELIX_SUCCESS);

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
//    }

    void AddRemoteHandlerInfoToRemoteProviderAndCheck(celix_event_admin_remote_provider_mqtt_t* earpm, const char* handlerInfo, const char* frameworkUUID = FAKE_FW_UUID) {
        auto connected = WaitForRemoteProviderConnectToBroker();
        ASSERT_TRUE(connected);
        mosquitto_property* properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, frameworkUUID);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  strlen(handlerInfo), handlerInfo, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        mosquitto_property_free_all(&properties);
        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCnt(earpm) != 0; });//Wait for receiving the handler description message
        ASSERT_TRUE(ok);
    }

    static bool WaitForRemoteProviderConnectToBroker(void) {
        //When the remote provider is connected, it will send an update handler description message
        return mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
    }

    static bool WaitFor(std::function<bool (void)> cond) {
        int remainTries = 3000;
        while (!cond() && remainTries > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});//Wait for condition
            remainTries--;
        }
        return remainTries > 0;
    }

    std::shared_ptr<celix_framework_t> subscriberFw{};
    std::shared_ptr<celix_bundle_context_t> subscriberCtx{};
    std::shared_ptr<celix_bundle_context_t> publisherCtx{};

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
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
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

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerDescriptionMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handlerId":123,"topics":["topic"],"fwExpiryInterval":30})");
    });
}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerDescriptionMessageTest) {
//    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        AddHandlerDescriptionAndCheck(earpm, R"({"handlerId":123,"topics":["topic"],"fwExpiryInterval":30})", FAKE_FW_UUID);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        const char* payLoad = R"({"handlerId":123})";
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
//                                  strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCnt(earpm) == 0; });
//        ASSERT_TRUE(ok);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerDescriptionMessageTest) {
//    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        AddHandlerDescriptionAndCheck(earpm, R"({"handlerId":123,"topics":["topic"],"fwExpiryInterval":30})", FAKE_FW_UUID);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//
//        const char* payLoad = R"({"handlers":[{"handlerId":123,"topics":["topic"]}],"fwExpiryInterval":60})";
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
//                                  strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        auto ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCnt(earpm) != 0; });
//        ASSERT_TRUE(ok);
//
//        payLoad = R"({"handlers":[],"fwExpiryInterval":60})";
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
//                                  strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        ok = WaitFor([earpm] { return celix_earpm_currentRemoteFrameworkCnt(earpm) == 0; });
//        ASSERT_TRUE(ok);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessQueryAllRemoteHandlerDescriptionMessageTest) {
//    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t*) {
//        auto connected = WaitForRemoteProviderConnectToBroker();
//        ASSERT_TRUE(connected);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
//                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        //The remote provider will send the handler description message when received the query message
//        auto received = mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
//        ASSERT_TRUE(received);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessQuerySpecificRemoteHandlerDescriptionMessageTest) {
//    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
//        auto connected = WaitForRemoteProviderConnectToBroker();
//        ASSERT_TRUE(connected);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        constexpr const char* payloadFormat = R"({"queriedFrameWorks":["%s"]})";
//        char payload[128]{0};
//        snprintf(payload, sizeof(payload), payloadFormat, fwUUID.c_str());
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
//                                  strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        //The remote provider will send the handler description message when received the query message
//        auto received = mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
//        ASSERT_TRUE(received);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessUnknownRemoteHandlerDescriptionMessageTest) {
//    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t*) {
//        auto connected = WaitForRemoteProviderConnectToBroker();
//        ASSERT_TRUE(connected);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"unknown",
//                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        std::this_thread::sleep_for(std::chrono::milliseconds{100});//Wait for processing the unknown message
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessQueryRemoteHandlerDescriptionMessageWithoutFrameworkUUIDTest) {
//    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t*) {
//        auto connected = WaitForRemoteProviderConnectToBroker();
//        ASSERT_TRUE(connected);
//        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
//                                       0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, nullptr);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        //The remote provider will send the handler description message when received the query message
//        auto received = mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC, std::chrono::milliseconds{100});
//        EXPECT_FALSE(received);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessSesionEndTopicTest) {
//    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        AddHandlerDescriptionAndCheck(earpm, R"({"handlerId":123,"topics":["subTopic"],"fwExpiryInterval":30})", FAKE_FW_UUID);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_SESSION_END_TOPIC,
//                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        int maxTries = 1;
//        while (celix_earpm_currentRemoteFrameworkCnt(earpm) != 0 && maxTries < 30) {
//            usleep(100000*maxTries);//Wait for processing the session expiry message
//            maxTries++;
//        }
//        ASSERT_LT(maxTries, 30);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, ProcessUnknownEventAdminMqttMessageTest) {
//    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t*) {
//        auto connected = WaitForRemoteProviderConnectToBroker();
//        ASSERT_TRUE(connected);
//        mosquitto_property* properties = nullptr;
//        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, FAKE_FW_UUID);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_TOPIC_PREFIX"unknown",
//                                       0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
//        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
//        mosquitto_property_free_all(&properties);
//        std::this_thread::sleep_for(std::chrono::milliseconds{100});//Wait for processing the unknown message
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, PostEventTest) {
//    TestPublishEvent(celix_earpm_postEvent);
//}
//
//TEST_F(CelixEarpmImplTestSuite, PostEventButNoSubscriberTest) {
//    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        celix_status_t status = celix_earpm_postEvent(earpm, "unsubTopic", nullptr);
//        EXPECT_EQ(status, CELIX_SUCCESS);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, SendEventTest) {
//    TestPublishEvent(celix_earpm_sendEvent);
//}
//
//TEST_F(CelixEarpmImplTestSuite, SendEventButNoSubscriberTest) {
//    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        celix_status_t status = celix_earpm_sendEvent(earpm, "unsubTopic", nullptr);
//        EXPECT_EQ(status, CELIX_SUCCESS);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, SendEventButSubscriberNotExistedInRemoteTest) {
//    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        AddHandlerDescriptionAndCheck(earpm, R"({"handlerId":123,"topics":["testTopic"],"fwExpiryInterval":30})");
//        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> eventProps{celix_properties_create(), celix_properties_destroy};
//        ASSERT_NE(eventProps, nullptr);
//        auto status = celix_properties_setLong(eventProps.get(), CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_AT_LEAST_ONCE);
//        ASSERT_EQ(status, CELIX_SUCCESS);
//        status = celix_properties_setLong(eventProps.get(), CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
//        ASSERT_EQ(status, CELIX_SUCCESS);
//        status = celix_earpm_sendEvent(earpm, "testTopic", eventProps.get());
//        EXPECT_EQ(status, ETIMEDOUT);
//    });
//}
//
//TEST_F(CelixEarpmImplTestSuite, SendEventButNoAckTest) {
//    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
//        AddHandlerDescriptionAndCheck(earpm, R"({"handlerId":123,"topics":["subTopic"],"fwExpiryInterval":30})");
//        std::unique_ptr<celix_properties_t, decltype(&celix_properties_destroy)> eventProps{celix_properties_create(), celix_properties_destroy};
//        ASSERT_NE(eventProps, nullptr);
//        auto status = celix_properties_setLong(eventProps.get(), CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, 1);
//        ASSERT_EQ(status, CELIX_SUCCESS);
//        status = celix_earpm_sendEvent(earpm, "subTopic", eventProps.get());
//        EXPECT_EQ(status, ETIMEDOUT);
//    });
//}


