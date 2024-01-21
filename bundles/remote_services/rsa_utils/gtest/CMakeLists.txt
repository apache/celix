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

add_executable(test_rsa_utils
        src/RsaUtilsTestSuite.cc
)

target_link_libraries(test_rsa_utils PRIVATE rsa_utils GTest::gtest GTest::gtest_main)
target_include_directories(test_utils PRIVATE ../src) #for version_private (needs refactoring of test)
add_test(NAME test_rsa_utils COMMAND test_rsa_utils)
setup_target_for_coverage(test_rsa_utils SCAN_DIR ..)

if (EI_TESTS)
    add_executable(test_rsa_utils_ei
            src/RsaUtilsErrorInjectionTestSuite.cc
    )
    target_link_libraries(test_rsa_utils_ei PRIVATE rsa_utils GTest::gtest GTest::gtest_main)
    target_link_libraries(test_rsa_utils_ei PRIVATE properties_ei)
    add_test(NAME test_rsa_utils_ei COMMAND test_rsa_utils_ei)
    setup_target_for_coverage(test_rsa_utils_ei SCAN_DIR ..)
endif ()
