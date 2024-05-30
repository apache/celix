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

find_package(CURL REQUIRED)
find_package(civetweb REQUIRED)
find_package(libffi REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include
    disc_mock
    bundle
)

add_subdirectory(bundle)
add_subdirectory(disc_mock)

add_executable(test_tm_scoped
        main.cc
        tms_tests.cpp
)
celix_deprecated_utils_headers(test_tm_scoped)
celix_deprecated_framework_headers(test_tm_scoped)
target_include_directories(test_tm_scoped PRIVATE ../src ../include)
target_link_libraries(test_tm_scoped PRIVATE
        Celix::framework
        GTest::gtest
        jansson::jansson
        calculator_api
        Celix::rsa_common
        CURL::libcurl
        civetweb::civetweb
        libffi::libffi
)

add_celix_bundle_dependencies(test_tm_scoped Celix::rsa_dfi Celix::rsa_topology_manager calculator topology_manager_disc_mock_bundle topology_manager_test_bundle)

file(GENERATE 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.properties"
    CONTENT "{
    \"CELIX_AUTO_START_1\":\"$<TARGET_PROPERTY:Celix::rsa_dfi,BUNDLE_FILE> $<TARGET_PROPERTY:calculator,BUNDLE_FILE> $<TARGET_PROPERTY:Celix::rsa_topology_manager,BUNDLE_FILE> $<TARGET_PROPERTY:topology_manager_disc_mock_bundle,BUNDLE_FILE>\",
    \"LOGHELPER_ENABLE_STDOUT_FALLBACK\":true,
    \"CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE\":true
}")

file(GENERATE 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config_import.properties"
    CONTENT "{
    \"CELIX_AUTO_START_1\":\"$<TARGET_PROPERTY:Celix::rsa_dfi,BUNDLE_FILE> $<TARGET_PROPERTY:calculator,BUNDLE_FILE> $<TARGET_PROPERTY:Celix::rsa_topology_manager,BUNDLE_FILE> $<TARGET_PROPERTY:topology_manager_test_bundle,BUNDLE_FILE>\",
    \"LOGHELPER_ENABLE_STDOUT_FALLBACK\":true,
    \"CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE\":true
}")

configure_file("scope.json" "scope.json")
configure_file("scope2.json" "scope2.json")
configure_file("scope3.json" "scope3.json")
configure_file("scope4.json" "scope4.json")


add_test(NAME run_test_tm_scoped COMMAND test_tm_scoped)
setup_target_for_coverage(test_tm_scoped SCAN_DIR ..)

