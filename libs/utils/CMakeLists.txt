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

set(MEMSTREAM_SOURCES )
set(MEMSTREAM_INCLUDES )
if (APPLE OR ANDROID)
    set(MEMSTREAM_SOURCES src/memstream/open_memstream.c  src/memstream/fmemopen.c)
    set(MEMSTREAM_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include/memstream)
    install(DIRECTORY include/memstream/ DESTINATION include/celix/memstream COMPONENT framework)
endif()

add_library(utils SHARED
    src/array_list.c
    src/hash_map.c
    src/linked_list.c
    src/linked_list_iterator.c
    src/celix_threads.c
    src/version.c
    src/version_range.c
    src/thpool.c
    src/properties.c
    src/utils.c
    src/filter.c
    ${MEMSTREAM_SOURCES}
)
set_target_properties(utils PROPERTIES OUTPUT_NAME "celix_utils")

if (ANDROID)
    target_compile_definitions(utils PRIVATE -DUSE_FILE32API)
endif ()

target_include_directories(utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${MEMSTREAM_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include/celix>
)
target_include_directories(utils PRIVATE src)
set_target_properties(utils PROPERTIES "SOVERSION" 2)

IF(UNIX AND NOT ANDROID)
    target_link_libraries(utils PRIVATE m pthread)
ELSEIF(ANDROID)
    target_link_libraries(utils PRIVATE m)
ENDIF()

install(TARGETS utils EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)
install(DIRECTORY include/ DESTINATION include/celix COMPONENT framework
        PATTERN memstream* EXCLUDE)

#Alias setup to match external usage
add_library(Celix::utils ALIAS utils)


celix_subproject(UTILS-TESTS "Option to build the utilities library tests" "OFF")
if (ENABLE_TESTING AND UTILS-TESTS)
    find_package(CppUTest REQUIRED)

    include_directories(SYSTEM ${CPPUTEST_INCLUDE_DIR})
    include_directories(include)
    include_directories(src)

    add_executable(hash_map_test private/test/hash_map_test.cpp)
    target_link_libraries(hash_map_test Celix::utils ${CPPUTEST_LIBRARY} pthread)

    add_executable(array_list_test private/test/array_list_test.cpp)
    target_link_libraries(array_list_test  Celix::utils ${CPPUTEST_LIBRARY} pthread)

    add_executable(celix_threads_test private/test/celix_threads_test.cpp)
    target_link_libraries(celix_threads_test Celix::utils ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} pthread)
    add_executable(linked_list_test private/test/linked_list_test.cpp)
    target_link_libraries(linked_list_test  Celix::utils ${CPPUTEST_LIBRARY} pthread)

    add_executable(thread_pool_test private/test/thread_pool_test.cpp)
    target_link_libraries(thread_pool_test  Celix::utils ${CPPUTEST_LIBRARY} pthread)

    add_executable(properties_test private/test/properties_test.cpp)
    target_link_libraries(properties_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY}  Celix::utils pthread)

    add_executable(utils_test private/test/utils_test.cpp)
    target_link_libraries(utils_test ${CPPUTEST_LIBRARY}  Celix::utils pthread)

    add_executable(filter_test private/test/filter_test.cpp)
    target_link_libraries(filter_test ${CPPUTEST_LIBRARY}  Celix::utils pthread)

    configure_file(private/resources-test/properties.txt ${CMAKE_CURRENT_BINARY_DIR}/resources-test/properties.txt COPYONLY)

    add_test(NAME run_array_list_test COMMAND array_list_test)
    add_test(NAME run_hash_map_test COMMAND hash_map_test)
    add_test(NAME run_celix_threads_test COMMAND celix_threads_test)
    add_test(NAME run_thread_pool_test COMMAND thread_pool_test)
    add_test(NAME run_linked_list_test COMMAND linked_list_test)
    add_test(NAME run_properties_test COMMAND properties_test)
    add_test(NAME run_utils_test COMMAND utils_test)
    add_test(NAME filter_test COMMAND filter_test)

    SETUP_TARGET_FOR_COVERAGE(array_list_test array_list_test ${CMAKE_BINARY_DIR}/coverage/array_list_test/array_list_test)
    SETUP_TARGET_FOR_COVERAGE(hash_map hash_map_test ${CMAKE_BINARY_DIR}/coverage/hash_map_test/hash_map_test)
    SETUP_TARGET_FOR_COVERAGE(celix_threads_test celix_threads_test ${CMAKE_BINARY_DIR}/coverage/celix_threads_test/celix_threads_test)
    SETUP_TARGET_FOR_COVERAGE(thread_pool_test thread_pool_test ${CMAKE_BINARY_DIR}/coverage/thread_pool_test/thread_pool_test)
    SETUP_TARGET_FOR_COVERAGE(linked_list_test linked_list_test ${CMAKE_BINARY_DIR}/coverage/linked_list_test/linked_list_test)
    SETUP_TARGET_FOR_COVERAGE(properties_test properties_test ${CMAKE_BINARY_DIR}/coverage/properties_test/properties_test)
    SETUP_TARGET_FOR_COVERAGE(utils_test utils_test ${CMAKE_BINARY_DIR}/coverage/utils_test/utils_test)

endif(ENABLE_TESTING AND UTILS-TESTS)
