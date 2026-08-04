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

celix_subproject(JANSSON_EXT "Option to enable building the Jansson extension library (JSON Schema validation, JSON Pointer and JSON Patch)" OFF)
if (JANSSON_EXT)
    find_package(jansson REQUIRED)

    set(JANSSON_EXT_SOURCES
        src/celix_jansson_schema.c
        src/celix_jansson_pointer.c
        src/celix_json_patch.c
        src/celix_jansson_uri.c
        src/celix_hash_table.c
        src/celix_string_format_check.c
        src/celix_smtp_address_validator.c
        src/celix_util.c
    )

    add_library(jansson_ext SHARED ${JANSSON_EXT_SOURCES})
    set_target_properties(jansson_ext PROPERTIES
        OUTPUT_NAME "celix_jansson_ext"
        VERSION 0.1.0
        SOVERSION 0)
    celix_target_hide_symbols(jansson_ext)
    target_link_libraries(jansson_ext
        PUBLIC jansson::jansson Threads::Threads
        PRIVATE m)
    target_include_directories(jansson_ext
        PRIVATE src
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/jansson_ext>)
    generate_export_header(jansson_ext 
        BASE_NAME CELIX_JANSSON_EXT
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/jansson_ext/celix_jansson_ext_export.h")
    add_library(Celix::jansson_ext ALIAS jansson_ext)

    # Install (always, no option gate)
    install(TARGETS jansson_ext
        EXPORT celix
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/jansson_ext)
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/jansson_ext
        COMPONENT framework)
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/celix/gen/includes/jansson_ext/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/jansson_ext
        COMPONENT framework)

    if (ENABLE_TESTING)
        add_library(jansson_ext_cut STATIC ${JANSSON_EXT_SOURCES})
        target_link_libraries(jansson_ext_cut
            PUBLIC jansson::jansson Threads::Threads
            PRIVATE m)
        target_include_directories(jansson_ext_cut
            PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_CURRENT_LIST_DIR}/src
                ${CMAKE_BINARY_DIR}/celix/gen/includes/jansson_ext)
        target_compile_definitions(jansson_ext_cut PUBLIC CELIX_JANSSON_EXT_STATIC_DEFINE)
        add_subdirectory(gtest)
    endif ()

    celix_subproject(JANSSON_EXT_EXAMPLES "Option to enable building the jansson extension examples" OFF)
    if (JANSSON_EXT_EXAMPLES)
        add_subdirectory(example)
    endif ()
endif ()
