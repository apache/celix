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

add_celix_bundle(celix_pubsub_topology_manager
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_topology_manager"
    VERSION "1.1.0"
    GROUP "Celix/PubSub"
    SOURCES
        src/pstm_activator.c
        src/pubsub_topology_manager.c
        src/pubsub_topology_manager.h
)
target_link_libraries(celix_pubsub_topology_manager PRIVATE Celix::framework Celix::log_helper Celix::shell_api)
target_link_libraries(celix_pubsub_topology_manager PRIVATE Celix::pubsub_spi Celix::pubsub_utils )
target_link_libraries(celix_pubsub_topology_manager PRIVATE libuuid::libuuid)

install_celix_bundle(celix_pubsub_topology_manager EXPORT celix COMPONENT pubsub)

add_library(Celix::celix_pubsub_topology_manager ALIAS celix_pubsub_topology_manager)

