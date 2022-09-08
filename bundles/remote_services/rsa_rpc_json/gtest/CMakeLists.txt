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

find_package(libuuid REQUIRED)
find_package(jansson REQUIRED)

add_executable(test_rsa_json_rpc
        src/RsaJsonRpcActivatorTestSuite.cc
        src/RsaJsonRpcTestSuite.cc
)


target_link_libraries(test_rsa_json_rpc PRIVATE
    Celix::c_rsa_spi
    calculator_api
    Celix::rsa_common
    libuuid::libuuid
    jansson::jansson
    Celix::framework
    Celix::shell_api
    GTest::gtest
    GTest::gtest_main
    )

celix_get_bundle_file(Celix::rsa_json_rpc RSA_JSON_RPC_BUNDLE_FILE)
target_compile_definitions(test_rsa_json_rpc PRIVATE -DRSA_JSON_RPC_BUNDLE="${RSA_JSON_RPC_BUNDLE_FILE}")
celix_get_bundle_file(calculator_shell CONSUMER_BUNDLE_FILE)
target_compile_definitions(test_rsa_json_rpc PRIVATE -DCONSUMER_BUNDLE="${CONSUMER_BUNDLE_FILE}")
celix_get_bundle_file(calculator PROVIDER_BUNDLE_FILE)
target_compile_definitions(test_rsa_json_rpc PRIVATE -DPROVIDER_BUNDLE="${PROVIDER_BUNDLE_FILE}")

add_test(NAME run_test_rsa_json_rpc COMMAND test_rsa_json_rpc)
setup_target_for_coverage(test_rsa_json_rpc SCAN_DIR ..)
