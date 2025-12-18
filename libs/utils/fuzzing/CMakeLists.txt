# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if (ENABLE_ADDRESS_SANITIZER OR ENABLE_UNDEFINED_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libsan")
endif ()

add_executable(celix_properties_fuzzer src/PropertiesFuzz.cc)
target_link_libraries(celix_properties_fuzzer PRIVATE utils_cuf)
target_compile_options(celix_properties_fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(celix_properties_fuzzer PRIVATE -fsanitize=fuzzer)

add_executable(celix_version_fuzzer src/VersionFuzz.cc)
target_link_libraries(celix_version_fuzzer PRIVATE utils_cuf)
target_compile_options(celix_version_fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(celix_version_fuzzer PRIVATE -fsanitize=fuzzer)

add_executable(celix_filter_fuzzer src/FilterFuzz.cc)
target_link_libraries(celix_filter_fuzzer PRIVATE utils_cuf)
target_compile_options(celix_filter_fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(celix_filter_fuzzer PRIVATE -fsanitize=fuzzer)

add_custom_command(TARGET celix_properties_fuzzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/properties_corpus
    $<TARGET_FILE_DIR:celix_properties_fuzzer>/properties_corpus
)

add_custom_command(TARGET celix_version_fuzzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/version_corpus
    $<TARGET_FILE_DIR:celix_version_fuzzer>/version_corpus
)

add_custom_command(TARGET celix_filter_fuzzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/filter_corpus
    $<TARGET_FILE_DIR:celix_filter_fuzzer>/filter_corpus
)

