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
if (LINKER_WRAP_SUPPORTED)
    ####unit test
    add_executable(unit_test_rsa_common
            src/EndpointDescriptionUnitTestSuite.cc
            )

    target_link_libraries(unit_test_rsa_common PRIVATE
            rsa_common_cut
            Celix::framework
            Celix::utils
            Celix::malloc_ei
            Celix::utils_ei
            Celix::properties_ei
            GTest::gtest
            GTest::gtest_main
            )

    celix_deprecated_utils_headers(unit_test_rsa_common)

    add_test(NAME run_unit_test_rsa_common COMMAND unit_test_rsa_common)
    setup_target_for_coverage(unit_test_rsa_common SCAN_DIR ..)
endif ()