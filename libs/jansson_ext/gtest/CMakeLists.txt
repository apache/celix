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

set(TEST_SOURCES
    src/test_smoke.cpp
    src/test_uri.cpp
    src/test_errors.cpp
    src/test_format_check.cpp
    src/test_ref.cpp
    src/test_defaults.cpp
    src/test_combinations.cpp
    src/test_suite.cpp
    src/test_path.cpp
    src/test_pointer.cpp
    src/test_patch.cpp
    src/test_json_patch.cpp
    src/test_validate_uri.cpp
    src/test_abort.cpp
    src/test_format_check_extended.cpp
    src/test_schema_keywords.cpp
    src/test_uri_extended.cpp
)

foreach(T ${TEST_SOURCES})
    get_filename_component(NAME ${T} NAME_WE)
    add_executable(${NAME} ${T})
    target_link_libraries(${NAME} PRIVATE jansson_ext_cut GTest::gtest GTest::gtest_main)
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${NAME}
        PRIVATE TEST_SUITE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/JSON-Schema-Test-Suite")
    add_test(NAME ${NAME} COMMAND ${NAME})
    setup_target_for_coverage(${NAME} SCAN_DIR ..)
endforeach()
