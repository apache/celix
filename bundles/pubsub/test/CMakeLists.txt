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

find_package(CppUTest REQUIRED)
find_package(Jansson REQUIRED)

add_celix_bundle(pubsub_endpoint_sut
        #"Vanilla" bundle which is under test
        SOURCES
        test/sut_endpoint_activator.c
        VERSION 1.0.0
        )
target_include_directories(pubsub_endpoint_sut PRIVATE test)
target_link_libraries(pubsub_endpoint_sut PRIVATE Celix::pubsub_api)
celix_bundle_files(pubsub_endpoint_sut
        meta_data/msg.descriptor
        DESTINATION "META-INF/descriptors"
        )
celix_bundle_files(pubsub_endpoint_sut
        meta_data/ping2.properties
        DESTINATION "META-INF/topics/pub"
        )

add_celix_bundle(pubsub_endpoint_tst
        #Test bundle containing cpputests and uses celix_test_runner launcher instead of the celix launcher
        SOURCES
        test/tst_endpoint_activator.c
        VERSION 1.0.0
        )
target_link_libraries(pubsub_endpoint_tst PRIVATE Celix::framework Celix::pubsub_api)
celix_bundle_files(pubsub_endpoint_tst
        meta_data/msg.descriptor
        DESTINATION "META-INF/descriptors"
        )
celix_bundle_files(pubsub_endpoint_tst
        meta_data/ping2.properties
        DESTINATION "META-INF/topics/sub"
        )


add_celix_bundle(pubsub_loopback
        #"Vanilla" bundle which is under test
        SOURCES
        test/loopback_activator.c
        VERSION 1.0.0
        )
target_include_directories(pubsub_loopback PRIVATE test)
target_link_libraries(pubsub_loopback PRIVATE Celix::pubsub_api)
celix_bundle_files(pubsub_loopback
        meta_data/msg.descriptor
        DESTINATION "META-INF/descriptors"
        )
celix_bundle_files(pubsub_loopback
        meta_data/pong2.properties
        DESTINATION "META-INF/topics/pub"
        )
celix_bundle_files(pubsub_loopback
        meta_data/pong2.properties
        DESTINATION "META-INF/topics/sub"
        )

add_celix_bundle(pubsub_sut
    #"Vanilla" bundle which is under test
    SOURCES
        test/sut_activator.c
    VERSION 1.0.0
)
target_include_directories(pubsub_sut PRIVATE test)
target_link_libraries(pubsub_sut PRIVATE Celix::pubsub_api)
celix_bundle_files(pubsub_sut
    meta_data/msg.descriptor
    DESTINATION "META-INF/descriptors"
)
celix_bundle_files(pubsub_sut
    meta_data/ping.properties
    DESTINATION "META-INF/topics/pub"
)

add_celix_bundle(pubsub_tst
    #Test bundle containing cpputests and uses celix_test_runner launcher instead of the celix launcher
    SOURCES
        test/tst_activator.c
    VERSION 1.0.0
)
target_link_libraries(pubsub_tst PRIVATE Celix::framework Celix::pubsub_api)
celix_bundle_files(pubsub_tst
    meta_data/msg.descriptor
    DESTINATION "META-INF/descriptors"
)
celix_bundle_files(pubsub_tst
    meta_data/ping.properties
    DESTINATION "META-INF/topics/sub"
)

add_celix_bundle(pubsub_deadlock_sut
    #"Vanilla" bundle which is used to trigger a publisher added call
    SOURCES
    test/sut_activator.c
    VERSION 1.0.0
)
celix_bundle_files(pubsub_deadlock_sut
    meta_data/msg.descriptor
    DESTINATION "META-INF/descriptors"
)
celix_bundle_files(pubsub_deadlock_sut
    meta_data/deadlock.scope.properties
    DESTINATION "META-INF/topics/pub"
)
celix_bundle_files(pubsub_deadlock_sut
    meta_data/deadlock.scope2.properties
    DESTINATION "META-INF/topics/pub"
)
target_link_libraries(pubsub_deadlock_sut PRIVATE Celix::pubsub_api)

celix_get_bundle_file(pubsub_deadlock_sut DEADLOCK_SUT_BUNDLE_FILE)

