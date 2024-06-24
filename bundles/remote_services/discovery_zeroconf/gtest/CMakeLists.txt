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

find_package(DNSSD REQUIRED)

add_executable(integration_test_discovery_zeroconf
        src/DiscoveryZeroconfIntegrationTestSuite.cc
        )

celix_deprecated_utils_headers(integration_test_discovery_zeroconf)

target_link_libraries(integration_test_discovery_zeroconf PRIVATE
        Celix::c_rsa_spi
        Celix::framework
        Celix::utils
        GTest::gtest
        GTest::gtest_main
        )

celix_get_bundle_file(Celix::rsa_discovery_zeroconf DISCOVERY_ZEROCONF_BUNDLE_FILE)
target_compile_definitions(integration_test_discovery_zeroconf PRIVATE -DDISCOVERY_ZEROCONF_BUNDLE="${DISCOVERY_ZEROCONF_BUNDLE_FILE}")


add_test(NAME run_integration_test_discovery_zeroconf COMMAND integration_test_discovery_zeroconf)
setup_target_for_coverage(integration_test_discovery_zeroconf SCAN_DIR ..)

if (EI_TESTS)
    ####unit test
    add_executable(unit_test_discovery_zeroconf
            src/DiscoveryZeroconfAnnouncerTestSuite.cc
            src/DiscoveryZeroconfWatcherTestSuite.cc
            src/DiscoveryZeroconfActivatorTestSuite.cc
            )

    celix_deprecated_utils_headers(unit_test_discovery_zeroconf)
    celix_deprecated_framework_headers(unit_test_discovery_zeroconf)

    target_link_libraries(unit_test_discovery_zeroconf PRIVATE
            rsa_discovery_zeroconf_cut
            rsa_common_cut
            Celix::c_rsa_spi
            Celix::framework
            Celix::threads_ei
            Celix::eventfd_ei
            Celix::bundle_ctx_ei
            Celix::string_hash_map_ei
            Celix::long_hash_map_ei
            Celix::array_list_ei
            Celix::properties_ei
            Celix::utils_ei
            Celix::dm_component_ei
            Celix::bundle_ctx_ei
            Celix::log_helper_ei
            Celix::mdnsresponder_ei
            Celix::malloc_ei
            GTest::gtest
            GTest::gtest_main
            )

    target_compile_definitions(unit_test_discovery_zeroconf PRIVATE -DMDNSD="${DNSSD_INCLUDE_DIR}/../bin/mdnsd")


    add_test(NAME run_unit_test_discovery_zeroconf COMMAND $<TARGET_FILE:unit_test_discovery_zeroconf>)
    setup_target_for_coverage(unit_test_discovery_zeroconf SCAN_DIR ..)
endif ()