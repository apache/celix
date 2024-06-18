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

#include "celix_earpm_broker_profile_parser.h"
#include "CelixEarpmTestSuiteBaseClass.h"
#include "celix_earpm_constants.h"
#include "celix_mqtt_broker_info_service.h"

class CelixEarpmBrokerProfileParserTestSuite : public CelixEarpmTestSuiteBaseClass {
public:
    CelixEarpmBrokerProfileParserTestSuite(): CelixEarpmTestSuiteBaseClass{".earpm_broker_profile_parser_test_cache"} {};
    ~CelixEarpmBrokerProfileParserTestSuite() override = default;

    void ParseBrokerProfileTest(const char* profile) {
        setenv(CELIX_EARPM_BROKER_PROFILE, profile, 1);
        auto parser = celix_earpmp_create(ctx.get());
        ASSERT_NE(parser, nullptr);
        celix_service_use_options_t opts{};
        opts.filter.serviceName = CELIX_MQTT_BROKER_INFO_SERVICE_NAME;
        opts.waitTimeoutInSeconds = 30;
        bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(found);
        celix_earpmp_destroy(parser);
        unsetenv(CELIX_EARPM_BROKER_PROFILE);
    }
};

TEST_F(CelixEarpmBrokerProfileParserTestSuite, CreateBrokerProfileParserTest) {
    auto parser = celix_earpmp_create(ctx.get());
    ASSERT_NE(parser, nullptr);
    celix_earpmp_destroy(parser);
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, BrokerProfileNotExistedTest) {
    auto parser = celix_earpmp_create(ctx.get());
    ASSERT_NE(parser, nullptr);
    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_MQTT_BROKER_INFO_SERVICE_NAME;
    opts.waitTimeoutInSeconds = 0.1;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_FALSE(found);
    celix_earpmp_destroy(parser);
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseBindAllInterfaceBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_all_if.conf");
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseBindSpecificInterfaceBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_if.conf");
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseBindHostBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_bind_host.conf");
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseBindIpv4BrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_ipv4.conf");
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseBindIpv6BrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_ipv6.conf");
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseMultilListenerBrokerProfileTest) {
    ParseBrokerProfileTest(MOSQUITTO_CONF_PATH"mosquitto_multi_listener.conf");
}

TEST_F(CelixEarpmBrokerProfileParserTestSuite, ParseInvalidBrokerProfileTest) {
    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF_PATH"mosquitto_invalid_listener.conf", 1);
    auto parser = celix_earpmp_create(ctx.get());
    ASSERT_NE(parser, nullptr);
    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_MQTT_BROKER_INFO_SERVICE_NAME;
    opts.waitTimeoutInSeconds = 0.1;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_FALSE(found);
    celix_earpmp_destroy(parser);
    unsetenv(CELIX_EARPM_BROKER_PROFILE);
}

//TEST_F(CelixEarpmBrokerProfileParserTestSuite, InterfaceChangedTest) {
//    setenv(CELIX_EARPM_BROKER_PROFILE, MOSQUITTO_CONF, 1);
//    auto parser = celix_earpmp_create(ctx.get());
//    ASSERT_NE(parser, nullptr);
//    celix_service_use_options_t opts{};
//    opts.filter.serviceName = CELIX_MQTT_BROKER_INFO_SERVICE_NAME;
//    opts.filter.versionRange = CELIX_MQTT_BROKER_INFO_SERVICE_RANGE;
//    opts.waitTimeoutInSeconds = 30;
//    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
//    EXPECT_TRUE(found);
//
//    system("sudo ifconfig lo:2 127.0.0.2 up");
//    opts.filter.serviceName = CELIX_MQTT_BROKER_INFO_SERVICE_NAME;
//    opts.filter.versionRange = CELIX_MQTT_BROKER_INFO_SERVICE_RANGE;
//    opts.filter.filter = "(" CELIX_MQTT_BROKER_ADDRESS "=127.0.0.2)";
//    opts.waitTimeoutInSeconds = 30;
//    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
//    EXPECT_TRUE(found);
//    system("sudo ifconfig lo:2 127.0.0.2 down");
//    celix_earpmp_destroy(parser);
//    unsetenv(CELIX_EARPM_BROKER_PROFILE);
//}
