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

celix_subproject(NETSTRING "Option to enable building the netstring" OFF)
if (NETSTRING)

    set(NETSTRING_SRC
            src/celix_netstring.c
            )

    add_library(netstring SHARED ${NETSTRING_SRC})

    set_target_properties(netstring
            PROPERTIES
            VERSION 1.0.0
            SOVERSION 1
            OUTPUT_NAME "celix_netstring")
    celix_target_hide_symbols(netstring)

    target_include_directories(netstring PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
            )

    generate_export_header(netstring
            BASE_NAME "CELIX_NETSTRING"
            EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/netstring/celix_netstring_export.h")
    target_include_directories(netstring PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/netstring>)

    install(TARGETS netstring EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT netstring
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/netstring)
    install(DIRECTORY api/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/netstring/ COMPONENT netstring)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/netstring/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/netstring/
            COMPONENT netstring)

    #Alias setup to match external usage
    add_library(Celix::netstring ALIAS netstring)

    if (ENABLE_TESTING)
        add_library(netstring_cut STATIC ${NETSTRING_SRC})
        target_include_directories(netstring_cut PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/api
                ${CMAKE_BINARY_DIR}/celix/gen/includes/netstring
                ${CMAKE_BINARY_DIR}/celix/gen/src/netstring
                )

        add_subdirectory(gtest)

    endif ()
endif ()
