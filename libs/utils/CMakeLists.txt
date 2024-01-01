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

celix_subproject(UTILS "Option to enable building the Utilities library" ON)
if (UTILS)
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

    set(UTILS_SRC
            src/array_list.c
            src/hash_map.c
            src/linked_list.c
            src/linked_list_iterator.c
            src/celix_threads.c
            src/version.c
            src/version_range.c
            src/properties.c
            src/utils.c
            src/filter.c
            src/celix_log_level.c
            src/celix_log_utils.c
            src/celix_hash_map.c
            src/celix_file_utils.c
            src/celix_convert_utils.c
            src/celix_errno.c
            src/celix_err.c
            src/celix_cleanup.c
            ${MEMSTREAM_SOURCES}
            )
    set(UTILS_PRIVATE_DEPS libzip::zip)
    set(UTILS_PUBLIC_DEPS)

    add_library(utils SHARED ${UTILS_SRC})

    set_target_properties(utils
            PROPERTIES
            SOVERSION ${CELIX_MAJOR}
            OUTPUT_NAME "celix_utils")
    celix_target_hide_symbols(utils)

    if (NOT OPEN_MEMSTREAM_EXISTS)
        target_compile_definitions(utils PUBLIC -DCELIX_UTILS_NO_MEMSTREAM_AVAILABLE)
    endif ()

    if (ANDROID)
        target_compile_definitions(utils PRIVATE -DUSE_FILE32API)
    endif ()

    if (NOT APPLE)
        set(UTILS_PUBLIC_DEPS ${UTILS_PUBLIC_DEPS} rt)
    endif ()

    target_include_directories(utils PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include_internal> #Note not installed, only for project internal usage
            $<BUILD_INTERFACE:${MEMSTREAM_INCLUDE_DIR}>
            )
    target_include_directories(utils PRIVATE src include_deprecated)
    IF(UNIX AND NOT ANDROID)
        set(UTILS_PRIVATE_DEPS ${UTILS_PRIVATE_DEPS} m pthread)
    ELSEIF(ANDROID)
        set(UTILS_PRIVATE_DEPS ${UTILS_PRIVATE_DEPS} m)
    ENDIF()

    target_link_libraries(utils PUBLIC ${UTILS_PUBLIC_DEPS} PRIVATE ${UTILS_PRIVATE_DEPS})
    generate_export_header(utils
            BASE_NAME "CELIX_UTILS"
            EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/utils/celix_utils_export.h")
    target_include_directories(utils PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/utils>)
    target_include_directories(utils PRIVATE $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/src/utils>)

    #Configure celix_err_constant header file
    set(CELIX_ERR_BUFFER_SIZE 512 CACHE STRING "The size of the thread-specific buffer used for library error messages")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/src/celix_err_constants.h.in" "${CMAKE_BINARY_DIR}/celix/gen/includes/utils/celix_err_constants.h" @ONLY)

    #Configure celix_utils_private_constants header file
    set(CELIX_UTILS_MAX_STRLEN 1073741824 CACHE STRING "The maximum string length used for string util functions")
    set(CELIX_PROPERTIES_OPTIMIZATION_STRING_BUFFER_SIZE 128 CACHE STRING "The string optimization buffer size used for properties")
    set(CELIX_PROPERTIES_OPTIMIZATION_ENTRIES_BUFFER_SIZE 16 CACHE STRING "The entries optimization buffer size used for properties")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/src/celix_utils_private_constants.h.in" "${CMAKE_BINARY_DIR}/celix/gen/src/utils/celix_utils_private_constants.h" @ONLY)

    install(TARGETS utils EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils)
    install(DIRECTORY include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils/
            COMPONENT framework PATTERN memstream* EXCLUDE)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/utils/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils/
            COMPONENT framework)

    #TODO #509, remove installation of deprecated headers
    install(DIRECTORY include_deprecated/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/utils COMPONENT framework)

    #Alias setup to match external usage
    add_library(Celix::utils ALIAS utils)

    if (ENABLE_TESTING AND EI_TESTS)
        add_subdirectory(error_injector)
    endif ()

    if (ENABLE_TESTING)
        add_library(utils_cut STATIC ${UTILS_SRC})
        target_compile_definitions(utils_cut PRIVATE CELIX_UTILS_STATIC_DEFINE)
        target_include_directories(utils_cut PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_CURRENT_LIST_DIR}/include_internal
                ${MEMSTREAM_INCLUDE_DIR}
                ${CMAKE_BINARY_DIR}/celix/gen/includes/utils
                ${CMAKE_BINARY_DIR}/celix/gen/src/utils
                src include_deprecated
                )
        target_link_libraries(utils_cut PUBLIC ${UTILS_PUBLIC_DEPS} ${UTILS_PRIVATE_DEPS})

        add_subdirectory(gtest)

        find_package(CppUTest)

        if (CppUTest_FOUND)
            include_directories(SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR})
            include_directories(include)
            include_directories(src)

            add_executable(hash_map_test private/test/hash_map_test.cpp)
            target_include_directories(hash_map_test PRIVATE include_deprecated)
            target_link_libraries(hash_map_test utils_cut CppUTest pthread)

            add_executable(array_list_test private/test/array_list_test.cpp)
            target_include_directories(array_list_test PRIVATE include_deprecated)
            target_link_libraries(array_list_test utils_cut CppUTest pthread)

            add_executable(linked_list_test private/test/linked_list_test.cpp)
            target_include_directories(linked_list_test PRIVATE include_deprecated)
            target_link_libraries(linked_list_test utils_cut CppUTest pthread)

            add_test(NAME run_array_list_test COMMAND array_list_test)
            add_test(NAME run_hash_map_test COMMAND hash_map_test)
            add_test(NAME run_linked_list_test COMMAND linked_list_test)

            setup_target_for_coverage(hash_map_test)
            setup_target_for_coverage(array_list_test)
            setup_target_for_coverage(linked_list_test)
            setup_target_for_coverage(ip_utils_test)
        else ()
            message(WARNING "Cannot find CppUTest, deprecated cpputest-based unit test will not be added")
        endif () #end CppUTest_FOUND

    endif ()

    add_subdirectory(benchmark)
endif ()
