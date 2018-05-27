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
    disc_mock
    bundle
)

add_subdirectory(bundle)
add_subdirectory(disc_mock)

#SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
#SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils" "${PROJECT_BINARY_DIR}/dfi")

add_executable(test_tm_scoped
    run_tests.cpp
    tms_tests.cpp
)
target_include_directories(test_tm_scoped PRIVATE ../src ../include)
target_link_libraries(test_tm_scoped PRIVATE
        Celix::framework
        ${CPPUTEST_LIBRARY}
        ${JANSSON_LIBRARY}
        calculator_api
        Celix::rsa_common
)

add_dependencies(test_tm_scoped rsa_dfi_bundle rsa_topology_manager_bundle)

file(GENERATE 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.properties"
    CONTENT "
cosgi.auto.start.1=$<TARGET_PROPERTY:Celix::rsa_dfi,BUNDLE_FILE> $<TARGET_PROPERTY:calculator,BUNDLE_FILE> $<TARGET_PROPERTY:Celix::rsa_topology_manager,BUNDLE_FILE> $<TARGET_PROPERTY:topology_manager_disc_mock_bundle,BUNDLE_FILE>
LOGHELPER_ENABLE_STDOUT_FALLBACK=true
org.osgi.framework.storage.clean=onFirstInit
")

file(GENERATE 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config_import.properties"
    CONTENT "
cosgi.auto.start.1=$<TARGET_PROPERTY:Celix::rsa_dfi,BUNDLE_FILE> $<TARGET_PROPERTY:calculator,BUNDLE_FILE> $<TARGET_PROPERTY:Celix::rsa_topology_manager,BUNDLE_FILE> $<TARGET_PROPERTY:topology_manager_test_bundle,BUNDLE_FILE>
LOGHELPER_ENABLE_STDOUT_FALLBACK=true
org.osgi.framework.storage.clean=onFirstInit
")

configure_file("scope.json" "scope.json")
configure_file("scope2.json" "scope2.json")
configure_file("scope3.json" "scope3.json")
configure_file("scope4.json" "scope4.json")


add_test(NAME run_test_tm_scoped COMMAND test_tm_scoped)
SETUP_TARGET_FOR_COVERAGE(test_tm_scoped_cov test_tm_scoped ${CMAKE_BINARY_DIR}/coverage/test_tm_scoped/test_tm_scoped)

