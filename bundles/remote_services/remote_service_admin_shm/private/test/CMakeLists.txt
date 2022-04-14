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

include_directories(
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include
    bundle
)


SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils")

add_executable(test_rsa_shm
    run_tests.cpp
    rsa_client_server_tests.cpp

    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c
)
target_link_libraries(test_rsa_shm celix_framework celix_utils CURL::libcurl CppUTest::CppUTest)

get_property(rsa_bundle_file TARGET remote_service_admin_shm PROPERTY BUNDLE_FILE)
get_property(calc_bundle_file TARGET calculator PROPERTY BUNDLE_FILE)
get_property(calculator_shell_bundle_file TARGET calculator_shell PROPERTY BUNDLE_FILE)
get_property(discovery_shm_bundle_file TARGET discovery_shm PROPERTY BUNDLE_FILE)
get_property(topology_manager_bundle_file TARGET topology_manager PROPERTY BUNDLE_FILE)
get_property(calc_proxy_bundle_file TARGET org.apache.celix.calc.api.Calculator_proxy PROPERTY BUNDLE_FILE)
get_property(calc_endpoint_bundle_file TARGET  org.apache.celix.calc.api.Calculator_endpoint PROPERTY BUNDLE_FILE)

get_filename_component(client_endpoints ${calc_proxy_bundle_file} PATH)
get_filename_component(server_endpoints ${calc_endpoint_bundle_file} PATH)

configure_file(client.properties.in client.properties)
configure_file(server.properties.in server.properties)

add_dependencies(test_rsa_shm remote_service_admin_shm calculator org.apache.celix.calc.api.Calculator_proxy org.apache.celix.calc.api.Calculator_endpoint)
 
#add_test(NAME run_test_rsa_shm COMMAND test_rsa_shm)
#setup_target_for_coverage(test_rsa_shm_cov test_rsa_shm ${CMAKE_BINARY_DIR}/coverage/test_rsa_shm/test_rsa_shm)

