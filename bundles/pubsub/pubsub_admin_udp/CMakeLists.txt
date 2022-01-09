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

find_package(UUID REQUIRED)

add_celix_bundle(celix_pubsub_admin_udp
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_udp"
    VERSION "2.0.0"
    GROUP "Celix/PubSub"
    SOURCES
        src/psa_activator.c
        src/pubsub_udp_admin.c
        src/pubsub_udp_topic_sender.c
        src/pubsub_udp_topic_receiver.c
)

target_link_libraries(celix_pubsub_admin_udp PRIVATE Celix::pubsub_spi Celix::pubsub_utils)
target_link_libraries(celix_pubsub_admin_udp PRIVATE Celix::framework Celix::log_helper)
target_link_libraries(celix_pubsub_admin_udp PRIVATE Celix::shell_api)
target_include_directories(celix_pubsub_admin_udp PRIVATE src)
# cmake find package UUID set the wrong include dir for OSX
if (NOT APPLE)
    target_link_libraries(celix_pubsub_admin_udp PRIVATE UUID::lib)
endif()

install_celix_bundle(celix_pubsub_admin_udp EXPORT celix COMPONENT pubsub)
add_library(Celix::celix_pubsub_admin_udp ALIAS celix_pubsub_admin_udp)
