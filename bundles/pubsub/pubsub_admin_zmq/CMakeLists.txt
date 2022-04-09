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

celix_subproject(PUBSUB_PSA_ZMQ "Build ZeroMQ PubSub Admin (LGPL License)" ON)
if (PUBSUB_PSA_ZMQ)
    find_package(ZeroMQ REQUIRED)
    find_package(czmq REQUIRED)
    find_package(libuuid REQUIRED)

    set(OPTIONAL_OPENSSL_LIB )
    # FIXME: ZMQ_SECURITY doesn't compile now
    option(BUILD_ZMQ_SECURITY "Build with security for ZeroMQ." OFF)
    if (BUILD_ZMQ_SECURITY)
        add_definitions(-DBUILD_WITH_ZMQ_SECURITY=1)

        find_package(OpenSSL 1.1.0 REQUIRED)
        include_directories("${OPENSSL_INCLUDE_DIR}")
        set(OPTIONAL_OPENSSL_LIB OpenSSL::SSL)

        set (ZMQ_CRYPTO_C "src/zmq_crypto.c")
    endif()

    add_celix_bundle(celix_pubsub_admin_zmq
            BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_zmq"
            VERSION "2.0.0"
            GROUP "Celix/PubSub"
            SOURCES
            src/psa_activator.c
            src/pubsub_zmq_admin.c
            src/pubsub_zmq_topic_sender.c
            src/pubsub_zmq_topic_receiver.c
            ${ZMQ_CRYPTO_C}
            )

    target_link_libraries(celix_pubsub_admin_zmq PRIVATE
            Celix::framework Celix::dfi Celix::log_helper Celix::utils
            ZeroMQ::ZeroMQ czmq::czmq ${OPTIONAL_OPENSSL_LIB})
    target_link_libraries(celix_pubsub_admin_zmq PRIVATE Celix::shell_api)
    target_link_libraries(celix_pubsub_admin_zmq PRIVATE Celix::pubsub_spi Celix::pubsub_utils)
    target_include_directories(celix_pubsub_admin_zmq PRIVATE src)
    install_celix_bundle(celix_pubsub_admin_zmq EXPORT celix COMPONENT pubsub)
    add_library(Celix::celix_pubsub_admin_zmq ALIAS celix_pubsub_admin_zmq)
endif (PUBSUB_PSA_ZMQ)
