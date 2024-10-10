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
#include <cerrno>
#include <functional>
#include <mosquitto.h>


#include "celix_stdlib_cleanup.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_array_list_ei.h"
#include "celix_utils_ei.h"
#include "mosquitto_ei.h"
#include "celix_properties_ei.h"
#include "celix_log_helper_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_filter_ei.h"
#include "asprintf_ei.h"
#include "jansson_ei.h"
#include "celix_event_handler_service.h"
#include "celix_event_constants.h"
#include "celix_earpm_impl.h"
#include "celix_earpm_client.h"
#include "celix_earpm_event_deliverer.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "CelixEarpmImplTestSuiteBaseClass.h"


class CelixEarpmImplErrorInjectionTestSuite : public CelixEarpmImplTestSuiteBaseClass {
public:
    CelixEarpmImplErrorInjectionTestSuite() : CelixEarpmImplTestSuiteBaseClass{".earpm_impl_ej_test_cache"} { }

    ~CelixEarpmImplErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_logHelper_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celixThreadCondition_init(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_longHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_addLong(nullptr, 0, 0);
        celix_ei_expect_celix_stringHashMap_putLong(nullptr, 0, 0);
        celix_ei_expect_json_object(nullptr, 0, nullptr);
        celix_ei_expect_json_array(nullptr, 0, nullptr);
        celix_ei_expect_json_array_append_new(nullptr, 0, 0);
        celix_ei_expect_json_pack_ex(nullptr, 0, nullptr);
        celix_ei_expect_json_object_set_new(nullptr, 0, 0);
        celix_ei_expect_json_dumps(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_saveToString(nullptr, 0, 0);
        celix_ei_expect_mosquitto_publish_v5(nullptr, 0, 0);
        celix_ei_expect_celix_arrayList_createLongArray(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createStringArray(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_addString(nullptr, 0, 0);
        celix_ei_expect_celix_filter_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_loadFromString(nullptr, 0, 0);
    }

    void TestAddEventHandlerServiceErrorInjection(const std::function<void()>& errorInjection, const std::function<void(celix_status_t status)>& checkResult) {
        auto earpm = celix_earpm_create(ctx.get());
        ASSERT_NE(nullptr, earpm);

        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");

        errorInjection();

        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);

        checkResult(status);

        celix_earpm_destroy(earpm);
    }

    void TestRemoveEventHandlerServiceErrorInjection(const std::function<void()>& errorInjection, const std::function<void(celix_status_t status)>& checkResult) {
        auto earpm = celix_earpm_create(ctx.get());
        ASSERT_NE(nullptr, earpm);

        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");

        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        ASSERT_EQ(CELIX_SUCCESS, status);

        errorInjection();

        status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);

        checkResult(status);

        celix_earpm_destroy(earpm);
    }

    void TestProcessSyncEventErrorInjection(const std::function<void()>& errorInjection, const std::function<void()>& checkResult) {
        TestRemoteProvider([errorInjection, checkResult](celix_event_admin_remote_provider_mqtt_t* earpm) {
            //subscribe to the sync event
            celix_event_handler_service_t eventHandlerService;
            eventHandlerService.handle = nullptr;
            eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS;};
            celix_autoptr(celix_properties_t) props = celix_properties_create();
            celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
            celix_properties_set(props, CELIX_EVENT_TOPIC, "syncEvent");
            auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
            EXPECT_EQ(status, CELIX_SUCCESS);

            errorInjection();

            //publish the event
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
            EXPECT_EQ(rc, MOSQ_ERR_SUCCESS);
            rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "syncEvent",
                                      (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, mqttProps);
            ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

            //wait for the sync event ack that is sent by the remote provider
            auto ok = mqttClient->WaitForReceivedMsg(responseTopic);
            ASSERT_TRUE(ok);

            checkResult();

            status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
            EXPECT_EQ(status, CELIX_SUCCESS);

            //clear the retained message from the broker
            rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "syncEvent",
                                      0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, nullptr);
            ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        });
    }
};

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAllocMemoryForEventAdminRemoteProviderTest) {
    celix_ei_expect_calloc((void*)&celix_earpm_create, 0, nullptr);
    auto provider = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, provider);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateLogHelperTest) {
    celix_ei_expect_celix_logHelper_create((void*)&celix_earpm_create, 0, nullptr);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToGetFrameworkUUIDTest) {
    celix_ei_expect_celix_bundleContext_getProperty((void*)&celix_earpm_create, 0, nullptr);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToGenerateSyncEventAckTopicTest) {
    celix_ei_expect_asprintf((void*)&celix_earpm_create, 0, -1);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateMutexTest) {
    celix_ei_expect_celixThreadMutex_create((void*)&celix_earpm_create, 0, ENOMEM);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateSyncEventAckConditionTest) {
    celix_ei_expect_celixThreadCondition_init((void*)&celix_earpm_create, 0, ENOMEM);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateEventHandlerMapTest) {
    celix_ei_expect_celix_longHashMap_createWithOptions((void*)&celix_earpm_create, 0, nullptr);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateEventSubscriptionMapTest) {
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)&celix_earpm_create, 0, nullptr);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateRemoteFrameworkMapTest) {
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)&celix_earpm_create, 0, nullptr, 2);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateEventDeliveryTest) {
    celix_ei_expect_calloc((void*)&celix_earpmDeliverer_create, 0, nullptr);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateMqttClientTest) {
    celix_ei_expect_calloc((void*)&celix_earpmClient_create, 0, nullptr);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToSubscribeCtrlMessageOfEventAdminRemoteProviderTest) {
    celix_ei_expect_celix_stringHashMap_putLong((void*)&celix_earpmClient_subscribe, 0, ENOMEM);
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_EQ(nullptr, earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDupEventHandlerFilterInfoTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_celix_utils_strdup((void*)&celix_earpm_addEventHandlerService, 1, nullptr);
        },
        [](celix_status_t status) {
            EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAllocMemoryForEventHandlerTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_calloc((void*)&celix_earpm_addEventHandlerService, 1, nullptr);
        },
        [](celix_status_t status) {
            EXPECT_EQ(status, CELIX_SERVICE_EXCEPTION);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPutEventHandlerServiceToMapTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_celix_longHashMap_put((void*)&celix_earpm_addEventHandlerService, 0, ENOMEM);
        },
        [](celix_status_t status) {
            EXPECT_EQ(status, ENOMEM);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAllocMemoryForSubscriptionTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_calloc((void*)&celix_earpm_addEventHandlerService, 3, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to create subscription info");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateHandlerListForSubscriptionTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_celix_arrayList_create((void*)&celix_earpm_addEventHandlerService, 3, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to create subscription info");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPutSubscriptionToMapTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_celix_stringHashMap_put((void*)&celix_earpm_addEventHandlerService, 2, ENOMEM);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to add subscription info");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAttachHandlerToSubscriptionTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_celix_arrayList_addLong((void*)&celix_earpm_addEventHandlerService, 2, ENOMEM);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to attach handler service");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToSubscribeTopicTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_celix_stringHashMap_putLong((void*)&celix_earpmClient_subscribe, 0, ENOMEM);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to subscribe topic with qos 0.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateHandlerInfoAddMessagePayloadTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_object((void*)&celix_earpm_addEventHandlerService, 1, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to create adding handler info message payload.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateTopicArrayListForHandlerInfoAddMessageTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_array((void*)&celix_earpm_addEventHandlerService, 2, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to create topic json array.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddTopicToHandlerInfoAddMessageTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_array_append_new((void*)&celix_earpm_addEventHandlerService, 2, -1);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to add topic to json array.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPackHandlerInfoOfHandlerInfoAddMessageTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_pack_ex((void*)&celix_earpm_addEventHandlerService, 2, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to pack handler information.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddEventHandlerFilterInfoToHandlerInfoAddMessageTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_object_set_new((void*)&celix_earpm_addEventHandlerService, 2, -1);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to add filter to handler information.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddHandlerInfoToMessagePayloadTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_object_set_new((void*)&celix_earpm_addEventHandlerService, 1, -1);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to add handler information to adding handler information message.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDumpHandlerInfoAddMessagePayloadTest) {
    TestAddEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_dumps((void*)&celix_earpm_addEventHandlerService, 1, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to dump adding handler information message payload");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPublishHandlerInfoAddMessageToRemoteWhenAddingEventHandlerServiceTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_ei_expect_mosquitto_publish_v5((void*)&celix_earpmClient_publishAsync, 2, -1);

        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(CELIX_SUCCESS, status);

        std::string expectedLog = "Failed to publish " + std::string(CELIX_EARPM_HANDLER_INFO_ADD_TOPIC);
        auto ok = WaitForLogMessage(expectedLog);
        EXPECT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateHandlerInfoRemoveMessagePayloadTest) {
    TestRemoveEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_object((void*)&celix_earpm_removeEventHandlerService, 1, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to create removing handler info message payload.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddHandlerIdToHandlerInfoRemoveMessageTest) {
    TestRemoveEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_object_set_new((void*)&celix_earpm_removeEventHandlerService, 1, -1);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to add handler id to removing handler info message.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDumpHandlerInfoRemoveMessagePayloadTest) {
    TestRemoveEventHandlerServiceErrorInjection(
        []() {
            celix_ei_expect_json_dumps((void*)&celix_earpm_removeEventHandlerService, 1, nullptr);
        },
        [this](celix_status_t) {
            auto ok = WaitForLogMessage("Failed to dump removing handler information message payload");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPublishHandlerInfoRemoveMessageToRemoteWhenRemovingEventHandlerServiceTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(CELIX_SUCCESS, status);

        celix_ei_expect_mosquitto_publish_v5((void*)&celix_earpmClient_publishAsync, 2, -1);
        status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);

        std::string expectedLog = "Failed to publish " + std::string(CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC);
        auto ok = WaitForLogMessage(expectedLog);
        EXPECT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, NotFoundSubscriptionWhenRemoveEventHandlerTest) {
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_NE(nullptr, earpm);

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");

    celix_ei_expect_celix_stringHashMap_put((void*)&celix_earpm_addEventHandlerService, 2, ENOMEM);
    (void)celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    auto ok = WaitForLogMessage("Failed to add subscription info");
    EXPECT_TRUE(ok);

    auto status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(CELIX_SUCCESS, status);

    ok = WaitForLogMessage("No subscription found");
    EXPECT_TRUE(ok);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, NotFoundHandlerInfoWhenRemoveEventHandlerTest) {
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_NE(nullptr, earpm);

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
    celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");
    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    ASSERT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_arrayList_addLong((void*)&celix_earpm_addEventHandlerService, 2, ENOMEM);
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 124);
    (void)celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    auto ok = WaitForLogMessage("Failed to attach handler service");
    EXPECT_TRUE(ok);

    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(CELIX_SUCCESS, status);
    ok = WaitForLogMessage("Not found handler");
    EXPECT_TRUE(ok);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToChangeSubscribedTopicQOSWhenRemoveEventHandlerTest) {
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

    celix_ei_expect_celix_stringHashMap_putLong((void*)&celix_earpmClient_subscribe, 0, ENOMEM);
    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props2);
    EXPECT_EQ(status, CELIX_SUCCESS);
    auto ok = WaitForLogMessage("Failed to subscribe topic with qos 0.");
    EXPECT_TRUE(ok);

    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props1);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToUnsubscribeTopicWhenRemoveEventHandlerTest) {
    auto earpm = celix_earpm_create(ctx.get());
    ASSERT_NE(nullptr, earpm);

    celix_event_handler_service_t eventHandlerService;
    eventHandlerService.handle = nullptr;
    eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
    celix_autoptr(celix_properties_t) props = celix_properties_create();
    celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
    celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");

    auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
    ASSERT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_celix_stringHashMap_putLong((void*)&celix_earpmClient_unsubscribe, 0, ENOMEM);
    status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto ok = WaitForLogMessage("Failed to unsubscribe topic.");
    EXPECT_TRUE(ok);

    celix_earpm_destroy(earpm);
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToSerializeAsyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_celix_properties_saveToString((void*)&celix_earpm_postEvent, 1, ENOMEM);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_postEvent(earpm, "topic", props);
        EXPECT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPublishAsyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_mosquitto_publish_v5((void*)&celix_earpmClient_publishAsync, 2, -1);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_postEvent(earpm, "topic", props);
        EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToSerializeSyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_celix_properties_saveToString((void*)&celix_earpm_sendEvent, 1, ENOMEM);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_sendEvent(earpm, "topic", props);
        EXPECT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPublishSyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_mosquitto_publish_v5((void*)&celix_earpmClient_publishSync, 2, -1);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_sendEvent(earpm, "topic", props);
        EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateRemoteHandlerListForSyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_celix_arrayList_createLongArray((void*)&celix_earpm_sendEvent, 3, nullptr);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_sendEvent(earpm, "topic", props);
        EXPECT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddRemoteHandlerToSyncEventRemoteHandlerListTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_celix_arrayList_addLong((void*)&celix_earpm_sendEvent, 3, ENOMEM);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_sendEvent(earpm, "topic", props);
        EXPECT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAssociateRemoteHandlerWithSyncEventTest) {
    TestRemoteProvider([](celix_event_admin_remote_provider_mqtt_t* earpm) {
        AddRemoteHandlerInfoToRemoteProviderAndCheck(earpm, R"({"handler":{"handlerId":123,"topics":["topic"]}})");

        celix_ei_expect_celix_longHashMap_put((void*)&celix_earpm_sendEvent, 3, ENOMEM);
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        auto status = celix_earpm_sendEvent(earpm, "topic", props);
        EXPECT_EQ(ENOMEM, status);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAllocMemoryForRemoteFrameworkInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to allocate memory for remote framework");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateEventAckMapForRemoteFrameworkInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_longHashMap_createWithOptions(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create event ack seq number map for remote framework");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateHandlerInfoMapForRemoteFrameworkInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_longHashMap_createWithOptions(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create handler information map for remote framework");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddRemoteFrameworkInfoToRemoteFrameworkMapWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to add remote framework info");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAllocMemoryForRemoteHandlerInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to allocate memory for handler information.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateTopicListForRemoteHandlerInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_arrayList_createStringArray(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create topics list for handler information.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddTopicToRemoteHandlerInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_arrayList_addString(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handler":{"handlerId":123, "topics":["topic"]}})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to add topic(topic) to handler information.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateFilterForRemoteHandlerInfoWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_filter_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"s({"handler":{"handlerId":123, "topics":["topic"], "filter":"(a=b)"}})s";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create filter((a=b)) for handler information.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddRemoteHandlerInfoToRemoteHandlerMapWhenProcessAddRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_celix_longHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"s({"handler":{"handlerId":123, "topics":["topic"], "filter":"(a=b)"}})s";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_ADD_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to add remote handler information.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateRemoteFrameworkInfoWhenProcessUpdateRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        const char* payload = R"({"handlers":[{"handlerId":123, "topics":["topic"]}]})";
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC,
                                  (int)strlen(payload), payload, CELIX_EARPM_QOS_AT_MOST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to allocate memory for remote framework");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateHandlerInfoUpdateMessagePayloadWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_json_object(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create updating handlers info message payload.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToCreateHandlerInfoArrayWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_json_array(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create handler information array.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddHandlersInfoToHandlerInfoUpdateMessageWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_json_object_set_new(CELIX_EI_UNKNOWN_CALLER, 0, -1);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to add handlers information to updating handler info message.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDumpHandlerInfoUpdateMessagePayloadWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_json_dumps(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to dump updating handler information message payload.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToGenerateHandlerInfoWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        ASSERT_EQ(CELIX_SUCCESS, status);

        celix_ei_expect_json_pack_ex(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to create handler information for handler 123.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAddHandlerInfoToHandlerInfoListWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS; };
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "topic");
        celix_properties_set(props, CELIX_EVENT_FILTER, "(a=b)");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        ASSERT_EQ(CELIX_SUCCESS, status);

        celix_ei_expect_json_array_append_new(CELIX_EI_UNKNOWN_CALLER, 0, -1, 2);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);

        auto ok = WaitForLogMessage("Failed to add information of handler 123.");
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToPublishHandlerInfoUpdateMessageWhenProcessQueryRemoteHandlerInfoMessageTest) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t*) {
        celix_ei_expect_mosquitto_publish_v5((void*)&celix_earpmClient_publishAsync, 2, -1);

        celix_autoptr(mosquitto_property) properties = CreateMqttProperties();
        auto rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC,
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, false, properties);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
        std::string expectLog = "Failed to publish " + std::string(CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC);
        auto ok = WaitForLogMessage(expectLog);
        ASSERT_TRUE(ok);
    });
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToUnserializeSyncEvent) {
    TestProcessSyncEventErrorInjection(
        []() {
            celix_ei_expect_celix_properties_loadFromString(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);
        },
        [this]() {
            auto ok = WaitForLogMessage("Failed to load event properties for syncEvent.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToAllocMemoryForSyncEventCallbackData) {
    TestProcessSyncEventErrorInjection(
        []() {
            celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        },
        [this]() {
            auto ok = WaitForLogMessage("Failed to allocate memory for send done callback data.");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDupSyncEventResponseTopic) {
    TestProcessSyncEventErrorInjection(
        []() {
            celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        },
        [this]() {
            auto ok = WaitForLogMessage("Failed to get response topic from sync event");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDupSyncEventCorrelationData) {
    TestProcessSyncEventErrorInjection(
        []() {
            celix_ei_expect_malloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        },
        [this]() {
            auto ok = WaitForLogMessage("Failed to allocate memory for correlation data of sync event");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToDelieverSyncEventToLocalHandler) {
    TestProcessSyncEventErrorInjection(
        []() {
            celix_ei_expect_calloc((void*)&celix_earpmDeliverer_sendEvent, 0, nullptr);
        },
        [this]() {
            auto ok = WaitForLogMessage("Failed to send event to local handler");
            EXPECT_TRUE(ok);
        }
    );
}

TEST_F(CelixEarpmImplErrorInjectionTestSuite, FailedToUnserializeAsyncEvent) {
    TestRemoteProvider([this](celix_event_admin_remote_provider_mqtt_t* earpm) {
        //subscribe to the async event
        celix_event_handler_service_t eventHandlerService;
        eventHandlerService.handle = nullptr;
        eventHandlerService.handleEvent = [](void*, const char*, const celix_properties_t*) { return CELIX_SUCCESS;};
        celix_autoptr(celix_properties_t) props = celix_properties_create();
        celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_ID, 123);
        celix_properties_set(props, CELIX_EVENT_TOPIC, "asyncEvent");
        auto status = celix_earpm_addEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(status, CELIX_SUCCESS);

        celix_ei_expect_celix_properties_loadFromString(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);

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

        auto ok = WaitForLogMessage("Failed to load event properties for asyncEvent.");
        EXPECT_TRUE(ok);

        status = celix_earpm_removeEventHandlerService(earpm, &eventHandlerService, props);
        EXPECT_EQ(status, CELIX_SUCCESS);

        //clear the retained message from the broker
        rc = mosquitto_publish_v5(mqttClient->mosq.get(), nullptr, "asyncEvent",
                                  0, nullptr, CELIX_EARPM_QOS_AT_LEAST_ONCE, true, nullptr);
        ASSERT_EQ(rc, MOSQ_ERR_SUCCESS);
    });
}