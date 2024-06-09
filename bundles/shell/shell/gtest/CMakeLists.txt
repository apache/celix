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

add_executable(test_shell
        src/ShellTestSuite.cc
)

add_celix_bundle(celix_shell_empty_resource_test_bundle NO_ACTIVATOR VERSION 0.0.0)

target_link_libraries(test_shell PRIVATE Celix::framework Celix::shell_api GTest::gtest GTest::gtest_main)
celix_target_bundle_set_definition(test_shell NAME TEST_BUNDLES Celix::shell celix_shell_empty_resource_test_bundle)
target_compile_options(test_shell PRIVATE -Wno-deprecated-declarations)

add_test(NAME test_shell COMMAND test_shell)
setup_target_for_coverage(test_shell SCAN_DIR ..)


if (CELIX_CXX14)
    add_executable(test_cxx_shell src/ShellTestSuite.cc)

    target_link_libraries(test_cxx_shell PRIVATE Celix::framework Celix::shell_api GTest::gtest GTest::gtest_main)
    celix_target_bundle_set_definition(test_cxx_shell NAME TEST_BUNDLES Celix::ShellCxx celix_shell_empty_resource_test_bundle)
    target_compile_definitions(test_cxx_shell PRIVATE -DCXX_SHELL)

    add_test(NAME test_cxx_shell COMMAND test_cxx_shell)
    setup_target_for_coverage(test_cxx_shell SCAN_DIR ..)
endif()