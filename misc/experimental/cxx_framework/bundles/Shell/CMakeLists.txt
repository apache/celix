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

add_library(celix_ShellApi INTERFACE)
target_include_directories(celix_ShellApi INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include/celix/shell>
)

execute_process(COMMAND git rev-parse HEAD OUTPUT_VARIABLE GIT_HASH)
configure_file(resources/version.properties.in resources/version.properties @ONLY)

#TODO should eventually be done by a CMake command (i.e. celix_bundle_add_resources)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/celix_cxx_shell_resources.zip
        COMMAND zip -rq ${CMAKE_CURRENT_BINARY_DIR}/celix_cxx_shell_resources.zip *
        COMMENT "Packaging Resources"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources
)

add_custom_target(create_resources ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/celix_cxx_shell_resources.zip
)

add_library(celix_ShellBundle SHARED
        src/ShellActivator.cc
        src/LbCommand.cc
        src/HelpCommand.cc
        src/StopAndStartCommand.cc
        src/InspectCommand.cc
        src/QueryCommand.cc
        src/VersionCommand.cc
        src/LoggingCommand.cc
)
target_include_directories(celix_ShellBundle PRIVATE src)
target_link_libraries(celix_ShellBundle PRIVATE celix_ShellApi)
target_link_libraries(celix_ShellBundle PUBLIC Celix::cxx::Framework)
add_dependencies(celix_ShellBundle create_resources)

#TODO should eventually be done by a CMake command (i.e. the add_celix_bundle)
if (UNIX AND NOT APPLE)
    target_link_libraries(celix_ShellBundle PRIVATE -Wl,--format=binary -Wl,celix_cxx_shell_resources.zip -Wl,--format=default)
else ()
    target_link_libraries(celix_ShellBundle PRIVATE -Wl,-sectcreate -Wl,resources -Wl,celix_cxx_shell -Wl,celix_cxx_shell_resources.zip)
endif ()

add_library(Celix::cxx::ShellApi ALIAS celix_ShellApi)
add_library(Celix::cxx::ShellBundle ALIAS celix_ShellBundle)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()