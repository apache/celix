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

#TODO rename to celix::shell && celix::shell_api

add_library(celix_ShellApi INTERFACE)
target_include_directories(celix_ShellApi INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include/celix/shell>
)

#TODO should eventually be done by a CMake command (i.e. celix_bundle_add_resources)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/celix_cxx_shell_resources.zip
        COMMAND zip -rq ${CMAKE_CURRENT_BINARY_DIR}/celix_cxx_shell_resources.zip *
        COMMENT "Packaging Resources"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources
)

add_custom_target(create_resources ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/celix_cxx_shell_resources.zip
)

add_library(celix_Shell SHARED
        src/ShellActivator.cc
        src/LbCommand.cc
        src/HelpCommand.cc
        src/StopAndStartCommand.cc
        src/InspectCommand.cc
        src/QueryCommand.cc
        src/VersionCommand.cc
)
target_include_directories(celix_Shell PRIVATE src)
target_link_libraries(celix_Shell PRIVATE celix_ShellApi glog::glog)
target_link_libraries(celix_Shell PUBLIC celix::cxx_Framework)
add_dependencies(celix_Shell create_resources)

#TODO should eventually be done by a CMake command (i.e. the add_celix_bundle)
if (UNIX AND NOT APPLE)
    target_link_libraries(celix_Shell PRIVATE -Wl,--format=binary -Wl,celix_cxx_shell_resources.zip -Wl,--format=default)
else ()
    target_link_libraries(celix_Shell PRIVATE -Wl,-sectcreate -Wl,resources -Wl,celix_cxx_shell -Wl,celix_cxx_shell_resources.zip)
endif ()


if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()

add_library(celix::ShellApi ALIAS celix_ShellApi)
add_library(celix::Shell ALIAS celix_Shell)