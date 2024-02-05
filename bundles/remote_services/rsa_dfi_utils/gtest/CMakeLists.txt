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

add_celix_bundle(rsa_dfi_utils_test_descriptor NO_ACTIVATOR VERSION 1.0.0)
celix_bundle_files(rsa_dfi_utils_test_descriptor
        ${CMAKE_CURRENT_SOURCE_DIR}/descriptors/rsa_dfi_utils_test.descriptor
        DESTINATION "META-INF/descriptors"
)

add_executable(test_rsa_dfi_utils
        src/DfiUtilsTestSuite.cc
)
celix_deprecated_utils_headers(test_rsa_dfi_utils)

target_link_libraries(test_rsa_dfi_utils PRIVATE Celix::rsa_dfi_utils Celix::framework GTest::gtest GTest::gtest_main)

celix_get_bundle_file(rsa_dfi_utils_test_descriptor DESCRIPTOR_BUNDLE_FILE)
target_compile_definitions(test_rsa_dfi_utils PRIVATE -DDESCRIPTOR_BUNDLE="${DESCRIPTOR_BUNDLE_FILE}")
target_compile_definitions(test_rsa_dfi_utils PRIVATE -DDESCRIPTOR_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/descriptors")

add_test(NAME run_test_rsa_dfi_utils COMMAND test_rsa_dfi_utils)
setup_target_for_coverage(test_rsa_dfi_utils SCAN_DIR ..)
