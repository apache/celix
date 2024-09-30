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
#include <cerrno>
#include <cstdlib>
#include <functional>

extern "C" {
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "endpoint_description.h"
}
#include "malloc_ei.h"
#include "celix_log_helper_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_threads_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_filter_ei.h"
#include "celix_array_list_ei.h"
#include "celix_utils_ei.h"
#include "celix_properties_ei.h"
#include "celix_earpm_broker_discovery.h"
#include "celix_earpm_constants.h"
#include "CelixEarpmTestSuiteBaseClass.h"


class CelixEarpmBrokerDiscoveryErrorInjectionTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmBrokerDiscoveryErrorInjectionTestSuite() : CelixEarpmTestSuiteBaseClass{".earpm_broker_discovery_ej_test_cache", "celix_earpm_discovery"} {};

    ~CelixEarpmBrokerDiscoveryErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_logHelper_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_scheduleEvent(nullptr, 0, 0);
        celix_ei_expect_celix_longHashMap_put(nullptr, 0, 0);
        celix_ei_expect_celix_filter_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_add(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_set(nullptr, 0, 0);
    };

    void TestAddingEndpointListener(const std::function<void(celix_earpm_broker_discovery_t*, void*, const celix_properties_t*)>& testBody) {
        setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
        auto discovery = celix_earpmDiscovery_create(ctx.get());
        ASSERT_NE(nullptr, discovery);

        celix_autoptr(celix_properties_t) properties = celix_properties_create();
        char scope[512] = {0};
        snprintf(scope, 512, R"((&(%s=%s)(%s=%s)))", CELIX_FRAMEWORK_SERVICE_NAME, CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME,
                 CELIX_RSA_SERVICE_IMPORTED_CONFIGS, CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE);
        auto status = celix_properties_set(properties, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_properties_setBool(properties, CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT, true);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_properties_setLong(properties, CELIX_FRAMEWORK_SERVICE_ID, 123);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME);
        EXPECT_EQ(status, CELIX_SUCCESS);
        endpoint_listener_t endpointListener;
        endpointListener.handle = nullptr;
        endpointListener.endpointAdded = [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS;};
        endpointListener.endpointRemoved = [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS;};

        testBody(discovery, &endpointListener, properties);

        celix_earpmDiscovery_destroy(discovery);
        unsetenv(CELIX_EARPM_BROKER_PROFILE);
    }
};

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateLogHelperTest) {
    celix_ei_expect_celix_logHelper_create((void*)&celix_earpmDiscovery_create, 0, nullptr);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_EQ(nullptr, discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToAllocMemoryForDiscoveryTest) {
    celix_ei_expect_calloc((void*)&celix_earpmDiscovery_create, 0, nullptr);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_EQ(nullptr, discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToGetFrameworkUUIDTest) {
    celix_ei_expect_celix_bundleContext_getProperty((void*)&celix_earpmDiscovery_create, 0, nullptr);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_EQ(nullptr, discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateMutexTest) {
    celix_ei_expect_celixThreadMutex_create((void*)&celix_earpmDiscovery_create, 0, ENOMEM);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_EQ(nullptr, discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateEndpointListenerMapTest) {
    celix_ei_expect_celix_longHashMap_createWithOptions((void*)&celix_earpmDiscovery_create, 0, nullptr);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_EQ(nullptr, discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToScheduledProfileEventTest) {
    celix_ei_expect_celix_bundleContext_scheduleEvent((void*)&celix_earpmDiscovery_create, 0, -1);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_EQ(nullptr, discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateEndpointListenerFilterTest) {
    TestAddingEndpointListener([](celix_earpm_broker_discovery_t *discovery, void *endpointListener,
                                  const celix_properties_t *properties) {
        celix_ei_expect_celix_filter_create((void *) &celix_earpmDiscovery_addEndpointListener, 0, nullptr);
        auto status = celix_earpmDiscovery_addEndpointListener(discovery, &endpointListener, properties);
        ASSERT_EQ(status, CELIX_ENOMEM);
    });
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToAllocMemoryForEndpointListenerEntryTest) {
    TestAddingEndpointListener([](celix_earpm_broker_discovery_t *discovery, void *endpointListener,
                                  const celix_properties_t *properties) {
        celix_ei_expect_calloc((void *) &celix_earpmDiscovery_addEndpointListener, 0, nullptr);
        auto status = celix_earpmDiscovery_addEndpointListener(discovery, &endpointListener, properties);
        ASSERT_EQ(status, CELIX_ENOMEM);
    });
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToAddEndpointListenerEntryToMapTest) {
    TestAddingEndpointListener([](celix_earpm_broker_discovery_t *discovery, void *endpointListener,
                                  const celix_properties_t *properties) {
        celix_ei_expect_celix_longHashMap_put((void *) &celix_earpmDiscovery_addEndpointListener, 0, ENOMEM);
        auto status = celix_earpmDiscovery_addEndpointListener(discovery, &endpointListener, properties);
        ASSERT_EQ(status, CELIX_ENOMEM);
    });
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateBrokerListenerListTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_celix_arrayList_createWithOptions(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to create broker listeners list.");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToAllocMemoryForBrokerListenerTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 3);//first calloc for log helper, second for discovery
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to create broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToDupBrokerListenerHostNameTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto_bind_host.conf", 1);
    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);//first for log helper
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to create broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToAddBrokerListenerToListTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_celix_arrayList_add(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to add broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToDupBrokerListenerBindInterfaceTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto_bind_if.conf", 1);
    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);//first for log helper
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to dup bind interface");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateBrokerEndpointListTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_celix_arrayList_createWithOptions(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to create broker endpoints list.");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreatePropertiesForBrokerEndpointTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to create properties for mqtt broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToSetPropertiesValForBrokerEndpointTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_celix_properties_set(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to set properties for mqtt broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToCreateBrokerEndpointTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 4);//first calloc for log helper, second for discovery, third broker listener
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to create endpoint for mqtt broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryErrorInjectionTestSuite, FailedToAddBrokerEndpointToListTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto.conf", 1);
    celix_ei_expect_celix_arrayList_add(CELIX_EI_UNKNOWN_CALLER, 0, ENOMEM, 2);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(nullptr, discovery);

    auto ok = WaitForLogMessage("Failed to add endpoint for mqtt broker listener");
    ASSERT_TRUE(ok);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}
