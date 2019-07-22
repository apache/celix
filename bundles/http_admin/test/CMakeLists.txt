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

add_celix_bundle(http_admin_sut
    #"Vanilla" bundle which is under test
    SOURCES
        test/sut_activator.c
    VERSION 1.0.0
)
target_include_directories(http_admin_sut PRIVATE test)
target_link_libraries(http_admin_sut PRIVATE Celix::http_admin_api)
celix_bundle_add_dir(http_admin_sut docroot DESTINATION ".")
celix_bundle_headers(http_admin_sut
        "X-Web-Resource: /alias$<SEMICOLON>/docroot/foo/bar, /socket_alias$<SEMICOLON>/docroot/foo/bar"
)

#add_celix_bundle(http_admin_tst
#    #Test bundle containing cpputests and uses celix_test_runner launcher instead of the celix launcher
#    SOURCES
#        test/http_websocket_tests.cc
#    VERSION 1.0.0
#)
#target_link_libraries(http_admin_tst PRIVATE Celix::framework Celix::http_admin_api)
#target_include_directories(http_admin_tst PRIVATE ${CPPUTEST_INCLUDE_DIR})
#celix_bundle_private_libs(http_admin_tst civetweb_shared)


add_celix_container(http_websocket_tests
        LAUNCHER_SRC ${CMAKE_CURRENT_LIST_DIR}/test/test_runner.cc
        DIR ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
            LOGHELPER_STDOUT_FALLBACK_INCLUDE_DEBUG=true
        BUNDLES
            Celix::http_admin_service
            http_admin_sut
#            http_admin_tst
)
target_sources(http_websocket_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/test/http_websocket_tests.cc)
target_link_libraries(http_websocket_tests PRIVATE Celix::http_admin_api ${CPPUTEST_LIBRARIES})
target_include_directories(http_websocket_tests PRIVATE ${CPPUTEST_INCLUDE_DIR})

add_test(NAME http_websocket_tests COMMAND http_websocket_tests WORKING_DIRECTORY $<TARGET_PROPERTY:http_websocket_tests,CONTAINER_LOC>)
SETUP_TARGET_FOR_COVERAGE(http_websocket_tests http_websocket_tests ${CMAKE_BINARY_DIR}/coverage/http_websocket_tests/http_websocket_tests ..)

get_target_property(http_admin_service_cmp Celix::http_admin_service BUNDLE_FILE)
get_target_property(http_admin_sut_cmp http_admin_sut BUNDLE_FILE)
#get_target_property(http_admin_tst_cmp http_admin_tst BUNDLE_FILE)
set(loghelper_std_out_fallback_incl_debug true)
set(use_websockets true)
set(listening_ports 65536) #Set invalid port to test range functionality
configure_file(${CMAKE_CURRENT_LIST_DIR}/config.properties.in ${CMAKE_CURRENT_BINARY_DIR}/http_websocket_tests/config.properties)

