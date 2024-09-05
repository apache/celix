# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

####integration test
find_package(mosquitto REQUIRED)
add_executable(integration_test_event_admin_remote_provider_mqtt
        src/CelixEarpmIntegrationTestSuite.cc
        )

target_link_libraries(integration_test_event_admin_remote_provider_mqtt PRIVATE
        Celix::event_admin_api
        Celix::framework
        mosquitto::libmosquitto
        GTest::gtest
        GTest::gtest_main
        )

target_include_directories(integration_test_event_admin_remote_provider_mqtt PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)

celix_target_bundle_set_definition(integration_test_event_admin_remote_provider_mqtt NAME INTEGRATED_BUNDLES
        Celix::event_admin
        Celix::event_admin_remote_provider_mqtt
)

target_compile_definitions(integration_test_event_admin_remote_provider_mqtt PRIVATE -DMOSQUITTO_CONF="${CMAKE_CURRENT_LIST_DIR}/res/mosquitto.conf")

add_test(NAME run_integration_test_event_admin_remote_provider_mqtt COMMAND integration_test_event_admin_remote_provider_mqtt)
setup_target_for_coverage(integration_test_event_admin_remote_provider_mqtt SCAN_DIR ..)

####unit test
add_executable(unit_test_event_admin_remote_provider_mqtt
        src/CelixEarpmImplTestSuite.cc
        src/CelixEarpmEventDelivererTestSuite.cc
        src/CelixEarpmClientTestSuite.cc
        src/CelixEarpmBrokerDiscoveryTestSuite.cc
        src/CelixEarpmActivatorTestSuite.cc
)

target_link_libraries(unit_test_event_admin_remote_provider_mqtt PRIVATE
        event_admin_remote_provider_mqtt_cut
        Celix::framework
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(unit_test_event_admin_remote_provider_mqtt PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

target_compile_definitions(unit_test_event_admin_remote_provider_mqtt PRIVATE -DMOSQUITTO_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/res/")

add_test(NAME run_unit_test_event_admin_remote_provider_mqtt COMMAND unit_test_event_admin_remote_provider_mqtt)
setup_target_for_coverage(unit_test_event_admin_remote_provider_mqtt SCAN_DIR ..)

if (EI_TESTS)
    ####unit test with error injection
    add_executable(unit_test_event_admin_remote_provider_mqtt_with_error_injection
            src/CelixEarpmActivatorErrorInjectionTestSuite.cc
            src/CelixEarpmBrokerDiscoveryErrorInjectionTestSuite.cc
            src/CelixEarpmEventDelivererErrorInjectionTestSuite.cc
            src/CelixEarpmClientErrorInjectionTestSuite.cc
            src/CelixEarpmImplErrorInjectionTestSuite.cc
            )

    target_link_libraries(unit_test_event_admin_remote_provider_mqtt_with_error_injection PRIVATE
            event_admin_remote_provider_mqtt_cut
            Celix::framework
            Celix::threads_ei
            Celix::bundle_ctx_ei
            Celix::string_hash_map_ei
            Celix::long_hash_map_ei
            Celix::array_list_ei
            Celix::properties_ei
            Celix::utils_ei
            Celix::dm_component_ei
            Celix::log_helper_ei
            Celix::malloc_ei
            Celix::filter_ei
            Celix::mosquitto_ei
            Celix::asprintf_ei
            Celix::jansson_ei
            GTest::gtest
            GTest::gtest_main
            )

    target_include_directories(unit_test_event_admin_remote_provider_mqtt_with_error_injection PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

    target_compile_definitions(unit_test_event_admin_remote_provider_mqtt_with_error_injection PRIVATE -DMOSQUITTO_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/res/")

    add_test(NAME run_unit_test_event_admin_remote_provider_mqtt_with_error_injection COMMAND unit_test_event_admin_remote_provider_mqtt_with_error_injection)
    setup_target_for_coverage(unit_test_event_admin_remote_provider_mqtt_with_error_injection SCAN_DIR ..)
endif ()