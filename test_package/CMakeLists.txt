#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance #  with the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing,
#   software distributed under the License is distributed on an
#   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#   KIND, either express or implied.  See the License for the
#   specific language governing permissions and limitations
#   under the License.

cmake_minimum_required (VERSION 3.14)
project(test_package)

set(CMAKE_CXX_STANDARD 17)

find_package(Celix REQUIRED)

add_celix_bundle(hello VERSION 1.0.0 SOURCES hello_bundle.c)
target_link_libraries(hello PRIVATE Celix::log_helper)
celix_get_bundle_file(hello HELLO_TEST_BUNDLE)

add_executable(use_framework test_framework.c)
target_link_libraries(use_framework Celix::framework)
add_celix_bundle_dependencies(use_framework hello)
target_compile_definitions(use_framework PRIVATE HELLO_TEST_BUNDLE_LOCATION="${HELLO_TEST_BUNDLE}")

option(TEST_HTTP_ADMIN "Test http_admin" OFF)
if (TEST_HTTP_ADMIN)
    add_celix_bundle(http_admin_sut
            SOURCES
            test_http_admin_activator.c
            VERSION 1.0.0
            )
    target_link_libraries(http_admin_sut PRIVATE Celix::http_admin_api)
    add_celix_container(use_http_admin COPY
            BUNDLES
            Celix::http_admin
            http_admin_sut
            )
endif ()

option(TEST_LOG_SERVICE "Test log_admin" OFF)
if (TEST_LOG_SERVICE)
    add_celix_bundle(my_log_writer
            SOURCES
            my_log_writer_activator.c
            VERSION 1.0.0
            )
    target_link_libraries(my_log_writer PRIVATE Celix::log_helper Celix::log_service_api)
    add_celix_container(use_log_writer COPY)
    celix_container_bundles(use_log_writer LEVEL 0 Celix::log_admin)
    celix_container_bundles(use_log_writer LEVEL 1 my_log_writer)
    celix_container_bundles(use_log_writer LEVEL 2 hello)
endif ()

option(TEST_SYSLOG_WRITER "Test syslog_writer" OFF)
if (TEST_SYSLOG_WRITER)
    add_celix_container(use_syslog_writer COPY)
    celix_container_bundles(use_syslog_writer LEVEL 0 Celix::log_admin)
    celix_container_bundles(use_syslog_writer LEVEL 1 Celix::syslog_writer)
    celix_container_bundles(use_syslog_writer LEVEL 2 hello)
endif ()

option(TEST_SHELL "Test shell" OFF)
if (TEST_SHELL)
    add_executable(use_shell test_shell.c )
    target_link_libraries(use_shell PRIVATE Celix::framework Celix::shell_api)
    celix_get_bundle_file(Celix::shell SHELL_BUNDLE)
    target_compile_definitions(use_shell PRIVATE SHELL_BUNDLE_LOCATION="${SHELL_BUNDLE}")
endif ()

option(TEST_REMOTE_SHELL "Test remote shell" OFF)
if(TEST_REMOTE_SHELL)
    add_celix_container("use_remote_shell" BUNDLES Celix::shell Celix::remote_shell hello)
endif ()

option(TEST_SHELL_TUI "Test shell TUI" OFF)
if(TEST_SHELL_TUI)
    add_celix_container("use_shell_tui" BUNDLES Celix::shell Celix::shell_tui hello)
endif ()

option(TEST_SHELL_WUI "Test shell WUI" OFF)
if(TEST_SHELL_WUI)
    add_celix_container("use_shell_wui" BUNDLES Celix::shell Celix::shell_wui hello)
endif ()
