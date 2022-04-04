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

celix_subproject(PUBSUB_INTEGRATION "Option to build the pubsub integration" ON DEPS SHELL SHELL_TUI)
if (PUBSUB_INTEGRATION)
    find_package(jansson REQUIRED)

    add_celix_bundle(pubsub_endpoint_sut
            #"Vanilla" bundle which is under test
            SOURCES
            gtest/sut_endpoint_activator.c
            VERSION 1.0.0
            )
    target_include_directories(pubsub_endpoint_sut PRIVATE gtest)
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
            gtest/tst_endpoint_activator.c
            VERSION 1.0.0
            )
    target_link_libraries(pubsub_endpoint_tst PRIVATE Celix::framework Celix::pubsub_api)
    celix_bundle_files(pubsub_endpoint_tst
            meta_data/msg.descriptor
            DESTINATION "META-INF/descriptors"
            )
    celix_bundle_files(pubsub_endpoint_tst
            meta_data/pong3.properties
            DESTINATION "META-INF/topics/sub"
            )


    add_celix_bundle(pubsub_loopback
            #"Vanilla" bundle which is under test
            SOURCES
            gtest/loopback_activator.c
            VERSION 1.0.0
            )
    target_include_directories(pubsub_loopback PRIVATE gtest)
    target_link_libraries(pubsub_loopback PRIVATE Celix::pubsub_api)
    celix_bundle_files(pubsub_loopback
            meta_data/msg.descriptor
            DESTINATION "META-INF/descriptors"
            )
    celix_bundle_files(pubsub_loopback
            meta_data/ping3.properties
            DESTINATION "META-INF/topics/pub"
            )
    celix_bundle_files(pubsub_loopback
            meta_data/pong2.properties
            DESTINATION "META-INF/topics/sub"
            )

    add_celix_bundle(pubsub_sut
            #"Vanilla" bundle which is under test
            SOURCES
            gtest/sut_activator.c
            VERSION 1.0.0
            )
    target_include_directories(pubsub_sut PRIVATE gtest)
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
            gtest/tst_activator.c
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
            gtest/sut_activator.c
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

    add_celix_bundle(pubsub_serializer
            #serializer bundle
            SOURCES
            gtest/serializer_activator.cc
            VERSION 1.0.0
            )
    target_include_directories(pubsub_serializer PRIVATE gtest)
    target_link_libraries(pubsub_serializer PRIVATE Celix::pubsub_api Celix::pubsub_spi)

    if (BUILD_PUBSUB_PSA_UDP_MC)
        add_celix_container(pubsub_udpmc_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_udp_multicast
                pubsub_sut
                pubsub_tst
                )
        target_link_libraries(pubsub_udpmc_tests PRIVATE Celix::pubsub_api jansson::jansson Celix::dfi GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_udpmc_tests SYSTEM PRIVATE gtest)

        add_celix_container(pstm_deadlock_udpmc_test
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v2
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_udp_multicast
                Celix::shell
                Celix::shell_tui
                )
        target_compile_definitions(pstm_deadlock_udpmc_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
        target_link_libraries(pstm_deadlock_udpmc_test PRIVATE Celix::pubsub_api jansson::jansson Celix::dfi GTest::gtest GTest::gtest_main)
        target_include_directories(pstm_deadlock_udpmc_test SYSTEM PRIVATE pstm_deadlock_udpmc_test)

        add_celix_bundle_dependencies(pstm_deadlock_udpmc_test pubsub_deadlock_sut)

        #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_udpmc_gtest/META-INF/descriptors/msg.descriptor COPYONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_udpmc_gtest/META-INF/topics/pub/deadlock.properties COPYONLY)

        add_test(NAME pstm_deadlock_udpmc_test COMMAND pstm_deadlock_udpmc_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_udpmc_test,CONTAINER_LOC>)
        setup_target_for_coverage(pstm_deadlock_udpmc_test SCAN_DIR ..)

        #TODO fix issues with UDPMC and reanble test again
        #add_test(NAME pubsub_udpmc_tests COMMAND pubsub_udpmc_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_udpmc_tests,CONTAINER_LOC>)
        #setup_target_for_coverage(pubsub_udpmc_tests SCAN_DIR ..)
    endif()

    if (BUILD_PUBSUB_PSA_TCP)
        # TCP v2 tests

        add_celix_container(pubsub_tcp_v2_wire_v1_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v1
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                pubsub_sut
                pubsub_tst
                pubsub_serializer
                )
        target_link_libraries(pubsub_tcp_v2_wire_v1_tests PRIVATE Celix::pubsub_api Celix::dfi GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_tcp_v2_wire_v1_tests SYSTEM PRIVATE gtest)
        add_test(NAME pubsub_tcp_v2_wire_v1_tests COMMAND pubsub_tcp_v2_wire_v1_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_tcp_v2_wire_v1_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_tcp_v2_wire_v1_tests SCAN_DIR ..)

        add_celix_container(pubsub_tcp_v2_wire_v2_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v2
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                pubsub_sut
                pubsub_tst
                pubsub_serializer
                )
        target_link_libraries(pubsub_tcp_v2_wire_v2_tests PRIVATE Celix::pubsub_api Celix::dfi GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_tcp_v2_wire_v2_tests SYSTEM PRIVATE gtest)
        add_test(NAME pubsub_tcp_v2_wire_v2_tests COMMAND pubsub_tcp_v2_wire_v2_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_tcp_v2_wire_v2_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_tcp_v2_wire_v2_tests SCAN_DIR ..)

        add_celix_container(pubsub_tcp_v2_wire_v2_with_no_scope_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                CELIX_PUBSUB_TEST_USE_NEGATIVE_SCOPE_FILTER=false
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v2
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                pubsub_sut
                pubsub_tst
                pubsub_serializer
                )
        target_link_libraries(pubsub_tcp_v2_wire_v2_with_no_scope_tests PRIVATE Celix::pubsub_api Celix::dfi GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_tcp_v2_wire_v2_with_no_scope_tests SYSTEM PRIVATE gtest)
        add_test(NAME pubsub_tcp_v2_wire_v2_with_no_scope_tests COMMAND pubsub_tcp_v2_wire_v2_with_no_scope_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_tcp_v2_wire_v2_with_no_scope_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_tcp_v2_wire_v2_with_no_scope_tests SCAN_DIR ..)

        add_celix_container(pubsub_tcp_v2_endpoint_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubEndpointIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::shell
                Celix::shell_tui
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v2
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                pubsub_endpoint_tst
                pubsub_endpoint_sut
                pubsub_loopback
                pubsub_serializer
                )
        target_link_libraries(pubsub_tcp_v2_endpoint_tests PRIVATE Celix::pubsub_api Celix::dfi GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_tcp_v2_endpoint_tests SYSTEM PRIVATE gtest)

        add_celix_container(pstm_deadlock_tcp_v2_test
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_protocol_wire_v2
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_tcp
                Celix::shell
                Celix::shell_tui
                pubsub_serializer
                )
        target_compile_definitions(pstm_deadlock_tcp_v2_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
        target_link_libraries(pstm_deadlock_tcp_v2_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main Celix::dfi)
        target_include_directories(pstm_deadlock_tcp_v2_test SYSTEM PRIVATE pstm_deadlock_tcp_v2_test)

        #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
        add_celix_bundle_dependencies(pstm_deadlock_tcp_v2_test pubsub_deadlock_sut)

        #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_tcp_gtest/META-INF/descriptors/msg.descriptor COPYONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_tcp_gtest/META-INF/topics/pub/deadlock.properties COPYONLY)

        add_test(NAME pstm_deadlock_tcp_v2_test COMMAND pstm_deadlock_tcp_v2_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_tcp_v2_test,CONTAINER_LOC>)
        setup_target_for_coverage(pstm_deadlock_tcp_v2_test SCAN_DIR ..)

        #TCP Endpoint test is disabled because the test is not stable when running on Travis
        if (ENABLE_PUBSUB_PSA_TCP_ENDPOINT_TEST)
            add_test(NAME pubsub_tcp_v2_endpoint_tests COMMAND pubsub_tcp_v2_endpoint_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_tcp_v2_endpoint_tests,CONTAINER_LOC>)
            setup_target_for_coverage(pubsub_tcp_v2_endpoint_tests SCAN_DIR ..)
        endif()
    endif()

    if (BUILD_PUBSUB_PSA_WS)
        add_celix_container(pubsub_websocket_v2_tests
                USE_CONFIG
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                CELIX_HTTP_ADMIN_LISTENING_PORTS=58080
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::http_admin
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_websocket
                pubsub_sut
                pubsub_tst
                pubsub_serializer
                )
        target_link_libraries(pubsub_websocket_v2_tests PRIVATE Celix::pubsub_api jansson::jansson civetweb_shared GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_websocket_v2_tests SYSTEM PRIVATE gtest)
        add_test(NAME pubsub_websocket_v2_tests COMMAND pubsub_websocket_v2_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_websocket_v2_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_websocket_v2_tests SCAN_DIR ..)

        add_celix_container(pstm_deadlock_websocket_v2_test
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                CELIX_HTTP_ADMIN_LISTENING_PORTS=58080
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_websocket
                Celix::shell
                Celix::shell_tui
                pubsub_serializer
                )
        target_compile_definitions(pstm_deadlock_websocket_v2_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
        target_link_libraries(pstm_deadlock_websocket_v2_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main jansson::jansson Celix::dfi civetweb_shared)
        target_include_directories(pstm_deadlock_websocket_v2_test SYSTEM PRIVATE pstm_deadlock_websocket_v2_test)

        #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
        add_celix_bundle_dependencies(pstm_deadlock_websocket_v2_test pubsub_deadlock_sut)

        #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_websocket_v2_test/META-INF/descriptors/msg.descriptor COPYONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_websocket_v2_test/META-INF/topics/pub/deadlock.properties COPYONLY)

        add_test(NAME pstm_deadlock_websocket_v2_test COMMAND pstm_deadlock_websocket_v2_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_websocket_v2_test,CONTAINER_LOC>)
        setup_target_for_coverage(pstm_deadlock_websocket_v2_test SCAN_DIR ..)
    endif()

    if (BUILD_PUBSUB_PSA_ZMQ)
        find_package(ZeroMQ REQUIRED)
        find_package(czmq REQUIRED)

        add_celix_container(pubsub_zmq_v2_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                Celix::celix_pubsub_protocol_wire_v2
                pubsub_sut
                pubsub_tst
                pubsub_serializer
                )

        target_link_libraries(pubsub_zmq_v2_tests PRIVATE Celix::pubsub_api Celix::dfi ZeroMQ::ZeroMQ czmq::czmq GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_zmq_v2_tests SYSTEM PRIVATE gtest)
        add_test(NAME pubsub_zmq_v2_tests COMMAND pubsub_zmq_v2_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_v2_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_zmq_v2_tests SCAN_DIR ..)

        add_celix_container(pubsub_zmq_v2_zerocopy_tests
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/gtest/PubSubIntegrationTestSuite.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                PSA_ZMQ_ZEROCOPY_ENABLED=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                Celix::celix_pubsub_protocol_wire_v2
                Celix::shell
                Celix::shell_tui
                pubsub_sut
                pubsub_tst
                pubsub_serializer
                )

        target_link_libraries(pubsub_zmq_v2_zerocopy_tests PRIVATE Celix::pubsub_api Celix::dfi ZeroMQ::ZeroMQ czmq::czmq GTest::gtest GTest::gtest_main)
        target_include_directories(pubsub_zmq_v2_zerocopy_tests SYSTEM PRIVATE gtest)

        add_test(NAME pubsub_zmq_v2_zerocopy_tests COMMAND pubsub_zmq_v2_zerocopy_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_v2_zerocopy_tests,CONTAINER_LOC>)
        setup_target_for_coverage(pubsub_zmq_v2_zerocopy_tests SCAN_DIR ..)

        add_celix_container(pstm_deadlock_zmq_v2_test
                USE_CONFIG #ensures that a config.properties will be created with the launch bundles.
                LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/pstm_deadlock_test/test_runner.cc
                DIR ${CMAKE_CURRENT_BINARY_DIR}
                PROPERTIES
                LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
                CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=trace
                BUNDLES
                Celix::celix_pubsub_serializer_json
                Celix::celix_pubsub_topology_manager
                Celix::celix_pubsub_admin_zmq
                Celix::celix_pubsub_protocol_wire_v2
                )

        target_compile_definitions(pstm_deadlock_zmq_v2_test PRIVATE -DDEADLOCK_SUT_BUNDLE_FILE=\"${DEADLOCK_SUT_BUNDLE_FILE}\")
        target_link_libraries(pstm_deadlock_zmq_v2_test PRIVATE Celix::pubsub_api GTest::gtest GTest::gtest_main Celix::dfi ZeroMQ::ZeroMQ czmq::czmq)
        target_include_directories(pstm_deadlock_zmq_v2_test SYSTEM PRIVATE pstm_deadlock_zmq_v2_test)

        #Note we do not link to bundles, as result (to ensure a bundle zip file is created) an dependency on the bundle is needed.
        add_celix_bundle_dependencies(pstm_deadlock_zmq_v2_test pubsub_deadlock_sut)

        #Framework "bundle" has no cache dir. Default as "cache dir" the cwd is used.
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/msg.descriptor ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_zmq_v2_test/META-INF/descriptors/msg.descriptor COPYONLY)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta_data/deadlock.scope.properties ${CMAKE_CURRENT_BINARY_DIR}/pstm_deadlock_zmq_v2_test/META-INF/topics/pub/deadlock.properties COPYONLY)

        add_test(NAME pstm_deadlock_zmq_v2_test COMMAND pstm_deadlock_zmq_v2_test WORKING_DIRECTORY $<TARGET_PROPERTY:pstm_deadlock_zmq_v2_test,CONTAINER_LOC>)
        setup_target_for_coverage(pstm_deadlock_zmq_v2_test SCAN_DIR ..)
    endif ()

    if (BUILD_PUBSUB_PSA_ZMQ)
        #Test suite to test if component with same topic and different scope combinations work
        add_executable(test_pubsub_topic_and_scope_integration
                gtest/PubSubTopicAndScopeIntegrationTestSuite.cc
                )
        target_link_libraries(test_pubsub_topic_and_scope_integration PRIVATE Celix::framework Celix::pubsub_api GTest::gtest GTest::gtest_main)
        add_test(NAME test_pubsub_topic_and_scope_integration COMMAND test_pubsub_topic_and_scope_integration)
        setup_target_for_coverage(test_pubsub_topic_and_scope_integration SCAN_DIR ..)

        #configure topology manager and pubsub zmq, json serializer and wire protocol v2 bundles
        celix_get_bundle_file(Celix::celix_pubsub_serializer_json PUBSUB_JSON_BUNDLE_FILE)
        celix_get_bundle_file(Celix::celix_pubsub_topology_manager PUBSUB_TOPMAN_BUNDLE_FILE)
        celix_get_bundle_file(Celix::celix_pubsub_admin_zmq PUBSUB_ZMQ_BUNDLE_FILE)
        celix_get_bundle_file(Celix::celix_pubsub_protocol_wire_v2 PUBSUB_WIRE_BUNDLE_FILE)
        add_celix_bundle_dependencies(test_pubsub_topic_and_scope_integration
                celix_pubsub_serializer_json
                celix_pubsub_topology_manager
                celix_pubsub_admin_zmq
                celix_pubsub_protocol_wire_v2
                )
        target_compile_definitions(test_pubsub_topic_and_scope_integration PRIVATE
                PUBSUB_JSON_BUNDLE_FILE="${PUBSUB_JSON_BUNDLE_FILE}"
                PUBSUB_TOPMAN_BUNDLE_FILE="${PUBSUB_TOPMAN_BUNDLE_FILE}"
                PUBSUB_ZMQ_BUNDLE_FILE="${PUBSUB_ZMQ_BUNDLE_FILE}"
                PUBSUB_WIRE_BUNDLE_FILE="${PUBSUB_WIRE_BUNDLE_FILE}"
                )
    endif ()

    #[[
    Add a integration test with use interceptors for a configurable PSA and wire protocol

    ARGV0 is test target name
    ARGV1 is PSA target name
    ARGV2 is wire protocol target name
    ARGV3 is option boolean whether http admin bundle should be used

    ]]
    function(add_celix_interceptors_test_for_psa_and_wire)
        set(TEST_TARGET_NAME ${ARGV0})
        set(PSA ${ARGV1})
        set(WIRE ${ARGV2})
        set(USE_HTTP_ADMIN ${ARGV3})

        #Test suite to test if pusbub interceptors
        add_executable(${TEST_TARGET_NAME}
                gtest/PubSubInterceptorTestSuite.cc
                )
        target_link_libraries(${TEST_TARGET_NAME} PRIVATE Celix::framework Celix::pubsub_api GTest::gtest GTest::gtest_main Celix::pubsub_spi)
        target_include_directories(${TEST_TARGET_NAME} PRIVATE gtest)
        add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})
        setup_target_for_coverage(${TEST_TARGET_NAME} SCAN_DIR ..)

        #configure topology manager and pubsub admin, json serializer and wire protocol bundles
        celix_get_bundle_file(Celix::celix_pubsub_serializer_json PUBSUB_JSON_BUNDLE_FILE)
        celix_get_bundle_file(Celix::celix_pubsub_topology_manager PUBSUB_TOPMAN_BUNDLE_FILE)
        celix_get_bundle_file(${PSA} PUBSUB_PSA_BUNDLE_FILE)
        celix_get_bundle_file(${WIRE} PUBSUB_WIRE_BUNDLE_FILE)

        celix_get_bundle_file(pubsub_sut PUBSUB_PUBLISHER_BUNDLE_FILE)
        celix_get_bundle_file(pubsub_tst PUBSUB_SUBSCRIBER_BUNDLE_FILE)
        add_celix_bundle_dependencies(${TEST_TARGET_NAME} Celix::celix_pubsub_serializer_json Celix::celix_pubsub_topology_manager ${PSA} ${WIRE} pubsub_sut pubsub_tst)
        target_compile_definitions(${TEST_TARGET_NAME} PRIVATE
                PUBSUB_JSON_BUNDLE_FILE="${PUBSUB_JSON_BUNDLE_FILE}"
                PUBSUB_TOPMAN_BUNDLE_FILE="${PUBSUB_TOPMAN_BUNDLE_FILE}"
                PUBSUB_PSA_BUNDLE_FILE="${PUBSUB_PSA_BUNDLE_FILE}"
                PUBSUB_WIRE_BUNDLE_FILE="${PUBSUB_WIRE_BUNDLE_FILE}"
                PUBSUB_PUBLISHER_BUNDLE_FILE="${PUBSUB_PUBLISHER_BUNDLE_FILE}"
                PUBSUB_SUBSCRIBER_BUNDLE_FILE="${PUBSUB_SUBSCRIBER_BUNDLE_FILE}"
                )

        #if PSA websocket is enabled add http_admin bundle
        if (USE_HTTP_ADMIN)
            target_link_libraries(${TEST_TARGET_NAME} PRIVATE Celix::http_admin_api)
            celix_get_bundle_file(Celix::http_admin HTTP_ADMIN_BUNDLE_FILE)
            add_celix_bundle_dependencies(${TEST_TARGET_NAME} Celix::http_admin)
            target_compile_definitions(${TEST_TARGET_NAME} PRIVATE HTTP_ADMIN_BUNDLE_FILE="${HTTP_ADMIN_BUNDLE_FILE}")
        endif ()

        #Linking against zmq if present
        if (TARGET ZeroMQ::ZeroMQ AND TARGET czmq::czmq)
            target_link_libraries(${TEST_TARGET_NAME} PRIVATE ZeroMQ::ZeroMQ czmq::czmq)
        endif ()
    endfunction()


    if (BUILD_PUBSUB_PSA_WS)
        add_celix_interceptors_test_for_psa_and_wire(test_pubsub_interceptors_ws_and_wire_v1_integration Celix::celix_pubsub_admin_websocket Celix::celix_pubsub_protocol_wire_v1 true)
        add_celix_interceptors_test_for_psa_and_wire(test_pubsub_interceptors_ws_and_wire_v2_integration Celix::celix_pubsub_admin_websocket Celix::celix_pubsub_protocol_wire_v2 true)
    endif ()

    if (BUILD_PUBSUB_PSA_TCP)
        add_celix_interceptors_test_for_psa_and_wire(test_pubsub_interceptors_tcp_and_wire_v1_integration Celix::celix_pubsub_admin_tcp Celix::celix_pubsub_protocol_wire_v1)
        add_celix_interceptors_test_for_psa_and_wire(test_pubsub_interceptors_tcp_and_wire_v2_integration Celix::celix_pubsub_admin_tcp Celix::celix_pubsub_protocol_wire_v2)
    endif ()

    if (BUILD_PUBSUB_PSA_ZMQ)
        add_celix_interceptors_test_for_psa_and_wire(test_pubsub_interceptors_zmq_and_wire_v1_integration Celix::celix_pubsub_admin_zmq Celix::celix_pubsub_protocol_wire_v1)
        add_celix_interceptors_test_for_psa_and_wire(test_pubsub_interceptors_zmq_and_wire_v2_integration Celix::celix_pubsub_admin_zmq Celix::celix_pubsub_protocol_wire_v2)
    endif ()
endif ()