if (BUILD_PUBSUB_PSA_UDP_MC)
    add_celix_container(pubsub_udpmc_tests
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
                Celix::pubsub_serializer_json
                Celix::pubsub_topology_manager
                Celix::pubsub_admin_udp_multicast
                pubsub_sut
                pubsub_tst
    )
    target_link_libraries(pubsub_udpmc_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi)
    target_include_directories(pubsub_udpmc_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)

    add_celix_container(pstm_deadlock_udpmc_test
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
            Celix::pubsub_serializer_json
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            Celix::pubsub_protocol_wire_v2
            Celix::shell
            Celix::shell_tui
            )
    target_compile_definitions(pstm_deadlock_udpmc_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
    target_link_libraries(pstm_deadlock_udpmc_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pstm_deadlock_udpmc_test SYSTEM PRIVATE pstm_deadlock_udpmc_test)

    #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
    add_dependencies(pstm_deadlock_udpmc_test pubsub_deadlock_sut_bundle)

    #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_udpmc_test/META-INF/descriptors/msg.descriptor COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_udpmc_test/META-INF/topics/pub/deadlock.properties COPYONLY)

    add_test(NAME pstm_deadlock_udpmc_test COMMAND pstm_deadlock_udpmc_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_udpmc_test,CONTAINER_LOC>)
    setup_target_for_coverage(pstm_deadlock_udpmc_test SCAN_DIR ..)

    #TODO fix issues with UDPMC and reanble test again
    #add_test(NAME pubsub_udpmc_tests COMMAND pubsub_udpmc_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_udpmc_tests,CONTAINER_LOC>)
    #setup_target_for_coverage(pubsub_udpmc_tests SCAN_DIR ..)
endif()

if (BUILD_PUBSUB_PSA_TCP)
    add_celix_container(pubsub_tcp_tests
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_tcp
            Celix::pubsub_protocol_wire_v2
            pubsub_sut
            pubsub_tst
            )
    target_link_libraries(pubsub_tcp_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi)
    target_include_directories(pubsub_tcp_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)
    add_test(NAME pubsub_tcp_tests COMMAND pubsub_tcp_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_tcp_tests,CONTAINER_LOC>)
    setup_target_for_coverage(pubsub_tcp_tests SCAN_DIR ..)


    add_celix_container(pubsub_tcp_endpoint_tests
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_endpoint_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::pubsub_serializer_json
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_tcp
            Celix::pubsub_protocol_wire_v2
            pubsub_loopback
            pubsub_endpoint_sut
            pubsub_endpoint_tst
            )
    target_link_libraries(pubsub_tcp_endpoint_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi)
    target_include_directories(pubsub_tcp_endpoint_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)

    add_celix_container(pstm_deadlock_tcp_test
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
            Celix::pubsub_serializer_json
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            Celix::pubsub_protocol_wire_v2
            Celix::shell
            Celix::shell_tui
            )
    target_compile_definitions(pstm_deadlock_tcp_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
    target_link_libraries(pstm_deadlock_tcp_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pstm_deadlock_tcp_test SYSTEM PRIVATE pstm_deadlock_tcp_test)

    #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
    add_dependencies(pstm_deadlock_tcp_test pubsub_deadlock_sut_bundle)

    #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_tcp_test/META-INF/descriptors/msg.descriptor COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_tcp_test/META-INF/topics/pub/deadlock.properties COPYONLY)

    add_test(NAME pstm_deadlock_tcp_test COMMAND pstm_deadlock_tcp_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_tcp_test,CONTAINER_LOC>)
    setup_target_for_coverage(pstm_deadlock_tcp_test SCAN_DIR ..)

    #TCP Endpoint test is disabled because the test is not stable when running on Travis
    if (ENABLE_PUBSUB_PSA_TCP_ENDPOINT_TEST)
        add_test(NAME pubsub_tcp_endpoint_tests COMMAND pubsub_tcp_endpoint_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_tcp_endpoint_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_tcp_endpoint_tests SCAN_DIR ..)
    endif()
endif()

if (BUILD_PUBSUB_PSA_WS)
    add_celix_container(pubsub_websocket_tests
            USE_CONFIG
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                USE_WEBSOCKETS=true
                LISTENING_PORTS=8080
            BUNDLES
                Celix::pubsub_serializer_json
                Celix::http_admin
                Celix::pubsub_topology_manager
                Celix::pubsub_admin_websocket
                pubsub_sut
                pubsub_tst
    )
    target_link_libraries(pubsub_websocket_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi)
    target_include_directories(pubsub_websocket_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)
    add_test(NAME pubsub_websocket_tests COMMAND pubsub_websocket_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_websocket_tests,CONTAINER_LOC>)
    setup_target_for_coverage(pubsub_websocket_tests SCAN_DIR ..)

    add_celix_container(pstm_deadlock_websocket_test
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
            Celix::pubsub_serializer_json
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            Celix::pubsub_protocol_wire_v2
            Celix::shell
            Celix::shell_tui
            )
    target_compile_definitions(pstm_deadlock_websocket_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
    target_link_libraries(pstm_deadlock_websocket_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pstm_deadlock_websocket_test SYSTEM PRIVATE pstm_deadlock_websocket_test)

    #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
    add_dependencies(pstm_deadlock_websocket_test pubsub_deadlock_sut_bundle)

    #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_websocket_test/META-INF/descriptors/msg.descriptor COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_websocket_test/META-INF/topics/pub/deadlock.properties COPYONLY)

    add_test(NAME pstm_deadlock_websocket_test COMMAND pstm_deadlock_websocket_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_websocket_test,CONTAINER_LOC>)
    setup_target_for_coverage(pstm_deadlock_websocket_test SCAN_DIR ..)
endif()

if (BUILD_PUBSUB_PSA_ZMQ)
    find_package(ZMQ REQUIRED)
    find_package(CZMQ REQUIRED)

    add_celix_container(pubsub_zmq_tests
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
                Celix::pubsub_serializer_json
                Celix::pubsub_topology_manager
                Celix::pubsub_admin_zmq
                Celix::pubsub_protocol_wire_v1
                pubsub_sut
                pubsub_tst
    )

    target_link_libraries(pubsub_zmq_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pubsub_zmq_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)
    add_test(NAME pubsub_zmq_tests COMMAND pubsub_zmq_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_tests,CONTAINER_LOC>)
    setup_target_for_coverage(pubsub_zmq_tests SCAN_DIR ..)

    add_celix_container(pubsub_zmq_wire_v2_tests
        USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
        LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
        DIR ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
        LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
        BUNDLES
        Celix::pubsub_serializer_json
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_zmq
        Celix::pubsub_protocol_wire_v2
        pubsub_sut
        pubsub_tst
        )

    target_link_libraries(pubsub_zmq_wire_v2_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pubsub_zmq_wire_v2_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)
    add_test(NAME pubsub_zmq_wire_v2_tests COMMAND pubsub_zmq_wire_v2_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_wire_v2_tests,CONTAINER_LOC>)
    setup_target_for_coverage(pubsub_zmq_wire_v2_tests SCAN_DIR ..)


    add_celix_container(pubsub_zmq_zerocopy_tests
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                PSA_ZMQ_ZEROCOPY_ENABLED=true
            BUNDLES
                Celix::pubsub_serializer_json
                Celix::pubsub_topology_manager
                Celix::pubsub_admin_zmq
                Celix::pubsub_protocol_wire_v1
                Celix::shell
                Celix::shell_tui
                pubsub_sut
                pubsub_tst
    )
    target_link_libraries(pubsub_zmq_zerocopy_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pubsub_zmq_zerocopy_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)

    add_test(NAME pubsub_zmq_zerocopy_tests COMMAND pubsub_zmq_zerocopy_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_zerocopy_tests,CONTAINER_LOC>)
    setup_target_for_coverage(pubsub_zmq_zerocopy_tests SCAN_DIR ..)

    add_celix_container(pubsub_zmq_zerocopy_wire_v2_tests
        USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
        LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
        DIR ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
        LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
        PSA_ZMQ_ZEROCOPY_ENABLED=true
        BUNDLES
        Celix::pubsub_serializer_json
        Celix::pubsub_topology_manager
        Celix::pubsub_admin_zmq
        Celix::pubsub_protocol_wire_v2
        Celix::shell
        Celix::shell_tui
        pubsub_sut
        pubsub_tst
        )
    target_link_libraries(pubsub_zmq_zerocopy_wire_v2_tests PRIVATE Celix::pubsub_api ${CppUTest_LIBRARIES} Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pubsub_zmq_zerocopy_wire_v2_tests SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR} test)

    add_test(NAME pubsub_zmq_zerocopy_wire_v2_tests COMMAND pubsub_zmq_zerocopy_wire_v2_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_zerocopy_wire_v2_tests,CONTAINER_LOC>)
    setup_target_for_coverage(pubsub_zmq_zerocopy_wire_v2_tests SCAN_DIR ..)

    add_celix_container(pstm_deadlock_zmq_test
            USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
            LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
            DIR ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
            BUNDLES
            Celix::pubsub_serializer_json
            Celix::pubsub_topology_manager
            Celix::pubsub_admin_zmq
            Celix::pubsub_protocol_wire_v2
            Celix::shell
            Celix::shell_tui
            )
    target_compile_definitions(pstm_deadlock_zmq_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
    target_link_libraries(pstm_deadlock_zmq_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main Jansson Celix::dfi ZMQ::lib CZMQ::lib)
    target_include_directories(pstm_deadlock_zmq_test SYSTEM PRIVATE pstm_deadlock_zmq_test)

    #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
    add_dependencies(pstm_deadlock_zmq_test pubsub_deadlock_sut_bundle)

    #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_zmq_test/META-INF/descriptors/msg.descriptor COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_zmq_test/META-INF/topics/pub/deadlock.properties COPYONLY)

    add_test(NAME pstm_deadlock_zmq_test COMMAND pstm_deadlock_zmq_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_zmq_test,CONTAINER_LOC>)
    setup_target_for_coverage(pstm_deadlock_zmq_test SCAN_DIR ..)
endif ()
