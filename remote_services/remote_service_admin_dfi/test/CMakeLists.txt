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

find_package(CppUTest REQUIRED)
include_directories(${CPPUTEST_INCLUDE_DIR})

add_celix_bundle(rsa_dfi_tst_bundle
    VERSION 0.0.1
    SOURCES
        src/tst_activator.c
)
get_target_property(DESCR calculator_api INTERFACE_CALCULATOR_DESCRIPTOR)
celix_bundle_files(rsa_dfi_tst_bundle ${DESCR} DESTINATION .)
target_link_libraries(rsa_dfi_tst_bundle PRIVATE ${CPPUTEST_LIBRARY} calculator_api)
target_include_directories(rsa_dfi_tst_bundle PRIVATE src)

add_executable(test_rsa_dfi
    src/run_tests.cpp
    src/rsa_tests.cpp
    src/rsa_client_server_tests.cpp
)
target_include_directories(test_rsa_dfi PRIVATE src)
target_link_libraries(test_rsa_dfi PRIVATE ${CURL_LIBRARIES} ${CPPUTEST_LIBRARY}
        Celix::framework
        Celix::rsa_common
        calculator_api)

get_property(rsa_bundle_file TARGET rsa_dfi PROPERTY BUNDLE_FILE)
get_property(calc_bundle_file TARGET calculator PROPERTY BUNDLE_FILE)
get_property(calculator_shell_bundle_file TARGET calculator_shell PROPERTY BUNDLE_FILE)
get_property(discovery_configured_bundle_file TARGET rsa_discovery_configured PROPERTY BUNDLE_FILE)
get_property(topology_manager_bundle_file TARGET Celix::rsa_topology_manager PROPERTY BUNDLE_FILE)
get_property(tst_bundle_file TARGET rsa_dfi_tst_bundle PROPERTY BUNDLE_FILE)

configure_file(config.properties.in config.properties)
configure_file(client.properties.in client.properties)
configure_file(server.properties.in server.properties)

add_dependencies(test_rsa_dfi
        rsa_dfi_bundle #note depend on the target creating the bundle zip not the lib target
        calculator_bundle
)

add_test(NAME run_test_rsa_dfi COMMAND test_rsa_dfi)
SETUP_TARGET_FOR_COVERAGE(test_rsa_dfi_cov test_rsa_dfi ${CMAKE_BINARY_DIR}/coverage/test_rsa_dfi/test_rsa_dfi)

