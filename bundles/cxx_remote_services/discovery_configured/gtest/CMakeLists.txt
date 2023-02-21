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

add_executable(test_cxx_rsa_configured_discovery
        src/RsaConfiguredDiscoveryTestSuite.cc
)

target_link_libraries(test_cxx_rsa_configured_discovery PRIVATE Celix::framework GTest::gtest GTest::gtest_main Celix::rsa_spi RsaConfiguredDiscovery_api)

add_celix_bundle_dependencies(test_cxx_rsa_configured_discovery RsaConfiguredDiscovery)
celix_get_bundle_file(RsaConfiguredDiscovery RSA_CONFIGURED_DISCOVERY_BUNDLE_LOCATION)
target_compile_definitions(test_cxx_rsa_configured_discovery PRIVATE RSA_CONFIGURED_DISCOVERY_BUNDLE_LOCATION="${RSA_CONFIGURED_DISCOVERY_BUNDLE_LOCATION}")
target_compile_definitions(test_cxx_rsa_configured_discovery PRIVATE RSA_CONFIGURED_DISCOVERY_DISCOVERY_FILE="${CMAKE_CURRENT_SOURCE_DIR}/resources/endpoint_discovery.json")

add_test(NAME test_cxx_rsa_configured_discovery COMMAND test_cxx_rsa_configured_discovery)
setup_target_for_coverage(test_cxx_rsa_configured_discovery SCAN_DIR ..)