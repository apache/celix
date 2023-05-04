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

add_library(utils_obj OBJECT
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
        src/celix_convert_utils.c
        src/celix_errorcodes.c
        src/celix_err.c
        ${MEMSTREAM_SOURCES}
)

target_link_libraries(utils_obj PRIVATE libzip::libzip)
#Note visibility preset is also needed on OBJECT libraries, to work correctly
set_target_properties(utils_obj PROPERTIES C_VISIBILITY_PRESET hidden)
if (NOT OPEN_MEMSTREAM_EXISTS)
    target_compile_definitions(utils_obj PUBLIC -DCELIX_UTILS_NO_MEMSTREAM_AVAILABLE)
endif ()

if (ANDROID)
    target_compile_definitions(utils_obj PRIVATE -DUSE_FILE32API)
endif ()

if (NOT APPLE)
    target_link_libraries(utils_obj PUBLIC -lrt)
endif ()

target_include_directories(utils_obj PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${MEMSTREAM_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include/celix/utils>)
target_include_directories(utils_obj PRIVATE src include_deprecated)
IF(UNIX AND NOT ANDROID)
    target_link_libraries(utils_obj PRIVATE m pthread)
ELSEIF(ANDROID)
    target_link_libraries(utils_obj PRIVATE m)
ENDIF()

add_library(utils SHARED)
target_link_libraries(utils PUBLIC utils_obj)
target_include_directories(utils_obj PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/utils>)

#Configure celix_err_constant header file
set(CELIX_ERR_BUFFER_SIZE 512 CACHE STRING "The size of the thread-specific buffer used for library error messages")
configure_file("${CMAKE_CURRENT_LIST_DIR}/src/celix_err_constants.h.in" "${CMAKE_BINARY_DIR}/celix/gen/includes/utils/celix_err_constants.h" @ONLY)

#Configure export header
generate_export_header(utils
        BASE_NAME "CELIX_UTILS"
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/utils/celix_utils_export.h")
# We are building this library
target_compile_definitions(utils_obj PRIVATE utils_EXPORTS)

set_target_properties(utils
        PROPERTIES
        C_VISIBILITY_PRESET hidden
        SOVERSION ${CELIX_MAJOR}
        OUTPUT_NAME "celix_utils")

# By omitting OBJECTS DESTINATION, object files will NOT be installed.
install(TARGETS utils utils_obj EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils)
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils/
        COMPONENT framework PATTERN memstream* EXCLUDE)
install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/utils/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils/
        COMPONENT framework)

if (CELIX_INSTALL_DEPRECATED_API)
    install(DIRECTORY include_deprecated/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils COMPONENT framework)
endif ()

#Alias setup to match external usage
add_library(Celix::utils ALIAS utils)
add_library(Celix::utils_obj ALIAS utils_obj)


if (ENABLE_TESTING)
    if (CELIX_CXX17) #utils tests are C++17
        add_subdirectory(gtest)
    endif()

    find_package(CppUTest)

    if (CppUTest_FOUND)
        include_directories(SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR})
        include_directories(include)
        include_directories(src)

        add_executable(hash_map_test private/test/hash_map_test.cpp)
        target_include_directories(hash_map_test PRIVATE include_deprecated)
        target_link_libraries(hash_map_test Celix::utils CppUTest::CppUTest pthread)

        add_executable(array_list_test private/test/array_list_test.cpp)
        target_include_directories(array_list_test PRIVATE include_deprecated)
        target_link_libraries(array_list_test  Celix::utils CppUTest::CppUTest pthread)

        add_executable(linked_list_test private/test/linked_list_test.cpp)
        target_include_directories(linked_list_test PRIVATE include_deprecated)
        target_link_libraries(linked_list_test  Celix::utils CppUTest::CppUTest pthread)

        add_executable(properties_test private/test/properties_test.cpp)
        target_include_directories(properties_test PRIVATE include_deprecated)
        target_link_libraries(properties_test CppUTest::CppUTest CppUTest::CppUTestExt  Celix::utils pthread)

        add_executable(ip_utils_test private/test/ip_utils_test.cpp)
        target_include_directories(ip_utils_test PRIVATE include_deprecated)
        target_link_libraries(ip_utils_test CppUTest::CppUTest  Celix::utils pthread)

        add_executable(version_test private/test/version_test.cpp)
        target_include_directories(version_test PRIVATE include_deprecated)
        target_link_libraries(version_test CppUTest::CppUTest  Celix::utils pthread)


        if (LINKER_WRAP_SUPPORTED)
            add_executable(version_ei_test private/test/version_ei_test.cc)
            target_include_directories(version_ei_test PRIVATE include_deprecated)
            target_link_libraries(version_ei_test CppUTest::CppUTest Celix::utils_obj Celix::malloc_ei Celix::utils_ei pthread)
            add_test(NAME version_ei_test COMMAND version_ei_test)
        endif ()

        configure_file(private/resources-test/properties.txt ${CMAKE_CURRENT_BINARY_DIR}/resources-test/properties.txt COPYONLY)

        add_test(NAME run_array_list_test COMMAND array_list_test)
        add_test(NAME run_hash_map_test COMMAND hash_map_test)
        add_test(NAME run_linked_list_test COMMAND linked_list_test)
        add_test(NAME run_properties_test COMMAND properties_test)
        add_test(NAME run_ip_utils_test COMMAND ip_utils_test)
        add_test(NAME version_test COMMAND version_test)

        setup_target_for_coverage(array_list_test)
        setup_target_for_coverage(hash_map_test)
        setup_target_for_coverage(linked_list_test)
        setup_target_for_coverage(properties_test)
        setup_target_for_coverage(ip_utils_test)
        setup_target_for_coverage(version_test)
    else ()
        message(WARNING "Cannot find CppUTest, deprecated cpputest-based unit test will not be added")
    endif () #end CppUTest_FOUND

endif ()

add_subdirectory(benchmark)
