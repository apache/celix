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

set(CMAKE_CXX_STANDARD 17)

add_executable(test_rcm
    src/RequirementCapabilityModelTestSuite.cc
)

target_link_libraries(test_rcm PRIVATE Celix::rcm GTest::gtest GTest::gtest_main)
add_test(NAME test_rcm COMMAND test_rcm)
setup_target_for_coverage(test_rcm SCAN_DIR ..)

if (LINKER_WRAP_SUPPORTED)
    add_executable(test_rcm_with_error_injection
        src/RequirementCapabilityModelWithErrorInjectionTestSuite.cc
    )
    target_link_libraries(test_rcm_with_error_injection PRIVATE Celix::rcm GTest::gtest GTest::gtest_main)
    target_link_libraries(test_rcm_with_error_injection PRIVATE Celix::malloc_ei Celix::utils_ei Celix::array_list_ei)
    target_include_directories(test_rcm_with_error_injection PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src) #private headers
    add_test(NAME test_rcm_with_error_injection COMMAND test_rcm_with_error_injection)
    setup_target_for_coverage(test_rcm_with_error_injection SCAN_DIR ..)
endif ()

