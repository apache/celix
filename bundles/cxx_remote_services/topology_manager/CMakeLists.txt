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

add_celix_bundle(async_topology_manager
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_async_topology_manager"
        NAME "Apache Celix Async Topology Manager"
        GROUP "Celix/AsyncRSA"
        SOURCES
        src/topology_manager.cc
        )
target_include_directories(async_topology_manager PRIVATE include)
target_link_libraries(async_topology_manager PRIVATE
        Celix::async_rsa_common
        Celix::async_rsa_discovery_configured
        Celix::framework
        Celix::log_helper
)

#if (ENABLE_TESTING)
#    add_subdirectory(gtest)
#endif()


install_celix_bundle(async_topology_manager EXPORT celix COMPONENT async_rsa)
#Setup target aliases to match external usage
add_library(Celix::async_topology_manager ALIAS async_topology_manager)