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

include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("/opt/local/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/private/include")
link_directories("/opt/local/lib")

add_executable(hash_map_test hash_map_test.c)
target_link_libraries(hash_map_test framework cunit ncurses)

add_executable(hash_map_test_hash hash_map_test_hash.c)
target_link_libraries(hash_map_test_hash framework cunit ncurses)

add_executable(array_list_test array_list_test.c)
target_link_libraries(array_list_test framework cunit ncurses)

add_executable(framework_test framework_test.c)
target_link_libraries(framework_test framework cunit ncurses)

enable_testing()
#add_test (tester ${EXECUTABLE_OUTPUT_PATH}/tester)
add_test (hash_map_test ${EXECUTABLE_OUTPUT_PATH}/hash_map_test)
add_test (hash_map_test_hash ${EXECUTABLE_OUTPUT_PATH}/hash_map_test_hash)
add_test (array_list_test ${EXECUTABLE_OUTPUT_PATH}/array_list_test)
add_test (framework_test ${EXECUTABLE_OUTPUT_PATH}/framework_test)