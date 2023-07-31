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

celix_subproject(PUBSUB "Option to build the pubsub bundles" ON)
if (PUBSUB)
    add_subdirectory(pubsub_api)
    add_subdirectory(pubsub_utils)
    add_subdirectory(pubsub_spi)
    add_subdirectory(pubsub_topology_manager)
    add_subdirectory(pubsub_admin_zmq)
    add_subdirectory(pubsub_admin_tcp)
    add_subdirectory(pubsub_admin_udp_mc)
    add_subdirectory(pubsub_admin_websocket)
    add_subdirectory(pubsub_discovery)
    add_subdirectory(pubsub_serializer_json)
    add_subdirectory(pubsub_serializer_avrobin)
    add_subdirectory(pubsub_protocol)
    add_subdirectory(keygen)
    add_subdirectory(examples)
    if (ENABLE_TESTING)
         add_subdirectory(integration)
    endif()
endif(PUBSUB)
