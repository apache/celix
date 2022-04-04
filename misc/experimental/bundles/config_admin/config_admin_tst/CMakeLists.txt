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

include_directories(
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/config_admin/service/public/include
    example_test/private/include
    example_test2/private/include
)

add_subdirectory(example_test)
add_subdirectory(example_test2)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils" )

add_executable(config_admin_test config_admin_test.cpp)
target_link_libraries(config_admin_test Celix::framework ${CELIX_LIBRARIES} CppUTest::CppUTest pthread)


get_property(config_admin_bundle_file TARGET config_admin PROPERTY BUNDLE_FILE)
get_property(example_test_bundle_file TARGET example_test PROPERTY BUNDLE_FILE)
get_property(example_test2_bundle_file TARGET example_test2 PROPERTY BUNDLE_FILE)

configure_file(config.properties.in config.properties @ONLY)


#ADD_TARGET_FOR_TEST(config_admin_test)
add_test(NAME run_config_admin_test COMMAND config_admin_test)
setup_target_for_coverage(config_admin_test)
