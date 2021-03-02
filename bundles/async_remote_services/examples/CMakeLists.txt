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

add_celix_bundle(hardcoded_example_provider
        SOURCES
        HardcodedExampleProvider.cc
        HardcodedExampleSerializer.cc
        SYMBOLIC_NAME "apache_celix_hardcoded_example_service_provider"
        VERSION 0.0.1
        )
add_celix_bundle(hardcoded_example_subscriber
        SOURCES
        HardcodedExampleSubscriber.cc
        HardcodedExampleSerializer.cc
        SYMBOLIC_NAME "apache_celix_hardcoded_example_service_subscriber"
        VERSION 0.0.1
        )

celix_bundle_files(hardcoded_example_provider
        ${CMAKE_CURRENT_SOURCE_DIR}/pubsub_properties/IHardcodedService.properties
        DESTINATION "META-INF/topics/pub"
        )
celix_bundle_files(hardcoded_example_provider
        ${CMAKE_CURRENT_SOURCE_DIR}/pubsub_properties/IHardcodedService.properties
        DESTINATION "META-INF/topics/sub"
        )

celix_bundle_files(hardcoded_example_subscriber
        ${CMAKE_CURRENT_SOURCE_DIR}/pubsub_properties/IHardcodedService.properties
        DESTINATION "META-INF/topics/pub"
        )
celix_bundle_files(hardcoded_example_subscriber
        ${CMAKE_CURRENT_SOURCE_DIR}/pubsub_properties/IHardcodedService.properties
        DESTINATION "META-INF/topics/sub"
        )

target_link_libraries(hardcoded_example_provider PRIVATE Celix::framework Celix::async_rsa_discovery_configured Celix::async_rsa_common Celix::async_rsa)
target_link_libraries(hardcoded_example_subscriber PRIVATE Celix::framework Celix::async_rsa_discovery_configured Celix::async_rsa_common Celix::async_rsa)

set(PUBSUB_CONTAINER_LIBS
        Jansson
        ZMQ::lib
        CZMQ::lib
        Celix::pubsub_api
        Celix::pubsub_spi
        Celix::async_rsa_common
        Celix::Promises)

find_package(ZMQ REQUIRED)
find_package(CZMQ REQUIRED)
find_package(Jansson REQUIRED)
add_celix_container(hardcoded_example_provider_cnt
        GROUP async_rsa
        BUNDLES
        Celix::log_admin
        Celix::shell
        Celix::shell_tui
        Celix::pubsub_serializer_json
        Celix::pubsub_discovery_etcd
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_zmq_v2
        Celix::pubsub_protocol_wire_v2
        Celix::async_rsa_discovery_configured
        Celix::async_topology_manager
        Celix::async_rsa
        hardcoded_example_provider
        PROPERTIES
        "org.osgi.framework.storage=.publisherCache"
        "CELIX_ASYNC_RSA_CONFIGURED_DISCOVERY_FILE=/home/oipo-unencrypted/Programming/celix/bundles/async_remote_services/examples/discovery/endpoint.json"
        )
target_link_libraries(hardcoded_example_provider PRIVATE ${PUBSUB_CONTAINER_LIBS})

add_celix_container(hardcoded_example_subscriber_cnt
        GROUP async_rsa
        BUNDLES
        Celix::log_admin
        Celix::shell
        Celix::shell_tui
        Celix::pubsub_serializer_json
        Celix::pubsub_discovery_etcd
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_zmq_v2
        Celix::pubsub_protocol_wire_v2
        Celix::async_rsa_discovery_configured
        Celix::async_topology_manager
        Celix::async_rsa
        hardcoded_example_subscriber
        PROPERTIES
        "org.osgi.framework.storage=.subscriberCache"
        "CELIX_ASYNC_RSA_CONFIGURED_DISCOVERY_FILE=/home/oipo-unencrypted/Programming/celix/bundles/async_remote_services/examples/discovery/endpoint.json"
        )
target_link_libraries(hardcoded_example_subscriber PRIVATE ${PUBSUB_CONTAINER_LIBS})
