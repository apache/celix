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
add_executable(integration_test_rsa_json_rpc
        src/RsaJsonRpcIntegrationTestSuite.cc
)
celix_deprecated_utils_headers(integration_test_rsa_json_rpc)


target_link_libraries(integration_test_rsa_json_rpc PRIVATE
    Celix::c_rsa_spi
    Celix::framework
    GTest::gtest
    GTest::gtest_main
    )

celix_get_bundle_file(Celix::rsa_json_rpc RSA_JSON_RPC_BUNDLE_FILE)
target_compile_definitions(integration_test_rsa_json_rpc PRIVATE -DRSA_JSON_RPC_BUNDLE="${RSA_JSON_RPC_BUNDLE_FILE}")


add_test(NAME run_integration_test_rsa_json_rpc COMMAND integration_test_rsa_json_rpc)
setup_target_for_coverage(integration_test_rsa_json_rpc SCAN_DIR ..)

if (EI_TESTS)
    ####unit test
    add_executable(unit_test_rsa_json_rpc
            src/RsaJsonRpcUnitTestSuite.cc
            src/RsaJsonRpcActivatorUnitTestSuite.cc
            )

    celix_deprecated_utils_headers(unit_test_rsa_json_rpc)

    target_link_libraries(unit_test_rsa_json_rpc PRIVATE
            rsa_json_rpc_cut
            Celix::c_rsa_spi
            rsa_common_cut
            Celix::framework
            Celix::bundle_ei
            Celix::malloc_ei
            Celix::threads_ei
            Celix::bundle_ctx_ei
            Celix::version_ei
            Celix::dfi_ei
            Celix::log_helper_ei
            Celix::properties_ei
            Celix::long_hash_map_ei
            GTest::gtest
            GTest::gtest_main
            )

    target_compile_definitions(unit_test_rsa_json_rpc PRIVATE -DRESOURCES_DIR="${CMAKE_CURRENT_LIST_DIR}/resources")

    add_test(NAME run_unit_test_rsa_json_rpc COMMAND unit_test_rsa_json_rpc)
    setup_target_for_coverage(unit_test_rsa_json_rpc SCAN_DIR ..)
endif ()