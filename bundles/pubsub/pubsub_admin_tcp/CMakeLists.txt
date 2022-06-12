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

celix_subproject(PUBSUB_PSA_TCP "Build TCP PubSub Admin" ON)
if (PUBSUB_PSA_TCP)
    find_package(libuuid REQUIRED)

    add_celix_bundle(celix_pubsub_admin_tcp
            BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_tcp"
            VERSION "2.0.0"
            GROUP "Celix/PubSub"
            SOURCES
            src/psa_activator.c
            src/pubsub_tcp_admin.c
            src/pubsub_tcp_topic_sender.c
            src/pubsub_tcp_topic_receiver.c
            src/pubsub_tcp_handler.c
            src/pubsub_tcp_common.c
            )

    target_link_libraries(celix_pubsub_admin_tcp PRIVATE Celix::pubsub_spi Celix::pubsub_utils)
    target_link_libraries(celix_pubsub_admin_tcp PRIVATE Celix::framework Celix::log_helper)
    target_link_libraries(celix_pubsub_admin_tcp PRIVATE Celix::shell_api)
    target_include_directories(celix_pubsub_admin_tcp PRIVATE src)
    # cmake find package UUID set the wrong include dir for OSX, does Conan solve this?
    if (NOT APPLE)
        target_link_libraries(celix_pubsub_admin_tcp PRIVATE libuuid::libuuid)
    endif()

    install_celix_bundle(celix_pubsub_admin_tcp EXPORT celix COMPONENT pubsub)
    add_library(Celix::celix_pubsub_admin_tcp ALIAS celix_pubsub_admin_tcp)
endif (PUBSUB_PSA_TCP)
