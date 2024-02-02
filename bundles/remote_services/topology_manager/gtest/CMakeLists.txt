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


add_executable(unit_test_tm
        src/TopologyManagerTestSuite.cc
)

celix_deprecated_utils_headers(unit_test_tm)
celix_deprecated_framework_headers(unit_test_tm)

target_link_libraries(unit_test_tm PRIVATE
        rsa_topology_manager_cut
        Celix::c_rsa_spi
        Celix::framework
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME run_unit_test_tm COMMAND unit_test_tm)
setup_target_for_coverage(unit_test_tm SCAN_DIR ..)

if (EI_TESTS)
    ####unit test with error injection
    add_executable(unit_test_tm_with_error_injection
            src/TopologyManagerErrorInjectionTestSuite.cc
    )

    celix_deprecated_utils_headers(unit_test_tm_with_error_injection)
    celix_deprecated_framework_headers(unit_test_tm_with_error_injection)

    target_link_libraries(unit_test_tm_with_error_injection PRIVATE
            rsa_topology_manager_cut
            Celix::c_rsa_spi
            Celix::framework
            Celix::threads_ei
#            Celix::bundle_ctx_ei
#            Celix::string_hash_map_ei
            Celix::long_hash_map_ei
            Celix::array_list_ei
            Celix::properties_ei
            Celix::utils_ei
            Celix::malloc_ei
            GTest::gtest
            GTest::gtest_main
    )

    add_test(NAME run_unit_test_tm_with_error_injection COMMAND unit_test_tm_with_error_injection)
    setup_target_for_coverage(unit_test_tm_with_error_injection SCAN_DIR ..)
endif ()