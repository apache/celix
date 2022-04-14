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

find_package(jansson REQUIRED)

add_celix_bundle(http_admin_sut
    #"Vanilla" bundle which is under test
    SOURCES
        src/sut_activator.c
    VERSION 1.0.0
)
target_include_directories(http_admin_sut PRIVATE test)
target_link_libraries(http_admin_sut PRIVATE Celix::http_admin_api)
celix_bundle_add_dir(http_admin_sut docroot DESTINATION ".")
celix_bundle_headers(http_admin_sut
        "X-Web-Resource: /alias$<SEMICOLON>/docroot/foo/bar, /socket_alias$<SEMICOLON>/docroot/foo/bar"
)

add_executable(http_websocket_tests
        src/http_admin_info_tests.cc
        src/http_websocket_tests.cc
)

celix_get_bundle_file(Celix::http_admin HTTP_ADMIN_BUNDLE)
celix_get_bundle_file(http_admin_sut HTTP_ADMIN_SUT_BUNDLE)
target_compile_definitions(http_websocket_tests PRIVATE
        HTTP_ADMIN_BUNDLE="${HTTP_ADMIN_BUNDLE}"
        HTTP_ADMIN_SUT_BUNDLE="${HTTP_ADMIN_SUT_BUNDLE}"
)
target_link_libraries(http_websocket_tests PRIVATE Celix::framework Celix::http_admin_api GTest::gtest GTest::gtest_main)

add_test(NAME http_websocket_tests COMMAND http_websocket_tests)
setup_target_for_coverage(http_websocket_tests SCAN_DIR ../http_admin)

