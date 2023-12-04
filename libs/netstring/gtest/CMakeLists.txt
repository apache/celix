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

add_executable(test_celix_netstring src/CelixNetstringTestSuite.cc)

target_include_directories(test_celix_netstring PRIVATE src)

target_link_libraries(test_celix_netstring PRIVATE netstring_cut GTest::gtest GTest::gtest_main)

add_test(NAME run_test_celix_netstring COMMAND test_celix_netstring)
setup_target_for_coverage(test_celix_netstring SCAN_DIR ..)

if (EI_TESTS)

    add_executable(test_celix_netstring_with_ei  src/CelixNetstringErrorInjectionTestSuite.cc)

    target_include_directories(test_celix_netstring_with_ei PRIVATE src)

    target_link_libraries(test_celix_netstring_with_ei
            PRIVATE
            netstring_cut
            Celix::stdio_ei
            Celix::malloc_ei
            GTest::gtest
            GTest::gtest_main)

    add_test(NAME run_test_celix_netstring_with_ei COMMAND test_celix_netstring_with_ei)
    setup_target_for_coverage(test_celix_netstring_with_ei SCAN_DIR ..)
    
endif ()