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

find_package(RapidJSON REQUIRED)

add_celix_bundle(async_rsa_discovery
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_async_remote_service_admin_discovery"
        NAME "Apache Celix Async Remote Service Admin Discovery"
        GROUP "Celix/AsyncRSA"
        SOURCES
#        src/discovery.cc
        src/ConfiguredEndpoint.cc
        src/ConfiguredDiscoveryManager.cc
        src/ConfiguredDiscoveryManagerActivator.cc
        )
target_include_directories(async_rsa_discovery PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include/celix/async_rsa_discovery>)
target_link_libraries(async_rsa_discovery PUBLIC
        Celix::async_rsa_common
        Celix::framework
        )

#if (ENABLE_TESTING)
#    add_subdirectory(gtest)
#endif()


install_celix_bundle(async_rsa_discovery EXPORT celix COMPONENT async_rsa_discovery)
#Setup target aliases to match external usage
add_library(Celix::async_rsa_discovery ALIAS async_rsa_discovery)
install(TARGETS async_rsa_discovery EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT async_rsa_discovery)
install(DIRECTORY include/ DESTINATION include/celix/async_rsa_discovery COMPONENT async_rsa_discovery)