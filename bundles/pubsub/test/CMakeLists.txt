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
        test/tst_activator.cc
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
target_link_libraries(pubsub_udpmc_tests PRIVATE Celix::pubsub_api ${CPPUTEST_LIBRARIES} ${JANSSON_LIBRARIES} Celix::dfi)
target_include_directories(pubsub_udpmc_tests PRIVATE ${CPPUTEST_INCLUDE_DIR})
message(WARNING "TODO fix issues with UDPMC and reanble test again")
#add_test(NAME pubsub_udpmc_tests COMMAND pubsub_udpmc_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_udpmc_tests,CONTAINER_LOC>)
#SETUP_TARGET_FOR_COVERAGE(pubsub_udpmc_tests_cov pubsub_udpmc_tests ${CMAKE_BINARY_DIR}/coverage/pubsub_udpmc_tests/pubsub_udpmc_tests ..)

if (BUILD_PUBSUB_PSA_ZMQ)
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
                pubsub_sut
                pubsub_tst
    )

    target_link_libraries(pubsub_zmq_tests PRIVATE Celix::pubsub_api ${CPPUTEST_LIBRARIES} ${JANSSON_LIBRARIES} Celix::dfi)
    target_include_directories(pubsub_zmq_tests PRIVATE ${CPPUTEST_INCLUDE_DIR})
    add_test(NAME pubsub_zmq_tests COMMAND pubsub_zmq_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_tests,CONTAINER_LOC>)
    SETUP_TARGET_FOR_COVERAGE(pubsub_zmq_tests_cov pubsub_zmq_tests ${CMAKE_BINARY_DIR}/coverage/pubsub_zmq_tests/pubsub_zmq_tests ..)


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
                pubsub_sut
                pubsub_tst
    )
    target_link_libraries(pubsub_zmq_zerocopy_tests PRIVATE Celix::pubsub_api ${CPPUTEST_LIBRARIES} ${JANSSON_LIBRARIES} Celix::dfi)
    target_include_directories(pubsub_zmq_zerocopy_tests PRIVATE ${CPPUTEST_INCLUDE_DIR})
    add_test(NAME pubsub_zmq_zerocopy_tests COMMAND pubsub_zmq_zerocopy_tests WORKING_DIRECTORY $<TARGET_PROPERTY:pubsub_zmq_zerocopy_tests,CONTAINER_LOC>)
    SETUP_TARGET_FOR_COVERAGE(pubsub_zmq_zerocopy_tests_cov pubsub_zmq_zerocopy_tests ${CMAKE_BINARY_DIR}/coverage/pubsub_zmq_tests/pubsub_zmq_zerocopy_tests ..)
endif ()