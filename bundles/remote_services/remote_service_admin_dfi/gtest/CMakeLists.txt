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

find_package(libffi REQUIRED)
find_package(LibXml2 REQUIRED)

add_celix_bundle(rsa_dfi_tst_bundle
    VERSION 0.0.1
    SOURCES
        src/tst_activator.c
)
celix_deprecated_utils_headers(rsa_dfi_tst_bundle)
celix_deprecated_framework_headers(rsa_dfi_tst_bundle)

#add calculator
get_target_property(DESCR calculator_api INTERFACE_DESCRIPTOR)
celix_bundle_files(rsa_dfi_tst_bundle ${DESCR} DESTINATION .)

#add remote example
get_target_property(DESCR remote_example_api INTERFACE_DESCRIPTOR)
celix_bundle_files(rsa_dfi_tst_bundle ${DESCR} DESTINATION .)

target_link_libraries(rsa_dfi_tst_bundle PRIVATE calculator_api remote_example_api)
target_include_directories(rsa_dfi_tst_bundle PRIVATE src)

add_executable(test_rsa_dfi
    src/rsa_tests.cc
    src/rsa_client_server_tests.cc
)
target_include_directories(test_rsa_dfi PRIVATE src)
celix_deprecated_utils_headers(test_rsa_dfi)
celix_deprecated_framework_headers(test_rsa_dfi)

target_link_libraries(test_rsa_dfi PRIVATE
        civetweb::civetweb
        Celix::framework
        Celix::rsa_common
        calculator_api
        GTest::gtest
        GTest::gtest_main
        libffi::libffi
)

get_property(rsa_bundle_file TARGET rsa_dfi PROPERTY BUNDLE_FILE)
get_property(calc_bundle_file TARGET calculator PROPERTY BUNDLE_FILE)
get_property(discovery_bundle_file TARGET rsa_discovery PROPERTY BUNDLE_FILE)
get_property(topology_manager_bundle_file TARGET Celix::rsa_topology_manager PROPERTY BUNDLE_FILE)
get_property(tst_bundle_file TARGET rsa_dfi_tst_bundle PROPERTY BUNDLE_FILE)
get_property(remote_example_bundle_file TARGET remote_example_service PROPERTY BUNDLE_FILE)

configure_file(config.properties.in config.properties)
configure_file(client.properties.in client.properties)
configure_file(server.properties.in server.properties)
#add exception service interface descriptor
configure_file(exception_test_service.descriptor exception_test_service.descriptor)

target_compile_options(test_rsa_dfi PRIVATE -DRSA_DFI_BUNDLE_FILE="${rsa_bundle_file}")
target_compile_options(test_rsa_dfi PRIVATE -DCALC_BUNDLE_FILE="${calc_bundle_file}")
target_compile_options(test_rsa_dfi PRIVATE -DTST_BUNDLE_FILE="${tst_bundle_file}")

add_celix_bundle_dependencies(test_rsa_dfi
        rsa_dfi #note depend on the target creating the bundle zip not the lib target
        calculator
        remote_example_service
        rsa_discovery
        Celix::rsa_topology_manager
        rsa_dfi_tst_bundle
)

add_test(NAME run_test_rsa_dfi COMMAND test_rsa_dfi)
setup_target_for_coverage(test_rsa_dfi SCAN_DIR ..)

