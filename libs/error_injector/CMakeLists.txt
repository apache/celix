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

add_library(error_injector INTERFACE)
target_include_directories(error_injector INTERFACE ${CMAKE_CURRENT_LIST_DIR}/api)
# get caller address of the target function, into which errors are injected
target_link_libraries(error_injector INTERFACE dl)
target_link_options(error_injector INTERFACE -rdynamic)
target_compile_options(error_injector INTERFACE -Wno-frame-address)
add_library(Celix::error_injector ALIAS error_injector)

add_subdirectory(malloc)
add_subdirectory(asprintf)
add_subdirectory(stdio)
add_subdirectory(stdlib)
add_subdirectory(string)
add_subdirectory(eventfd)
add_subdirectory(stat)
add_subdirectory(fts)
add_subdirectory(dlfcn)
add_subdirectory(ifaddrs)
add_subdirectory(sys_shm)
add_subdirectory(socket)
add_subdirectory(pthread)
add_subdirectory(unistd)
add_subdirectory(dirent)

celix_subproject(ERROR_INJECTOR_MDNSRESPONDER "Option to enable building the mdnsresponder error injector" OFF)
if (ERROR_INJECTOR_MDNSRESPONDER)
    add_subdirectory(mdnsresponder)
endif ()

celix_subproject(ERROR_INJECTOR_JANSSON "Option to enable building the jansson error injector" ON)
if (ERROR_INJECTOR_JANSSON)
    add_subdirectory(jansson)
endif ()

celix_subproject(ERROR_INJECTOR_MOSQUITTO "Option to enable building the mosquitto error injector" OFF)
if (ERROR_INJECTOR_MOSQUITTO)
    add_subdirectory(mosquitto)
endif ()
