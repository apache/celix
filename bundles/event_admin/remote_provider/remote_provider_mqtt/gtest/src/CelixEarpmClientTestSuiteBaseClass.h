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

#ifndef CELIX_CELIXEARPMCLIENTTESTSUITEBASECLASS_H
#define CELIX_CELIXEARPMCLIENTTESTSUITEBASECLASS_H

#include <cstring>
#include <string>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <functional>
#include <future>
#include <thread>
#include <netinet/in.h>
#include <mosquitto.h>
#include <mqtt_protocol.h>
#include <uuid/uuid.h>


#include "celix_log_helper.h"
extern "C" {
#include "endpoint_description.h"
}
#include "remote_constants.h"
#include "celix_earpm_client.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "CelixEarpmTestSuiteBaseClass.h"


namespace {
    constexpr const char *MQTT_BROKER_ADDRESS = "127.0.0.1";
    constexpr int MQTT_BROKER_PORT = 1883;
}

class CelixEarpmClientTestSuiteBaseClass : public CelixEarpmTestSuiteBaseClass {
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

    explicit CelixEarpmClientTestSuiteBaseClass(const char* testCache = ".earpm_client_test_cache") : CelixEarpmTestSuiteBaseClass{testCache, "earpm_client"} {
        logHelper = std::shared_ptr <celix_log_helper_t>
                {celix_logHelper_create(ctx.get(), "earpm_client"), [](celix_log_helper_t *h) { celix_logHelper_destroy(h); }};

        defaultOpts.ctx = ctx.get();
        defaultOpts.logHelper = logHelper.get();
        defaultOpts.sessionEndMsgTopic = CELIX_EARPM_SESSION_END_TOPIC;
        defaultOpts.sessionEndMsgSenderUUID = fwUUID.c_str();
        defaultOpts.sessionEndMsgVersion = "1.0.0";
        defaultOpts.callbackHandle = nullptr;
        defaultOpts.receiveMsgCallback = [](void*, const celix_earpm_client_request_info_t*) {};
        defaultOpts.connectedCallback = [](void*) {};
    }

    ~CelixEarpmClientTestSuiteBaseClass() override = default;

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

    void TestAddMqttBrokerEndpoint(const std::function<void (endpoint_description_t*)>& modifyEndpoint) const {
        std::promise<void> clientConnectedPromise;
        auto clientConnectedFuture = clientConnectedPromise.get_future();

        celix_earpm_client_create_options_t opts{defaultOpts};
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

    void TestClient(const std::function<void (celix_earpm_client_t*)>& testBody) const {
        celix_earpm_client_create_options_t opts{defaultOpts};
        auto* client = celix_earpmClient_create(&opts);
        ASSERT_NE(client, nullptr);
        testBody(client);
        celix_earpmClient_destroy(client);
    }

    void TestClient(const std::function<void (celix_earpm_client_t*)>& testBody, const std::function<void (void)>& connectedCallback,
                    const std::function<void (const celix_earpm_client_request_info_t*)>& receiveMsgCallBack = [](const celix_earpm_client_request_info_t*){}) const {
        celix_earpm_client_create_options_t opts{defaultOpts};
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
                                        const std::function<void (const celix_earpm_client_request_info_t*)>& receiveMsgCallBack = [](const celix_earpm_client_request_info_t*){}) const {
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
    celix_earpm_client_create_options_t defaultOpts{};
};

pid_t CelixEarpmClientTestSuiteBaseClass::pid{0};
mosquitto* CelixEarpmClientTestSuiteBaseClass::testMosqClient{nullptr};


#endif //CELIX_CELIXEARPMCLIENTTESTSUITEBASECLASS_H
