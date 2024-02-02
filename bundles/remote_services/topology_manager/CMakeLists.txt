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

find_package(libuuid REQUIRED)

add_celix_bundle(rsa_topology_manager
    SOURCES
        src/topology_manager.c
        src/scope.c
        src/activator.c
    VERSION 0.9.0
    SYMBOLIC_NAME "apache_celix_rs_topology_manager"
    GROUP "Celix/RSA"
    NAME "Apache Celix RS Topology Manager"
    FILENAME celix_rsa_topology_manager
)
target_include_directories(rsa_topology_manager PRIVATE src)
target_include_directories(rsa_topology_manager PRIVATE include)
target_link_libraries(rsa_topology_manager PRIVATE Celix::rsa_common Celix::log_helper Celix::c_rsa_spi libuuid::libuuid)
celix_deprecated_utils_headers(rsa_topology_manager)
celix_deprecated_framework_headers(rsa_topology_manager)

install_celix_bundle(rsa_topology_manager EXPORT celix COMPONENT rsa)
#Setup target aliases to match external usage
add_library(Celix::rsa_topology_manager ALIAS rsa_topology_manager)

if (ENABLE_TESTING AND BUILD_RSA_REMOTE_SERVICE_ADMIN_DFI AND BUILD_RSA_DISCOVERY_COMMON)
    find_package(jansson REQUIRED)
    add_subdirectory(tms_tst)
endif ()

if (ENABLE_TESTING)
    add_library(rsa_topology_manager_cut STATIC src/topology_manager.c src/scope.c)
    celix_deprecated_utils_headers(rsa_topology_manager_cut)
    celix_deprecated_framework_headers(rsa_topology_manager_cut)
    target_include_directories(rsa_topology_manager_cut PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/src)
    target_link_libraries(rsa_topology_manager_cut PUBLIC Celix::rsa_common Celix::log_helper Celix::c_rsa_spi libuuid::libuuid)
    add_subdirectory(gtest)
endif ()