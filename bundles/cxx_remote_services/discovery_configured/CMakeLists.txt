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

#TODO alias and install lib
add_library(RsaConfiguredDiscovery_api INTERFACE)
target_include_directories(RsaConfiguredDiscovery_api INTERFACE
        include
)

add_celix_bundle(RsaConfiguredDiscovery
    VERSION 0.9.0
    SYMBOLIC_NAME "apache::celix::RsaConfiguredDiscovery"
    NAME "Apache Celix Async Configured Discovery"
    GROUP "Celix/RSA"
    SOURCES
        src/ConfiguredDiscoveryManager.cc
        src/ConfiguredDiscoveryManagerActivator.cc
)
target_link_libraries(RsaConfiguredDiscovery PRIVATE
        RsaConfiguredDiscovery_api
        Celix::framework
        Celix::rsa_spi
        Celix::log_helper
)
target_include_directories(RsaConfiguredDiscovery PRIVATE src)
target_compile_options(RsaConfiguredDiscovery PRIVATE -std=c++17)
install_celix_bundle(RsaConfiguredDiscovery EXPORT celix COMPONENT rsa)
add_library(Celix::RsaConfiguredDiscovery ALIAS RsaConfiguredDiscovery)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif()
