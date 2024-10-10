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
#include <mosquitto.h>

#include "celix_stdlib_cleanup.h"
#include "celix_event_handler_service.h"
#include "celix_event_admin_service.h"
#include "celix_event_constants.h"
#include "celix_earpm_impl.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "CelixEarpmImplTestSuiteBaseClass.h"


class CelixEarpmImplTestSuite : public CelixEarpmImplTestSuiteBaseClass {
public:
    CelixEarpmImplTestSuite() : CelixEarpmImplTestSuiteBaseClass{} { }

    ~CelixEarpmImplTestSuite() override = default;
};

TEST_F(CelixEarpmImplTestSuite, CreateEarpmTest) {
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_NE(earpm, nullptr);
    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplTestSuite, CreateEarpmWithInvalidDefaultEventQosTest) {
    setenv(CELIX_EARPM_EVENT_DEFAULT_QOS, std::to_string(CELIX_EARPM_QOS_MAX).c_str(), 1);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);

    setenv(CELIX_EARPM_EVENT_DEFAULT_QOS, std::to_string(CELIX_EARPM_QOS_UNKNOWN).c_str(), 1);
    earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(earpm, nullptr);
    unsetenv(CELIX_EARPM_EVENT_DEFAULT_QOS);
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

TEST_F(CelixEarpmImplTestSuite, AddEventHandlerServicesThatHaveSameEventTopicTest) {
    auto earpm = celix_earpm_create(ctx.get());

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props1 = celix_properties_create();
    celix_properties_setLong(props1, CELIX_FRAMEWORK_SERVICE_ID, 123);
    celix_properties_setLong(props1, CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_AT_MOST_ONCE);
    celix_properties_set(props1, CELIX_EVENT_TOPIC, "topic");
    celix_properties_set(props1, CELIX_EVENT_FILTER, "(a=b)");
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props1);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_autoptr(celix_properties_t) props2 = celix_properties_copy(props1);
    celix_properties_setLong(props2, CELIX_FRAMEWORK_SERVICE_ID, 124);
    celix_properties_setLong(props2, CELIX_EVENT_REMOTE_QOS, CELIX_EARPM_QOS_AT_LEAST_ONCE);
    status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props2);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props2);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props1);
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
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payLoad = R"({"handlers":[{"handlerId":123,"topics":["topic"]}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
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
        mqttClient->Reset();//clear received messages cache
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        //The remote provider will send the handler description message when received the query message
        auto received = mqttClient->WaitForReceivedMsg(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
        ASSERT_TRUE(received);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUnknownRemoteHandlerInfoControlMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"unknown",
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Unknown action");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessSessionEndMessageTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})", FAKE_FW_UUID);
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_SESSION_END_TOPIC,
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

TEST_F(CelixEarpmImplTestSuite, ProcessMessageWithoutVersion) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, nullptr);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage(CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC" message version(null) is incompatible.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessMessageVersionFormatError) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_MSG_VERSION", "1");
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                       0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage(CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC" message version(1) is incompatible.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessMessageVersionStringTooLong) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_MSG_VERSION", "100000.200000000.300000");
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage(CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC" message version(100000.200000000.300000) is incompatible.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessMessageVersionIncompatible) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_MSG_VERSION", "10.0.0");
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage(CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC" message version(10.0.0) is incompatible.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessControlMessageWithoutSenderUUIDTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = nullptr;
        auto rc = mosquitto_property_add_string_pair(&properties, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_MSG_VERSION", "1.0.0");
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No sender UUID for control message");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidPayloadTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"(invalid)";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Failed to parse message");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithoutHandlerInfoTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No handler information");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithoutHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidHandlerIdTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":"invalid","topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":-1,"topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id(-1) is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithoutTopicsTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidTopicsTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123,"topics":123}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessAddRemoteHandlerInfoMessageWithInvalidTopicTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123,"topics":[123]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topic is not string.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithInvalidPayloadTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"(invalid)";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Failed to parse message");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithoutHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithInvalidHandlerIdTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlerId":"invalid"})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessRemoveRemoteHandlerInfoMessageWithInvalidHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlerId":-1})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id(-1) is invalid");
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
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        char topic[128]{0};
        snprintf(topic, sizeof(topic), "%s%s", CELIX_EARPM_SYNC_EVENT_ACK_TOPIC_PREFIX, fwUUID.c_str());
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, topic, 0, nullptr,
                                  CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Correlation data size is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessSyncEventAckMessageWithInvalidCorrelationDataTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_property_add_binary(&properties, MQTT_PROP_CORRELATION_DATA, "invalid", strlen("invalid"));
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

TEST_F(CelixEarpmImplTestSuite, ProcessSyncEventAckMessageBeforeRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        long correlationData = 1;
        auto rc = mosquitto_property_add_binary(&properties, MQTT_PROP_CORRELATION_DATA, &correlationData, sizeof(correlationData));
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        char topic[128]{0};
        snprintf(topic, sizeof(topic), "%s%s", CELIX_EARPM_SYNC_EVENT_ACK_TOPIC_PREFIX, fwUUID.c_str());
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, topic, 0, nullptr,
                                  CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No remote framework info");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidPayloadTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payLoad = R"(invalid)";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Failed to parse message");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithoutHandlerInfoTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payLoad = R"({})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payLoad), payLoad, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("No handler information");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithoutHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"topics":["topic"]}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidHandlerIdTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"handlerId":"invalid","topics":["topic"]}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id is lost or not integer");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidHandlerIdTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"handlerId":-1,"topics":["topic"]}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Handler id(-1) is invalid");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithoutTopicsTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"handlerId":123}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidTopicsTypeTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"handlerId":123,"topics":123}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        auto ok = WaitForLogMessage("Topics is lost or not array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplTestSuite, ProcessUpdateRemoteHandlerInfoMessageWithInvalidTopicTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"handlerId":123,"topics":[123]}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
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
        celix_autoptr(mosquitto_property) mqttProperties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "asyncEvent",
                                       (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, mqttProperties);
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
        //subscribe to the sync event
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

        //publish the sync event
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
        rc = mosquitto_property_add_string_pair(&mqttProps, MQTT_PROP_USER_PROPERTY, "CELIX_EARPM_MSG_VERSION", "1.0.0");
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

TEST_F(CelixEarpmImplTestSuite, PostEventWithInvalidArgsTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        auto status = celix_earpm_postEvent(nullptr, "topic", nullptr);//handle is null
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpm_postEvent(earpm, nullptr, nullptr);//topic is null
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

TEST_F(CelixEarpmImplTestSuite, SendEventWithInvalidArgsTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        auto status = celix_earpm_sendEvent(nullptr, "topic", nullptr);//handle is null
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

        status = celix_earpm_sendEvent(earpm, nullptr, nullptr);//topic is null
        EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
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


