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


add_executable(test_log_admin
        src/LogAdminTestSuite.cc
)
target_link_libraries(test_log_admin PRIVATE Celix::framework Celix::log_service_api Celix::shell_api GTest::gtest GTest::gtest_main)

add_celix_bundle_dependencies(test_log_admin Celix::log_admin)
target_compile_definitions(test_log_admin PRIVATE -DLOG_ADMIN_BUNDLE=\"$<TARGET_PROPERTY:log_admin,BUNDLE_FILE>\")

add_test(NAME test_log_admin COMMAND test_log_admin)
setup_target_for_coverage(test_log_admin SCAN_DIR ..)


if (EI_TESTS)
    add_executable(test_log_admin_with_ei src/LogAdminWithErrorInjectionTestSuite.cc)
    target_link_libraries(test_log_admin_with_ei PRIVATE
            log_admin_cut
            Celix::framework Celix::log_service_api Celix::shell_api
            Celix::malloc_ei Celix::asprintf_ei Celix::properties_ei Celix::string_hash_map_ei
            GTest::gtest GTest::gtest_main
    )
    target_include_directories(test_log_admin_with_ei PRIVATE ../src)
    add_test(NAME test_log_admin_with_ei COMMAND test_log_admin_with_ei)
    setup_target_for_coverage(test_log_admin_with_ei SCAN_DIR ..)
endif ()

