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
add_executable(integration_test_rsa_shm
        src/RsaShmIntegrationTestSuite.cc
)

celix_deprecated_utils_headers(integration_test_rsa_shm)
celix_deprecated_framework_headers(integration_test_rsa_shm)

target_link_libraries(integration_test_rsa_shm PRIVATE
    Celix::c_rsa_spi
    Celix::framework
    GTest::gtest
    GTest::gtest_main
    )

celix_get_bundle_file(Celix::rsa_shm RSA_SHM_BUNDLE_FILE)
target_compile_definitions(integration_test_rsa_shm PRIVATE -DRSA_SHM_BUNDLE="${RSA_SHM_BUNDLE_FILE}")
celix_get_bundle_file(Celix::rsa_json_rpc RSA_JSON_RPC_BUNDLE_FILE)
target_compile_definitions(integration_test_rsa_shm PRIVATE -DRSA_JSON_RPC_BUNDLE="${RSA_JSON_RPC_BUNDLE_FILE}")

add_celix_bundle_dependencies(integration_test_rsa_shm Celix::rsa_shm)

add_test(NAME run_integration_test_rsa_shm COMMAND integration_test_rsa_shm)
setup_target_for_coverage(integration_test_rsa_shm SCAN_DIR ..)

if (EI_TESTS AND BUILD_RSA_JSON_RPC)
    ####unit test
    add_executable(unit_test_rsa_shm
            src/RsaShmImplUnitTestSuite.cc
            src/RsaShmExportRegistrationUnitTestSuite.cc
            src/RsaShmImportRegistrationUnitTestSuite.cc
            src/RsaShmClientServerUnitTestSuite.cc
            src/RsaShmActivatorUnitTestSuite.cc
            src/thpool_ei.cc
            src/shm_pool_ei.cc
            )

    target_link_libraries(unit_test_rsa_shm PRIVATE
            rsa_shm_cut
            Celix::c_rsa_spi
            rsa_common_cut
            Celix::log_helper
            Celix::framework
            Celix::threads_ei
            Celix::bundle_ctx_ei
            Celix::malloc_ei
            Celix::asprintf_ei
            Celix::utils_ei
            Celix::socket_ei
            Celix::stdio_ei
            Celix::pthread_ei
            Celix::log_helper_ei
            Celix::properties_ei
            Celix::long_hash_map_ei
            Celix::string_hash_map_ei
            Celix::dm_component_ei
            Celix::asprintf_ei
            GTest::gtest
            GTest::gtest_main
            )

    target_link_options(unit_test_rsa_shm PRIVATE
            LINKER:--wrap,thpool_init
            LINKER:--wrap,thpool_add_work
            LINKER:--wrap,shmPool_malloc
            )

    target_compile_definitions(unit_test_rsa_shm PRIVATE -DRESOURCES_DIR="${CMAKE_CURRENT_LIST_DIR}/resources")
    celix_get_bundle_file(Celix::rsa_json_rpc RSA_JSON_RPC_BUNDLE_FILE)
    target_compile_definitions(unit_test_rsa_shm PRIVATE -DRSA_JSON_RPC_BUNDLE="${RSA_JSON_RPC_BUNDLE_FILE}")

    add_celix_bundle_dependencies(unit_test_rsa_shm Celix::rsa_json_rpc)

    celix_deprecated_utils_headers(unit_test_rsa_shm)
    celix_deprecated_framework_headers(unit_test_rsa_shm)

    add_test(NAME run_unit_test_rsa_shm COMMAND unit_test_rsa_shm)
    setup_target_for_coverage(unit_test_rsa_shm SCAN_DIR ..)
endif ()