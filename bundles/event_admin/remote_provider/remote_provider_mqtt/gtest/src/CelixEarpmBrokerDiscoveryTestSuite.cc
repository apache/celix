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
#include <functional>
#include <cstdlib>
#include <future>
#include <thread>
#include <netinet/in.h>

#include "endpoint_listener.h"
#include "remote_constants.h"
#include "celix_earpm_broker_discovery.h"
#include "celix_earpm_constants.h"
#include "CelixEarpmTestSuiteBaseClass.h"

class CelixEarpmBrokerDiscoveryTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmBrokerDiscoveryTestSuite(): CelixEarpmTestSuiteBaseClass{".earpm_broker_discovery_test_cache"} {};
    ~CelixEarpmBrokerDiscoveryTestSuite() override = default;

    static celix_properties_t* CreateBrokerEndpointListenerServiceProperties(void) {
        auto properties = celix_properties_create();
        char scope[512] = {0};
        snprintf(scope, 512, R"((&(%s=%s)(%s=%s)))", CELIX_FRAMEWORK_SERVICE_NAME, CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME,
                 CELIX_RSA_SERVICE_IMPORTED_CONFIGS, CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE);
        auto status = celix_properties_set(properties, CELIX_RSA_ENDPOINT_LISTENER_SCOPE, scope);
        EXPECT_EQ(status, CELIX_SUCCESS);
        status = celix_properties_setBool(properties, CELIX_RSA_DISCOVERY_INTERFACE_SPECIFIC_ENDPOINTS_SUPPORT, true);
        EXPECT_EQ(status, CELIX_SUCCESS);
        return properties;
    }

    using update_endpoint_fp = celix_status_t (void *handle, endpoint_description_t *endpoint, char *matchedFilter);
    long RegisterBrokerEndpointListenerService(void* handle, update_endpoint_fp endpointAdded, update_endpoint_fp endpointRemoved = nullptr) {
        static endpoint_listener_t listener{};
        listener.handle = handle;
        listener.endpointAdded = endpointAdded;
        listener.endpointRemoved = endpointRemoved != nullptr ? endpointRemoved :
                [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS; };
        auto properties = CreateBrokerEndpointListenerServiceProperties();
        return celix_bundleContext_registerService(ctx.get(), &listener, CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME, properties);
    }

    long TrackEndpointListenerForDiscovery(celix_earpm_broker_discovery_t* discovery) {
        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME;
        opts.callbackHandle = discovery;
        opts.addWithProperties = [](void* handle, void* svc, const celix_properties_t* props) {
            celix_earpmDiscovery_addEndpointListener(handle, svc, props);
        };
        opts.removeWithProperties = [](void* handle, void* svc, const celix_properties_t* props) {
            celix_earpmDiscovery_removeEndpointListener(handle, svc, props);
        };
        return celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    }

    void ParseBrokerProfileTest(const char* profile, const std::function<void(endpoint_description_t*)>& checkBrokerInfoFp = nullptr) {
        setenv(CELIX_EARPM_BROKER_PROFILE, profile, 1);
        auto discovery = celix_earpmDiscovery_create(ctx.get());
        ASSERT_NE(discovery, nullptr);
        auto status = celix_earpmDiscovery_start(discovery);
        EXPECT_EQ(status, CELIX_SUCCESS);

        std::promise<void> endpointPromise;
        auto endpointFuture = endpointPromise.get_future();
        std::function<void(endpoint_description_t*)> checkBrokerInfoWrapper = [&endpointPromise, checkBrokerInfoFp](endpoint_description_t* endpoint) {
            EXPECT_STREQ(CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME, endpoint->serviceName);
            EXPECT_STREQ(CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE, celix_properties_get(endpoint->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, nullptr));
            if (checkBrokerInfoFp) {
                checkBrokerInfoFp(endpoint);
            }
            try {
                endpointPromise.set_value();
            } catch (...) {
                // Ignore
            }
        };
        auto eplId = RegisterBrokerEndpointListenerService(&checkBrokerInfoWrapper, [](void* handle, endpoint_description_t* endpoint, char*) {
            auto checkBrokerInfoFunc = static_cast<std::function<void(endpoint_description_t*)>*>(handle);
            (*checkBrokerInfoFunc)(endpoint);
            return CELIX_SUCCESS;
        });
        ASSERT_GE(eplId, 0);

        auto eplTrackerId = TrackEndpointListenerForDiscovery(discovery);
        ASSERT_GE(eplTrackerId, 0);

        auto rc = endpointFuture.wait_for(std::chrono::seconds{30});
        EXPECT_EQ(rc, std::future_status::ready);

        celix_bundleContext_stopTracker(ctx.get(), eplTrackerId);
        celix_bundleContext_unregisterService(ctx.get(), eplId);

        status = celix_earpmDiscovery_stop(discovery);
        EXPECT_EQ(status, CELIX_SUCCESS);
        celix_earpmDiscovery_destroy(discovery);
        unsetenv(CELIX_EARPM_BROKER_PROFILE);
    }

    void ParseInvalidBrokerProfileTest(const char* profile) {
        setenv(CELIX_EARPM_BROKER_PROFILE, profile, 1);
        auto discovery = celix_earpmDiscovery_create(ctx.get());
        ASSERT_NE(discovery, nullptr);
        auto status = celix_earpmDiscovery_start(discovery);
        EXPECT_EQ(status, CELIX_SUCCESS);


        auto eplId = RegisterBrokerEndpointListenerService(nullptr, [](void*, endpoint_description_t*, char*) {
            ADD_FAILURE() << "Unexpected endpoint added";
            return CELIX_SUCCESS;
        });
        ASSERT_GE(eplId, 0);
        auto eplTrackerId = TrackEndpointListenerForDiscovery(discovery);
        ASSERT_GE(eplTrackerId, 0);

        std::this_thread::sleep_for(std::chrono::milliseconds{10});//let the discovery service process the profile

        celix_bundleContext_stopTracker(ctx.get(), eplTrackerId);
        celix_bundleContext_unregisterService(ctx.get(), eplId);

        status = celix_earpmDiscovery_stop(discovery);
        EXPECT_EQ(status, CELIX_SUCCESS);
        celix_earpmDiscovery_destroy(discovery);
        unsetenv(CELIX_EARPM_BROKER_PROFILE);
    }
};

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, CreateBrokerDiscoveryTest) {
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(discovery, nullptr);
    celix_earpmDiscovery_destroy(discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, StartBrokerDiscoveryTest) {
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(discovery, nullptr);
    auto status = celix_earpmDiscovery_start(discovery);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_earpmDiscovery_stop(discovery);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_earpmDiscovery_destroy(discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindAllInterfaceBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_all_if.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("all", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindSpecificInterfaceBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_if.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("lo", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindHostBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_host.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 0));
        EXPECT_STREQ("127.0.0.1", celix_properties_get(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, nullptr));
        EXPECT_STREQ("all", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseStartWithSpaceBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_start_with_space.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 0));
        EXPECT_STREQ("127.0.0.1", celix_properties_get(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, nullptr));
        EXPECT_STREQ("all", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindHostAndInterfaceBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_host_and_if.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, 0));
        EXPECT_STREQ("127.0.0.1", celix_properties_get(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, nullptr));
        EXPECT_STREQ("lo", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindIpv4BrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_ipv4.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_EQ(AF_INET, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("lo", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindIpv6BrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_ipv6.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_EQ(AF_INET6, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("lo", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseMultilListenerBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_multi_listener.conf", [](endpoint_description_t* endpoint) {
        auto port = celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0);
        if (port != 1883 && port != 1884) {
            FAIL() << "Unexpected broker port " << port;
        }
        EXPECT_EQ(AF_INET, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("lo", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseSocketDomainLostedBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_socket_domain_val_lost.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_EQ(-1, celix_properties_getAsLong(endpoint->properties, CELIX_EARPM_MQTT_BROKER_SOCKET_DOMAIN, -1));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("lo", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindInterfaceLostedBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_if_val_lost.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("all", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseBindUnixSocketBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_unix_socket.conf", [](endpoint_description_t* endpoint) {
        EXPECT_EQ(nullptr, celix_properties_get(endpoint->properties, CELIX_EARPM_MQTT_BROKER_PORT, nullptr));
        EXPECT_STREQ("/var/mosquitto.sock", celix_properties_get(endpoint->properties, CELIX_EARPM_MQTT_BROKER_ADDRESS, nullptr));
    });
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, BrokerProfileNotExistedTest) {
    ParseInvalidBrokerProfileTest(MOSQUITTO_CONF_PATH"not_existed.conf");
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, ParseInvalidBrokerProfileTest) {
    ParseInvalidBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_invalid_listener.conf");
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, DiscoveryStartAfterEndpointListenerAddedTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto_bind_all_if.conf", 1);
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(discovery, nullptr);

    std::promise<void> endpointPromise;
    auto endpointFuture = endpointPromise.get_future();
    auto eplId = RegisterBrokerEndpointListenerService(&endpointPromise, [](void* handle, endpoint_description_t* endpoint, char*) {
        EXPECT_STREQ(CELIX_EARPM_MQTT_BROKER_INFO_SERVICE_NAME, endpoint->serviceName);
        EXPECT_STREQ(CELIX_EARPM_MQTT_BROKER_SERVICE_CONFIG_TYPE, celix_properties_get(endpoint->properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, nullptr));
        EXPECT_EQ(1883, celix_properties_getAsLong(endpoint->properties, CELIX_RSA_PORT, 0));
        EXPECT_STREQ("", celix_properties_get(endpoint->properties, CELIX_RSA_IP_ADDRESSES, nullptr));
        EXPECT_STREQ("all", celix_properties_get(endpoint->properties, CELIX_RSA_EXPORTED_ENDPOINT_EXPOSURE_INTERFACE, nullptr));

        auto promise = static_cast<std::promise<void>*>(handle);
        promise->set_value();
        return CELIX_SUCCESS;
    });
    ASSERT_GE(eplId, 0);

    auto eplTrackerId = TrackEndpointListenerForDiscovery(discovery);
    ASSERT_GE(eplTrackerId, 0);

    auto status = celix_earpmDiscovery_start(discovery);
    EXPECT_EQ(status, CELIX_SUCCESS);

    auto rc = endpointFuture.wait_for(std::chrono::seconds{30});
    EXPECT_EQ(rc, std::future_status::ready);

    status = celix_earpmDiscovery_stop(discovery);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_bundleContext_stopTracker(ctx.get(), eplTrackerId);
    celix_bundleContext_unregisterService(ctx.get(), eplId);

    celix_earpmDiscovery_destroy(discovery);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, EndpointListenerServicePropertiesInvalidTest) {
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(discovery, nullptr);

    celix_autoptr(celix_properties_t) properties = celix_properties_create();
    ASSERT_NE(properties, nullptr);
    endpoint_listener_t listener{};
    listener.handle = nullptr;
    listener.endpointAdded = [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS; };
    listener.endpointRemoved = [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS; };

    auto status = celix_earpmDiscovery_addEndpointListener(discovery, &listener, properties);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    status = celix_earpmDiscovery_removeEndpointListener(discovery, &listener, properties);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    celix_earpmDiscovery_destroy(discovery);
}

TEST_F(CelixEarpmBrokerDiscoveryTestSuite, RemoveNotExistedEndpointListenerServiceTest) {
    auto discovery = celix_earpmDiscovery_create(ctx.get());
    ASSERT_NE(discovery, nullptr);

    celix_autoptr(celix_properties_t) properties = celix_properties_create();
    ASSERT_NE(properties, nullptr);
    celix_properties_setLong(properties, CELIX_FRAMEWORK_SERVICE_ID, 123);
    endpoint_listener_t listener{};
    listener.handle = nullptr;
    listener.endpointAdded = [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS; };
    listener.endpointRemoved = [](void*, endpoint_description_t*, char*) { return CELIX_SUCCESS; };

    auto status = celix_earpmDiscovery_removeEndpointListener(discovery, &listener, properties);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_earpmDiscovery_destroy(discovery);
}
