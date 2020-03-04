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

find_package(CppUTest REQUIRED)

add_executable(test_shell
    src/run_tests.cpp
    src/shell_tests.cpp
)

target_link_libraries(test_shell PRIVATE Celix::framework CURL::libcurl ${CPPUTEST_LIBRARY} Celix::shell_api)
target_include_directories(test_shell SYSTEM PRIVATE ${CPPUTEST_INCLUDE_DIRS})
add_dependencies(test_shell shell_bundle)
target_compile_definitions(test_shell PRIVATE -DSHELL_BUNDLE_LOCATION=\"$<TARGET_PROPERTY:shell,BUNDLE_FILE>\")
target_compile_options(test_shell PRIVATE -Wno-deprecated-declarations)
if (CELIX_INSTALL_DEPRECATED_API)
    target_compile_definitions(test_shell PRIVATE CELIX_INSTALL_DEPRECATED_API)
endif ()

add_test(NAME test_shell COMMAND test_shell)
SETUP_TARGET_FOR_COVERAGE(test_shell_cov test_shell ${CMAKE_BINARY_DIR}/coverage/test_shell/test_shell ..)

