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

celix_subproject(PUBSUB_PSA_UDP_MC "Build UDP MC PubSub Admin" ON)
if (PUBSUB_PSA_UDP_MC)
    find_package(jansson REQUIRED)

    add_celix_bundle(celix_pubsub_admin_udp_multicast
            BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_udp_multicast"
            VERSION "1.0.0"
            GROUP "Celix/PubSub"
            SOURCES
            src/psa_activator.c
            src/pubsub_udpmc_admin.c
            src/pubsub_udpmc_topic_sender.c
            src/pubsub_udpmc_topic_receiver.c
            src/pubsub_udpmc_common.c
            src/large_udp.c
            )
    target_include_directories(celix_pubsub_admin_udp_multicast PRIVATE
            src
            )
    set_target_properties(celix_pubsub_admin_udp_multicast PROPERTIES INSTALL_RPATH "$ORIGIN")
    target_link_libraries(celix_pubsub_admin_udp_multicast PRIVATE Celix::framework Celix::dfi Celix::log_helper Celix::utils Celix::shell_api)
    target_link_libraries(celix_pubsub_admin_udp_multicast PRIVATE Celix::pubsub_spi Celix::pubsub_utils )
    install_celix_bundle(celix_pubsub_admin_udp_multicast EXPORT celix COMPONENT pubsub)

    add_library(Celix::celix_pubsub_admin_udp_multicast ALIAS celix_pubsub_admin_udp_multicast)
endif (PUBSUB_PSA_UDP_MC)


