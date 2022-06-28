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

find_package(libzip REQUIRED)

set(MEMSTREAM_SOURCES )
set(MEMSTREAM_INCLUDES )
include(CheckFunctionExists)
#check_function_exists(fmemopen FMEMOPEN_EXISTS)
check_function_exists(open_memstream OPEN_MEMSTREAM_EXISTS)
if (NOT OPEN_MEMSTREAM_EXISTS)
    set(MEMSTREAM_SOURCES src/memstream/open_memstream.c  src/memstream/fmemopen.c ${MEMSTREAM_SOURCES})
    set(MEMSTREAM_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include/memstream ${MEMSTREAM_INCLUDE_DIR})
    install(DIRECTORY include/memstream/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/memstream COMPONENT framework)
endif()

add_library(utils SHARED
    src/array_list.c
    src/hash_map.c
    src/linked_list.c
    src/linked_list_iterator.c
    src/celix_threads.c
    src/version.c
    src/version_range.c
    src/properties.c
    src/utils.c
    src/ip_utils.c
    src/filter.c
    src/celix_log_utils.c
    src/celix_hash_map.c
    src/celix_file_utils.c
    ${MEMSTREAM_SOURCES}
)
set_target_properties(utils PROPERTIES OUTPUT_NAME "celix_utils")
target_link_libraries(utils PRIVATE libzip::libzip)

if (NOT OPEN_MEMSTREAM_EXISTS)
    target_compile_definitions(utils PUBLIC -DNO_MEMSTREAM_AVAILABLE)
endif ()

if (ANDROID)
    target_compile_definitions(utils PRIVATE -DUSE_FILE32API)
endif ()

if (NOT APPLE) 
    target_link_libraries(utils PUBLIC -lrt)
endif ()

target_include_directories(utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${MEMSTREAM_INCLUDE_DIR}>
)
target_include_directories(utils PRIVATE src)
set_target_properties(utils PROPERTIES "SOVERSION" 2)

IF(UNIX AND NOT ANDROID)
    target_link_libraries(utils PRIVATE m pthread)
ELSEIF(ANDROID)
    target_link_libraries(utils PRIVATE m)
ENDIF()

install(TARGETS utils EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix COMPONENT framework
        PATTERN memstream* EXCLUDE)

#Alias setup to match external usage
add_library(Celix::utils ALIAS utils)


if (ENABLE_TESTING)
    add_subdirectory(gtest)

    find_package(CppUTest)

    if (CppUTest_FOUND)
        include_directories(SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR})
        include_directories(include)
        include_directories(src)

        add_executable(hash_map_test private/test/hash_map_test.cpp)
        target_link_libraries(hash_map_test Celix::utils CppUTest::CppUTest pthread)

        add_executable(array_list_test private/test/array_list_test.cpp)
        target_link_libraries(array_list_test  Celix::utils CppUTest::CppUTest pthread)

        add_executable(celix_threads_test private/test/celix_threads_test.cpp)
        target_link_libraries(celix_threads_test Celix::utils CppUTest::CppUTest ${CppUTest_EXT_LIBRARIES} pthread)

        add_executable(linked_list_test private/test/linked_list_test.cpp)
        target_link_libraries(linked_list_test  Celix::utils CppUTest::CppUTest pthread)

        add_executable(properties_test private/test/properties_test.cpp)
        target_link_libraries(properties_test CppUTest::CppUTest CppUTest::CppUTestExt  Celix::utils pthread)

        add_executable(utils_test private/test/utils_test.cpp)
        target_link_libraries(utils_test CppUTest::CppUTest  Celix::utils pthread)

        add_executable(ip_utils_test private/test/ip_utils_test.cpp)
        target_link_libraries(ip_utils_test CppUTest::CppUTest  Celix::utils pthread)

        add_executable(filter_test private/test/filter_test.cpp)
        target_link_libraries(filter_test CppUTest::CppUTest  Celix::utils pthread)

        add_executable(version_test private/test/version_test.cpp)
        target_link_libraries(version_test CppUTest::CppUTest  Celix::utils pthread)

        configure_file(private/resources-test/properties.txt ${CMAKE_CURRENT_BINARY_DIR}/resources-test/properties.txt COPYONLY)

        add_test(NAME run_array_list_test COMMAND array_list_test)
        add_test(NAME run_hash_map_test COMMAND hash_map_test)
        add_test(NAME run_celix_threads_test COMMAND celix_threads_test)
        add_test(NAME run_linked_list_test COMMAND linked_list_test)
        add_test(NAME run_properties_test COMMAND properties_test)
        add_test(NAME run_utils_test COMMAND utils_test)
        add_test(NAME run_ip_utils_test COMMAND ip_utils_test)
        add_test(NAME filter_test COMMAND filter_test)
        add_test(NAME version_test COMMAND version_test)

        setup_target_for_coverage(array_list_test)
        setup_target_for_coverage(hash_map_test)
        setup_target_for_coverage(celix_threads_test)
        setup_target_for_coverage(linked_list_test)
        setup_target_for_coverage(properties_test)
        setup_target_for_coverage(utils_test)
        setup_target_for_coverage(ip_utils_test)
        setup_target_for_coverage(filter_test)
        setup_target_for_coverage(version_test)
    else ()
        message(WARNING "Cannot find CppUTest, deprecated cpputest-based unit test will not be added")
    endif () #end CppUTest_FOUND

endif ()

add_subdirectory(benchmark)
