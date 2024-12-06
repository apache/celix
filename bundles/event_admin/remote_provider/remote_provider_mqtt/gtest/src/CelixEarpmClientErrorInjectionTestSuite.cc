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
#include <cstdlib>

#include "celix_bundle_context_ei.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_utils_ei.h"
#include "mosquitto_ei.h"
#include "celix_properties_ei.h"
#include "celix_earpm_client.h"
#include "celix_earpm_broker_discovery.h"
#include "CelixEarpmClientTestSuiteBaseClass.h"

class CelixEarpmClientErrorInjectionTestSuite : public CelixEarpmClientTestSuiteBaseClass {
public:
    CelixEarpmClientErrorInjectionTestSuite() : CelixEarpmClientTestSuiteBaseClass{".earpm_client_ei_test_cache"} {}

    ~CelixEarpmClientErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadCondition_init(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_mosquitto_property_add_int32(nullptr, 0, 0);
        celix_ei_expect_mosquitto_property_add_string_pair(nullptr, 0, 0);
        celix_ei_expect_mosquitto_new(nullptr, 0, nullptr);
        celix_ei_expect_mosquitto_int_option(nullptr, 0, 0);
        celix_ei_expect_mosquitto_will_set_v5(nullptr, 0, 0);
        celix_ei_expect_celix_properties_getAsStringArrayList(nullptr, 0, 0);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celixThreadCondition_signal(nullptr, 0, 0);
        celix_ei_expect_celix_stringHashMap_putLong(nullptr, 0, 0);
        celix_ei_expect_mosquitto_subscribe_v5(nullptr, 0, 0);
        celix_ei_expect_mosquitto_unsubscribe(nullptr, 0, 0);
        celix_ei_expect_mosquitto_publish_v5(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_put(nullptr, 0, 0);
        celix_ei_expect_mosquitto_property_read_string(nullptr, 0, nullptr);
        celix_ei_expect_mosquitto_property_read_binary(nullptr, 0, nullptr);
        celix_ei_expect_mosquitto_property_read_string_pair(nullptr, 0, nullptr);
    };
};

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToGetFrameworkUUIDTest) {
    celix_ei_expect_celix_bundleContext_getProperty((void*)&celix_earpmClient_create, 0, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAllocMemoryForClientTest) {
    celix_ei_expect_calloc((void*)&celix_earpmClient_create, 0, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateMutexTest) {
    celix_ei_expect_celixThreadMutex_create((void*)&celix_earpmClient_create, 0, ENOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateMessageStatusConditionTest) {
    celix_ei_expect_celixThreadCondition_init((void*)&celix_earpmClient_create, 0, ENOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateMessagePoolTest) {
    celix_ei_expect_calloc((void*)&celix_earpmClient_create, 1, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreatePublishingMessageQueueTest) {
    celix_ei_expect_celix_longHashMap_createWithOptions((void*)&celix_earpmClient_create, 0, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateSubscriptionMapTest) {
    celix_ei_expect_celix_stringHashMap_create((void*)&celix_earpmClient_create, 0, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateBrokerInfoMapTest) {
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)&celix_earpmClient_create, 0, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateBrokerInfoChangedConditionTest) {
    celix_ei_expect_celixThreadCondition_init((void*)&celix_earpmClient_create, 0, ENOMEM, 2);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddMqttSessionExpiryIntervalToConnectPropertyTest) {
    celix_ei_expect_mosquitto_property_add_int32((void*)&celix_earpmClient_create, 0, MOSQ_ERR_NOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddMqttSessionExpiryIntervalToDisconnectPropertyTest) {
    celix_ei_expect_mosquitto_property_add_int32((void*)&celix_earpmClient_create, 0, MOSQ_ERR_NOMEM, 2);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}
TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateMosquittoInstanceTest) {
    celix_ei_expect_mosquitto_new((void*)&celix_earpmClient_create, 0, nullptr);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToSetMqttVersionOptionTest) {
    celix_ei_expect_mosquitto_int_option((void*)&celix_earpmClient_create, 1, MOSQ_ERR_NOT_SUPPORTED);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToSetMqttNoDelayOptionTest) {
    celix_ei_expect_mosquitto_int_option((void*)&celix_earpmClient_create, 1, MOSQ_ERR_NOT_SUPPORTED, 2);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddDelayIntervalToWillMessagePropertyTest) {
    celix_ei_expect_mosquitto_property_add_int32((void*)&celix_earpmClient_create, 1, MOSQ_ERR_NOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddSenderUUIDToWillMessagePropertyTest) {
    celix_ei_expect_mosquitto_property_add_string_pair((void*)&celix_earpmClient_create, 1, MOSQ_ERR_NOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddMsgVersionToWillMessagePropertyTest) {
    celix_ei_expect_mosquitto_property_add_string_pair((void*)&celix_earpmClient_create, 1, MOSQ_ERR_NOMEM, 2);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToSetWillMessageTest) {
    celix_ei_expect_mosquitto_will_set_v5((void*)&celix_earpmClient_create, 1, MOSQ_ERR_NOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToCreateWorkThreadTest) {
    celix_ei_expect_celixThread_create((void*)&celix_earpmClient_create, 0, ENOMEM);
    celix_earpm_client_create_options_t opts{defaultOpts};
    celix_earpm_client_t *client = celix_earpmClient_create(&opts);
    ASSERT_EQ(nullptr, client);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToGetBrokerStaticAddressTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1883);

        celix_ei_expect_celix_properties_getAsStringArrayList((void*)&celix_earpmClient_mqttBrokerEndpointAdded, 1, ENOMEM);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToGetBrokerDynamicAddressTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, 1883);

        celix_ei_expect_celix_properties_getAsStringArrayList((void *) &celix_earpmClient_mqttBrokerEndpointAdded, 1,
                                                              ENOMEM, 2);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToDupBrokerInterfaceNameTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, LOOP_BACK_INTERFACE);
        celix_properties_setLong(endpoint->properties, CELIX_RSA_PORT, 1883);
        celix_properties_set(endpoint->properties, CELIX_RSA_IP_ADDRESSES, "");

        celix_ei_expect_celix_utils_strdup((void *) &celix_earpmClient_mqttBrokerEndpointAdded, 1, nullptr);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAllocMemoryForBrokerInfoTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1883);

        celix_ei_expect_calloc((void *) &celix_earpmClient_mqttBrokerEndpointAdded, 1, nullptr);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddBrokerInfoToMapTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1883);

        celix_ei_expect_celix_stringHashMap_put((void *) &celix_earpmClient_mqttBrokerEndpointAdded, 0, ENOMEM);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToNotifyBrokerInfoChangedTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1883);

        celix_ei_expect_celixThreadCondition_signal((void *) &celix_earpmClient_mqttBrokerEndpointAdded, 0, EPERM);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(CELIX_SUCCESS, status);
        auto ok = WaitForLogMessage("Failed to signal adding broker information");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToDupBrokeInfoMapWhenConnectingBrokerTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1883);

        celix_ei_expect_celix_stringHashMap_createWithOptions(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(CELIX_SUCCESS, status);
        auto ok = WaitForLogMessage("Failed to create broker info map.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToRetainBrokeInfoWhenConnectingBrokerTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_autoptr(endpoint_description_t) endpoint = CreateMqttBrokerEndpoint();
        celix_properties_set(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, "127.0.0.1");
        celix_properties_setLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 1883);

        celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM, 2);
        auto status = celix_earpmClient_mqttBrokerEndpointAdded(client, endpoint, nullptr);
        ASSERT_EQ(CELIX_SUCCESS, status);
        auto ok = WaitForLogMessage("Failed to copy broker info.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddSubscriptionToMapTest) {
    TestClient([](celix_earpm_client_t* client) {
        celix_ei_expect_celix_stringHashMap_putLong((void *) &celix_earpmClient_subscribe, 0, ENOMEM);
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_MOST_ONCE);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToSubscribeMessageTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        celix_ei_expect_mosquitto_subscribe_v5((void *) &celix_earpmClient_subscribe, 0, MOSQ_ERR_NOMEM);
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_MOST_ONCE);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToChangeSubscribedMessageQosTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_MOST_ONCE);
        ASSERT_EQ(CELIX_SUCCESS, status);

        celix_ei_expect_mosquitto_subscribe_v5((void *) &celix_earpmClient_subscribe, 0, MOSQ_ERR_NOMEM);
        status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToUnSubscribedMessageTest) {
    TestClientAfterConnectedBroker([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_MOST_ONCE);
        ASSERT_EQ(CELIX_SUCCESS, status);

        celix_ei_expect_mosquitto_unsubscribe((void *) &celix_earpmClient_unsubscribe, 0, MOSQ_ERR_NOMEM);
        status = celix_earpmClient_unsubscribe(client, "test/topic");
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToMarkMessageUnSubscribedWhenUnconnectBrokerTest) {
    TestClient([](celix_earpm_client_t* client) {
        auto status = celix_earpmClient_subscribe(client, "test/topic", CELIX_EARPM_QOS_AT_MOST_ONCE);
        ASSERT_EQ(CELIX_SUCCESS, status);

        celix_ei_expect_celix_stringHashMap_putLong((void *) &celix_earpmClient_unsubscribe, 0, ENOMEM);
        status = celix_earpmClient_unsubscribe(client, "test/topic");
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToDupAsyncMessageTopicTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_celix_utils_strdup((void *) &celix_earpmClient_publishAsync, 1, nullptr);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddSenderUUIDToAsyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_string_pair((void *) &celix_earpmClient_publishAsync, 1, MOSQ_ERR_NOMEM);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddVersionToAsyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_string_pair((void *) &celix_earpmClient_publishAsync, 1, MOSQ_ERR_NOMEM, 2);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToPublishAsyncMessageTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_publish_v5((void *) &celix_earpmClient_publishAsync, 2, MOSQ_ERR_OVERSIZE_PACKET);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToEnqueueAsyncMessageToPublishingQueueTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_celix_longHashMap_put((void *) &celix_earpmClient_publishAsync, 2, ENOMEM);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToDupAsyncMessagePayloadTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_malloc((void *) &celix_earpmClient_publishAsync, 2, nullptr);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToDupSyncMessageTopicTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_celix_utils_strdup((void *) &celix_earpmClient_publishSync, 1, nullptr);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddExpiryIntervalToSyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 1;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_int32((void *) &celix_earpmClient_publishSync, 1, MOSQ_ERR_NOMEM);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddResponseTopicToSyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 1;
        requestInfo.responseTopic = "response/topic";
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_string((void *) &celix_earpmClient_publishSync, 1, MOSQ_ERR_NOMEM);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddCorrelationDataToSyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 1;
        requestInfo.responseTopic = "response/topic";
        long correlationData{0};
        requestInfo.correlationData = &correlationData;
        requestInfo.correlationDataSize = sizeof(correlationData);
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_binary((void *) &celix_earpmClient_publishSync, 1, MOSQ_ERR_NOMEM);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddSenderUUIDToSyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_string_pair((void *) &celix_earpmClient_publishSync, 1, MOSQ_ERR_NOMEM);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToAddVersionToSyncMessageTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.expiryInterval = 1;
        requestInfo.responseTopic = "response/topic";
        long correlationData{0};
        requestInfo.correlationData = &correlationData;
        requestInfo.correlationDataSize = sizeof(correlationData);
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_property_add_string_pair((void *) &celix_earpmClient_publishSync, 1, MOSQ_ERR_NOMEM,  2);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToPublishSyncMessageTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_publish_v5((void *) &celix_earpmClient_publishSync, 2, MOSQ_ERR_OVERSIZE_PACKET);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToEnqueueSyncMessageToPublishingQueueTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_celix_longHashMap_put((void *) &celix_earpmClient_publishSync, 2, ENOMEM);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToDupSyncMessagePayloadTest) {
    TestClient([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_malloc((void *) &celix_earpmClient_publishSync, 2, nullptr);
        auto status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToPublishSyncMessageWhichInWaitingQueueTest) {
    setenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY, "1", 1);
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client) {
        celix_earpm_client_request_info_t requestInfo{};
        memset(&requestInfo, 0, sizeof(requestInfo));
        requestInfo.topic = "test/topic";
        requestInfo.payload = "test payload";
        requestInfo.payloadSize = strlen(requestInfo.payload);
        requestInfo.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
        requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
        requestInfo.senderUUID = fwUUID.c_str();
        requestInfo.version = "1.0.0";
        celix_ei_expect_mosquitto_publish_v5(CELIX_EI_UNKNOWN_CALLER, 0, MOSQ_ERR_OVERSIZE_PACKET, 2);
        auto status = celix_earpmClient_publishAsync(client, &requestInfo);
        ASSERT_EQ(CELIX_SUCCESS, status);

        status = celix_earpmClient_publishSync(client, &requestInfo);
        ASSERT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
    unsetenv(CELIX_EARPM_PARALLEL_MSG_CAPACITY);
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToReadMessageResponseTopicTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client){
        auto status = celix_earpmClient_subscribe(client, "test/message", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_ei_expect_mosquitto_property_read_string(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

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

        auto ok = WaitForLogMessage("Failed to get response topic from sync event");
        EXPECT_TRUE(ok);

        status = celix_earpmClient_unsubscribe(client, "test/message");
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear retained message
        rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/message", 0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                  true, props);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
        }, [](const celix_earpm_client_request_info_t*) {
            ADD_FAILURE() << "Should not be called";
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToReadMessageCorrelationDataTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client){
        auto status = celix_earpmClient_subscribe(client, "test/message", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_ei_expect_mosquitto_property_read_binary(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

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

        auto ok = WaitForLogMessage("Failed to get correlation data from sync event");
        EXPECT_TRUE(ok);

        status = celix_earpmClient_unsubscribe(client, "test/message");
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear retained message
        rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/message", 0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                  true, props);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
    }, [](const celix_earpm_client_request_info_t*) {
        ADD_FAILURE() << "Should not be called";
    });
}

TEST_F(CelixEarpmClientErrorInjectionTestSuite, FailedToReadMessageSenderUUIDTest) {
    TestClientAfterConnectedBroker([this](celix_earpm_client_t* client){
        auto status = celix_earpmClient_subscribe(client, "test/message", CELIX_EARPM_QOS_AT_LEAST_ONCE);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_ei_expect_mosquitto_property_read_string_pair(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

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

        auto ok = WaitForLogMessage("Failed to get user property from sync event");
        EXPECT_TRUE(ok);

        status = celix_earpmClient_unsubscribe(client, "test/message");
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear retained message
        rc = mosquitto_publish_v5(testMosqClient, nullptr, "test/message", 0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE,
                                  true, props);
        EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
    }, [](const celix_earpm_client_request_info_t*) {
        ADD_FAILURE() << "Should not be called";
    });
}
