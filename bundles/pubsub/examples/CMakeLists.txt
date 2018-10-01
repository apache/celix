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

add_subdirectory(pubsub)
add_subdirectory(mp_pubsub)

find_program(ETCD_CMD NAMES etcd)
find_program(XTERM_CMD NAMES xterm)

find_package(ZMQ REQUIRED)
find_package(CZMQ REQUIRED)
find_package(Jansson REQUIRED)

set(PUBSUB_CONTAINER_LIBS ${JANSSON_LIBRARY} ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY} Celix::dfi)

# UDP Multicast
add_celix_container(pubsub_publisher_udp_mc
    GROUP pubsub
    BUNDLES
        Celix::log_service
        Celix::shell
        Celix::shell_tui
        Celix::pubsub_serializer_json
        Celix::pubsub_discovery_etcd
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_udp_multicast
        celix_pubsub_poi_publisher
        celix_pubsub_poi_publisher2
    PROPERTIES
        PSA_UDPMC_VERBOSE=true
        PUBSUB_ETCD_DISCOVERY_VERBOSE=true
        PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
)
target_link_libraries(pubsub_publisher_udp_mc PRIVATE ${PUBSUB_CONTAINER_LIBS})

add_celix_container(pubsub_subscriber_udp_mc
    GROUP "pubsub"
    BUNDLES
        Celix::log_service
        Celix::shell
        Celix::shell_tui
        Celix::pubsub_serializer_json
        Celix::pubsub_discovery_etcd
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_udp_multicast
        celix_pubsub_poi_subscriber
    PROPERTIES
        PSA_UDPMC_VERBOSE=true
        PUBSUB_ETCD_DISCOVERY_VERBOSE=true
        PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
)
target_link_libraries(pubsub_subscriber_udp_mc PRIVATE ${PUBSUB_CONTAINER_LIBS})


add_celix_container(pubsub_subscriber2_udp_mc
    GROUP "pubsub"
    BUNDLES
        Celix::log_service
        Celix::shell
        Celix::shell_tui
        Celix::pubsub_serializer_json
        Celix::pubsub_discovery_etcd
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_udp_multicast
        celix_pubsub_poi_subscriber
    PROPERTIES
        PSA_UDPMC_VERBOSE=true
        PUBSUB_ETCD_DISCOVERY_VERBOSE=true
        PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
)
target_link_libraries(pubsub_subscriber2_udp_mc PRIVATE ${PUBSUB_CONTAINER_LIBS})

if (ETCD_CMD AND XTERM_CMD)
    #Runtime starting a publish and subscriber for udp mc
    add_celix_runtime(pubsub_rt_upd_mc
        NAME udp_mc
        GROUP pubsub
        CONTAINERS
            pubsub_publisher_udp_mc
            pubsub_subscriber_udp_mc
            pubsub_subscriber2_udp_mc
        COMMANDS
            etcd
        USE_TERM
    )
endif ()

if (BUILD_PUBSUB_PSA_ZMQ)

    # Dynamic ZMQ / UDP admin
    add_celix_container(pubsub_publisher
        GROUP "pubsub"
        BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            Celix::pubsub_admin_udp_multicast
            celix_pubsub_poi_publisher
            celix_pubsub_poi_publisher2
    )
    target_link_libraries(pubsub_publisher PRIVATE ${PUBSUB_CONTAINER_LIBS})

    add_celix_container(pubsub_subscriber
            GROUP "pubsub"
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            Celix::pubsub_admin_udp_multicast
            celix_pubsub_poi_subscriber
    )
    target_link_libraries(pubsub_subscriber PRIVATE ${PUBSUB_CONTAINER_LIBS})

    # ZMQ
    add_celix_container("pubsub_zmq"
            GROUP "pubsub"
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            celix_pubsub_poi_publisher
            celix_pubsub_poi_subscriber
            )
    target_link_libraries(pubsub_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

    add_celix_container("pubsub_publisher_zmq"
         GROUP "pubsub"
        BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            celix_pubsub_poi_publisher
            celix_pubsub_poi_publisher2
        PROPERTIES
            PSA_ZMQ_VERBOSE=true
            PUBSUB_ETCD_DISCOVERY_VERBOSE=true
            PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
    )
    target_link_libraries(pubsub_publisher_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

    add_celix_container(pubsub_subscriber_zmq
        GROUP "pubsub"
        BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            celix_pubsub_poi_subscriber
        PROPERTIES
            PSA_ZMQ_VERBOSE=true
            PUBSUB_ETCD_DISCOVERY_VERBOSE=true
            PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
    )
    target_link_libraries(pubsub_subscriber_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

    add_celix_container(pubsub_subscriber2_zmq
        GROUP "pubsub"
        BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            celix_pubsub_poi_subscriber
        PROPERTIES
            PSA_ZMQ_VERBOSE=true
            PUBSUB_ETCD_DISCOVERY_VERBOSE=true
            PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
    )
    target_link_libraries(pubsub_subscriber2_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

    # ZMQ Multipart
    add_celix_container("pubsub_mp_subscriber_zmq"
            GROUP "pubsub"
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            org.apache.celix.pubsub_subscriber.MpSubscriber
            )
    target_link_libraries(pubsub_mp_subscriber_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

    add_celix_container("pubsub_mp_publisher_zmq"
            GROUP "pubsub"
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_discovery_etcd
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            org.apache.celix.pubsub_publisher.MpPublisher
            )
    target_link_libraries(pubsub_mp_publisher_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

    if (ETCD_CMD AND XTERM_CMD)
        #Runtime starting two bundles using both zmq and upd mc pubsub
        add_celix_runtime(pubsub_rt_zmq_udpmc_combi
            NAME combi
            GROUP pubsub
            CONTAINERS
                pubsub_publisher_zmq
                pubsub_subscriber_zmq
                pubsub_subscriber_zmq
            COMMANDS
                etcd
            USE_TERM
        )

        #Runtime starting a publish and 2 subscribers for zmq
        add_celix_runtime(pubsub_rt_zmq
            NAME zmq
            GROUP pubsub
            CONTAINERS
                pubsub_publisher
                pubsub_subscriber_zmq
                pubsub_subscriber2_zmq
            COMMANDS
                etcd
            USE_TERM
        )

        #Runtime starting a multipart (multiple message in one send) publish and subscriber for zmq
        add_celix_runtime(pubsub_rt_multipart_zmq
            NAME zmq_multipart
            GROUP pubsub
            CONTAINERS
                pubsub_mp_subscriber_zmq
                pubsub_mp_publisher_zmq
            COMMANDS
                etcd
            USE_TERM
        )
    endif ()

endif()
