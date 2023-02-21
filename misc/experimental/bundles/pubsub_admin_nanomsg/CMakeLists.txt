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

if (BUILD_PUBSUB_PSA_NANOMSG)

    find_package(NanoMsg REQUIRED)

    add_celix_bundle(celix_pubsub_admin_nanomsg
        BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_nanomsg"
        VERSION "1.0.0"
        GROUP "Celix/PubSub"
        SOURCES
            src/psa_nanomsg_activator.cc
            src/pubsub_nanomsg_admin.cc
            src/pubsub_nanomsg_topic_sender.cc
            src/pubsub_nanomsg_topic_receiver.cc
            src/pubsub_nanomsg_common.cc
    )

    set_target_properties(celix_pubsub_admin_nanomsg PROPERTIES INSTALL_RPATH "$ORIGIN")
    target_link_libraries(celix_pubsub_admin_nanomsg PRIVATE
            Celix::pubsub_spi
            Celix::framework Celix::dfi Celix::log_helper
            NANOMSG::lib
    )
    target_include_directories(celix_pubsub_admin_nanomsg PRIVATE
        src
    )

    install_celix_bundle(celix_pubsub_admin_nanomsg EXPORT celix COMPONENT pubsub)
    target_link_libraries(celix_pubsub_admin_nanomsg PRIVATE Celix::shell_api)
    add_library(Celix::pubsub_admin_nanomsg ALIAS celix_pubsub_admin_nanomsg)
endif()
