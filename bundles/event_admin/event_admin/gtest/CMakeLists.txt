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

add_executable(integration_test_event_admin
        src/CelixEventAdminIntegrationTestSuite.cc
        )

target_link_libraries(integration_test_event_admin PRIVATE
        Celix::event_admin_api
        Celix::framework
        GTest::gtest
        GTest::gtest_main
        )

celix_get_bundle_file(Celix::event_admin EVENT_ADMIN_BUNDLE_FILE)
target_compile_definitions(integration_test_event_admin PRIVATE -DEVENT_ADMIN_BUNDLE="${EVENT_ADMIN_BUNDLE_FILE}")


add_test(NAME run_integration_test_event_admin COMMAND integration_test_event_admin)
setup_target_for_coverage(integration_test_event_admin SCAN_DIR ..)

####unit test
add_executable(unit_test_event_admin
        src/CelixEventTestSuite.cc
        src/CelixEventAdminTestSuite.cc
        src/CelixEventAdapterTestSuite.cc
        src/CelixEventAdminActivatorTestSuite.cc
)

target_link_libraries(unit_test_event_admin PRIVATE
        event_admin_cut
        Celix::framework
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(unit_test_event_admin PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

add_celix_bundle(test_bundle NO_ACTIVATOR VERSION 1.0.0)
celix_get_bundle_file(test_bundle TEST_BUNDLE_FILE)
target_compile_definitions(unit_test_event_admin PRIVATE -DTEST_BUNDLE="${TEST_BUNDLE_FILE}")

add_test(NAME run_unit_test_event_admin COMMAND unit_test_event_admin)
setup_target_for_coverage(unit_test_event_admin SCAN_DIR ..)

if (EI_TESTS)
    ####unit test with error injection
    add_executable(unit_test_event_admin_with_error_injection
            src/CelixEventErrorInjectionTestSuite.cc
            src/CelixEventAdminErrorInjectionTestSuite.cc
            src/CelixEventAdapterErrorInjectionTestSuite.cc
            #src/CelixEventAdminActivatorErrorInjectionTestSuite.cc
            )

    target_link_libraries(unit_test_event_admin_with_error_injection PRIVATE
            event_admin_cut
            Celix::framework
            Celix::threads_ei
            Celix::bundle_ctx_ei
            Celix::string_hash_map_ei
            Celix::long_hash_map_ei
            Celix::array_list_ei
            Celix::properties_ei
            Celix::utils_ei
            #Celix::dm_component_ei
            Celix::log_helper_ei
            Celix::malloc_ei
            GTest::gtest
            GTest::gtest_main
            )

    target_include_directories(unit_test_event_admin_with_error_injection PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

    target_compile_definitions(unit_test_event_admin_with_error_injection PRIVATE -DTEST_BUNDLE="${TEST_BUNDLE_FILE}")

    add_test(NAME run_unit_test_event_admin_with_error_injection COMMAND unit_test_event_admin_with_error_injection)
    setup_target_for_coverage(unit_test_event_admin_with_error_injection SCAN_DIR ..)
endif ()