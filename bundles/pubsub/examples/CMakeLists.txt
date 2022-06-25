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

celix_subproject(PUBSUB_EXAMPLES "Option to build the pubsub examples" ON DEPS LOG_SERVICE SHELL SHELL_TUI PUBSUB_DISCOVERY_ETCD)
if (PUBSUB_EXAMPLES)
    add_subdirectory(pubsub)

    find_program(ETCD_CMD NAMES etcd)
    find_program(XTERM_CMD NAMES xterm)

    find_package(jansson REQUIRED)

    set(PUBSUB_CONTAINER_LIBS jansson::jansson Celix::dfi)

    # UDP Multicast
    if (BUILD_PUBSUB_PSA_UDP_MC)
        add_celix_container(pubsub_publisher_udp_mc
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_udp_multicast
                celix_pubsub_poi_publisher
                celix_pubsub_poi_publisher2
                PROPERTIES
                PSA_UDPMC_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_publisher_udp_mc PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber_udp_mc
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_udp_multicast
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_UDPMC_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber_udp_mc PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber2_udp_mc
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_udp_multicast
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_UDPMC_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber2_udp_mc PRIVATE ${PUBSUB_CONTAINER_LIBS})

        if (ETCD_CMD AND XTERM_CMD)
            # Runtime starting a publish and subscriber for udp mc
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
    endif()

    if (BUILD_PUBSUB_PSA_TCP)
        # TCP
        add_celix_container(pubsub_publisher_tcp
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                Celix::celix_pubsub_protocol_wire_v2
                celix_pubsub_poi_publisher
                celix_pubsub_poi_publisher2
                PROPERTIES
                PSA_TCP_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_publisher_tcp PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber_tcp
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                Celix::celix_pubsub_protocol_wire_v2
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_TCP_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber_tcp PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber2_tcp
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                Celix::celix_pubsub_protocol_wire_v2
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_TCP_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber2_tcp PRIVATE ${PUBSUB_CONTAINER_LIBS})

        if (ETCD_CMD AND XTERM_CMD)
            # Runtime starting a publish and subscriber for tcp
            add_celix_runtime(pubsub_rt_tcp
                    NAME tcp
                    GROUP pubsub
                    CONTAINERS
                    pubsub_publisher_tcp
                    pubsub_subscriber_tcp
                    pubsub_subscriber2_tcp
                    COMMANDS
                    etcd
                    USE_TERM
                    )
        endif ()
    endif()

    if (BUILD_PUBSUB_PSA_ZMQ)
        find_package(ZeroMQ REQUIRED)
        find_package(czmq REQUIRED)

        set(PUBSUB_CONTAINER_LIBS ${PUBSUB_CONTAINER_LIBS} ZeroMQ::ZeroMQ czmq::czmq ${OPTIONAL_OPENSSL_LIB})

        if (BUILD_PUBSUB_PSA_TCP AND BUILD_PUBSUB_PSA_UDP_MC)
            # Dynamic ZMQ / UDP / TCP admin
            add_celix_container(pubsub_publisher
                    GROUP "pubsub"
                    BUNDLES
                    Celix::shell
                    Celix::shell_tui
                    Celix::celix_pubsub_serializer_json
                    Celix::celix_pubsub_discovery_etcd
                    Celix::celix_pubsub_topology_manager
                    Celix::celix_pubsub_admin_zmq
                    Celix::celix_pubsub_admin_udp_multicast
                    Celix::celix_pubsub_admin_tcp
                    Celix::celix_pubsub_protocol_wire_v1
                    celix_pubsub_poi_publisher
                    celix_pubsub_poi_publisher2
                    )
            target_link_libraries(pubsub_publisher PRIVATE ${PUBSUB_CONTAINER_LIBS})

            add_celix_container(pubsub_subscriber
                    GROUP "pubsub"
                    BUNDLES
                    Celix::shell
                    Celix::shell_tui
                    Celix::celix_pubsub_serializer_json
                    Celix::celix_pubsub_discovery_etcd
                    Celix::celix_pubsub_topology_manager
                    Celix::celix_pubsub_admin_zmq
                    Celix::celix_pubsub_admin_udp_multicast
                    Celix::celix_pubsub_admin_tcp
                    Celix::celix_pubsub_protocol_wire_v1
                    celix_pubsub_poi_subscriber
                    )
            target_link_libraries(pubsub_subscriber PRIVATE ${PUBSUB_CONTAINER_LIBS})
        endif()

        # ZMQ
        add_celix_container(pubsub_zmq
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                celix_pubsub_poi_publisher
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_ZMQ_ZEROCOPY_ENABLED=true
                )
        target_link_libraries(pubsub_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_publisher_zmq
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v1
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                celix_pubsub_poi_publisher
                celix_pubsub_poi_publisher2
                celix_pubsub_interceptors_example
                PROPERTIES
                PSA_ZMQ_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                PSA_ZMQ_ZEROCOPY_ENABLED=false
                )
        target_link_libraries(pubsub_publisher_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber_zmq
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v1
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                celix_pubsub_poi_subscriber
                celix_pubsub_interceptors_example
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
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_ZMQ_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber2_zmq PRIVATE ${PUBSUB_CONTAINER_LIBS})

        if (ETCD_CMD AND XTERM_CMD)
            # Runtime starting two bundles using both zmq and upd mc pubsub
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

            # Runtime starting a publish and 2 subscribers for zmq
            add_celix_runtime(pubsub_rt_zmq
                    NAME zmq
                    GROUP pubsub
                    CONTAINERS
                    pubsub_publisher_zmq
                    pubsub_subscriber_zmq
                    pubsub_subscriber2_zmq
                    COMMANDS
                    etcd
                    USE_TERM
                    )
        endif ()
    endif()

    if (BUILD_PUBSUB_PSA_NANOMSG)
        set(PUBSUB_CONTAINER_LIBS NANOMSG::lib ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_publisher1_nanomsg
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_nanomsg
                celix_pubsub_poi_publisher
                PROPERTIES
                PSA_NANOMSG_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_publisher1_nanomsg PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_publisher2_nanomsg
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_nanomsg
                celix_pubsub_poi_publisher
                PROPERTIES
                PSA_NANOMSG_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_publisher2_nanomsg PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber1_nanomsg
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_nanomsg
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_NANOMSG_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber1_nanomsg PRIVATE ${PUBSUB_CONTAINER_LIBS})

        add_celix_container(pubsub_subscriber2_nanomsg
                GROUP "pubsub"
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_discovery_etcd
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_nanomsg
                celix_pubsub_poi_subscriber
                PROPERTIES
                PSA_NANOMSG_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                )
        target_link_libraries(pubsub_subscriber2_nanomsg PRIVATE ${PUBSUB_CONTAINER_LIBS})

        if (ETCD_CMD AND XTERM_CMD)
            # Runtime starting a publisher and 2 subscribers for nanomsg
            add_celix_runtime(pubsub_rt_nanomsg
                    NAME nanomsg
                    GROUP pubsub
                    CONTAINERS
                    pubsub_publisher1_nanomsg
                    pubsub_publisher2_nanomsg
                    pubsub_subscriber1_nanomsg
                    pubsub_subscriber2_nanomsg
                    COMMANDS
                    etcd
                    USE_TERM
                    )
        endif ()

    endif()

    if (BUILD_PUBSUB_PSA_WS)
        add_celix_container(pubsub_websocket_example
                GROUP pubsub
                BUNDLES
                Celix::log_admin
                Celix::shell
                Celix::shell_tui
                Celix::http_admin
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_websocket
                celix_pubsub_websocket_example
                PROPERTIES
                PSA_TCP_VERBOSE=true
                PUBSUB_ETCD_DISCOVERY_VERBOSE=true
                PUBSUB_TOPOLOGY_MANAGER_VERBOSE=true
                CELIX_HTTP_ADMIN_LISTENING_PORTS=7660
                CELIX_HTTP_ADMIN_NUM_THREADS=5
                )
        target_link_libraries(pubsub_websocket_example PRIVATE ${PUBSUB_CONTAINER_LIBS})
    endif()
endif ()
