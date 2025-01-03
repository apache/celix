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

celix_subproject(SHELL_API "Option to enable building the Shell API" ON)
if (SHELL_API)
    add_library(shell_api INTERFACE)
    target_include_directories(shell_api INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
            )
    target_link_libraries(shell_api INTERFACE Celix::utils)

    install(TARGETS shell_api EXPORT celix COMPONENT shell
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/shell)
    install(DIRECTORY api/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/shell COMPONENT shell)

    add_library(Celix::shell_api ALIAS shell_api)
endif ()

celix_subproject(SHELL "Option to enable building the Shell bundles" ON)
if (SHELL)

    add_library(shell_commands STATIC
            src/lb_command.c
            src/start_command.c
            src/stop_command.c
            src/install_command.c
            src/update_command.c
            src/uninstall_command.c
            src/unload_command.c
            src/help_command.c
            src/dm_shell_list_command.c
            src/query_command.c
            src/quit_command.c
            src/std_commands.c
            src/bundle_command.c)
    target_include_directories(shell_commands PRIVATE src)
    target_link_libraries(shell_commands PRIVATE Celix::shell_api Celix::log_helper)

    add_celix_bundle(shell
            SYMBOLIC_NAME "apache_celix_c_shell"
            VERSION "2.1.0"
            NAME "Apache Celix C Shell"
            FILENAME celix_shell
            GROUP "Celix/Shell"
            SOURCES
            src/c_shell_activator.c
            src/c_shell.c
            )
    target_include_directories(shell PRIVATE src)
    target_link_libraries(shell PRIVATE shell_commands Celix::shell_api Celix::log_helper)
    celix_deprecated_utils_headers(shell)
    install_celix_bundle(shell EXPORT celix COMPONENT shell)

    #Setup target aliases to match external usage
    add_library(Celix::shell ALIAS shell)

    if (CELIX_CXX14)
        add_celix_bundle(ShellCxx
                SYMBOLIC_NAME "Apache_Celix_CxxShell"
                VERSION "2.1.0"
                NAME "Apache Celix CXX Shell"
                FILENAME celix_ShellCxx
                GROUP "Celix/Shell"
                SOURCES
                src/Shell.cc
                )
        target_include_directories(ShellCxx PRIVATE src)
        target_link_libraries(ShellCxx PRIVATE shell_commands Celix::shell_api Celix::log_service_api Celix::log_helper)
        install_celix_bundle(ShellCxx EXPORT celix COMPONENT shell)
        add_library(Celix::ShellCxx ALIAS ShellCxx)
    endif ()

    if (ENABLE_TESTING)
        add_subdirectory(gtest)
    endif ()
endif (SHELL)
