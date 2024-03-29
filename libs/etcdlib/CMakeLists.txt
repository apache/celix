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


if (NOT COMMAND celix_subproject) 
    #If COMMAND celix_subproject is not defined, this CMakeLists will
    #act as a top level project. Making the etcdlib useable 
    #stand-alone

    cmake_minimum_required (VERSION 3.18)
    project(ETCDLIB 
        VERSION 1.2.0
        LANGUAGES C CXX
    )

    include(GenerateExportHeader)
    include(GNUInstallDirs)

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    set(CMAKE_C_FLAGS "-pthread -D_GNU_SOURCE -std=gnu99 ${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS "-pthread ${CMAKE_EXE_LINKER_FLAGS}")

    set(ETCDLIB_CMP ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})
    set(ETCDLIB_STANDALONE ON)
else()
    set(ETCDLIB_CMP framework)
    celix_subproject(CELIX_ETCDLIB "Option to build the ETCD library" ON)
endif ()

if (CELIX_ETCDLIB OR ETCDLIB_STANDALONE)
    find_package(CURL REQUIRED)
    find_package(jansson REQUIRED)

    add_library(etcdlib SHARED src/etcd.c)
    target_include_directories(etcdlib PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>)
    target_include_directories(etcdlib PRIVATE src)
    set_target_properties(etcdlib
            PROPERTIES
                VERSION 1.2.0
                SOVERSION 1
                C_VISIBILITY_PRESET hidden
    )
    target_link_libraries(etcdlib PRIVATE CURL::libcurl jansson::jansson ${CELIX_OPTIONAL_EXTRA_LIBS})

    generate_export_header(etcdlib
            BASE_NAME "ETCDLIB"
            EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/etcdlib/etcdlib_export.h")
    target_include_directories(etcdlib PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/etcdlib>)


    add_library(etcdlib_static STATIC src/etcd.c)
    target_include_directories(etcdlib_static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>)
    target_include_directories(etcdlib_static PRIVATE src)
    target_include_directories(etcdlib_static PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/etcdlib>)
    set_target_properties(etcdlib_static
            PROPERTIES
                OUTPUT_NAME "celix_etcdlib_static")
    target_compile_definitions(etcdlib_static PRIVATE  ETCDLIB_STATIC_DEFINE)
    target_link_libraries(etcdlib_static PRIVATE CURL::libcurl jansson::jansson ${CELIX_OPTIONAL_EXTRA_LIBS})

    add_executable(etcdlib_test ${CMAKE_CURRENT_SOURCE_DIR}/test/etcdlib_test.c)
    target_link_libraries(etcdlib_test PRIVATE etcdlib_static CURL::libcurl jansson::jansson)

    install(DIRECTORY api/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/etcdlib COMPONENT ${ETCDLIB_CMP})
    install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/etcdlib/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/etcdlib COMPONENT ${ETCDLIB_CMP})
    if (NOT COMMAND celix_subproject)
        install(TARGETS etcdlib etcdlib_static DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ETCDLIB_CMP}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/etcdlib)
    else ()
        install(TARGETS etcdlib etcdlib_static EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ETCDLIB_CMP}
                INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/etcdlib)
        #Setup target aliases to match external usage
        add_library(Celix::etcdlib ALIAS etcdlib)
        add_library(Celix::etcdlib_static ALIAS etcdlib_static)
    endif ()
endif ()
