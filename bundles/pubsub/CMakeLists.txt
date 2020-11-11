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

celix_subproject(PUBSUB "Option to build the pubsub bundles" ON DEPS FRAMEWORK)
if (PUBSUB)

    option(BUILD_PUBSUB_PSA_ZMQ "Build ZeroMQ PubSub Admin (LGPL License)" ON)
    if (BUILD_PUBSUB_PSA_ZMQ)
        option(BUILD_ZMQ_SECURITY "Build with security for ZeroMQ." OFF)
        add_subdirectory(pubsub_admin_zmq/v1)
        add_subdirectory(pubsub_admin_zmq/v2)
    endif (BUILD_PUBSUB_PSA_ZMQ)

    option(BUILD_PUBSUB_PSA_TCP "Build TCP PubSub Admin" ON)
    if (BUILD_PUBSUB_PSA_TCP)
        add_subdirectory(pubsub_admin_tcp)
    endif (BUILD_PUBSUB_PSA_TCP)

    option(BUILD_PUBSUB_PSA_UDP_MC "Build UDP MC PubSub Admin" ON)
    if (BUILD_PUBSUB_PSA_UDP_MC)
        add_subdirectory(pubsub_admin_udp_mc)
    endif (BUILD_PUBSUB_PSA_UDP_MC)

    option(BUILD_PUBSUB_PSA_WS "Build WebSocket PubSub Admin" ON)
    if (BUILD_PUBSUB_PSA_WS)
        add_subdirectory(pubsub_admin_websocket)
    endif (BUILD_PUBSUB_PSA_WS)

    add_subdirectory(pubsub_api)
    add_subdirectory(pubsub_utils)
    add_subdirectory(pubsub_spi)
    add_subdirectory(pubsub_topology_manager)
    add_subdirectory(pubsub_discovery)
    add_subdirectory(pubsub_serializer_json)
    add_subdirectory(pubsub_serializer_avrobin)
    add_subdirectory(pubsub_protocol)
    add_subdirectory(keygen)
    add_subdirectory(mock)

    add_subdirectory(examples)

    if (ENABLE_TESTING)
        add_subdirectory(test)
    endif()

endif(PUBSUB)
