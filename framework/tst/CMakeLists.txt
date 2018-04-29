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


add_celix_bundle(simple_test_bundle1 NO_ACTIVATOR VERSION 1.0.0)
add_celix_bundle(simple_test_bundle2 NO_ACTIVATOR VERSION 1.0.0)
add_celix_bundle(simple_test_bundle3 NO_ACTIVATOR VERSION 1.0.0)

add_executable(test_framework
    run_tests.cpp
    single_framework_test.cpp
    multiple_frameworks_test.cpp
    bundle_context_bundles_tests.cpp
    bundle_context_services_test.cpp
)
target_link_libraries(test_framework Celix::framework ${CURL_LIBRARIES} ${CPPUTEST_LIBRARY})
add_dependencies(test_framework simple_test_bundle1_bundle simple_test_bundle2_bundle simple_test_bundle3_bundle)

configure_file(config.properties.in config.properties @ONLY)
configure_file(framework1.properties.in framework1.properties @ONLY)
configure_file(framework2.properties.in framework2.properties @ONLY)

add_test(NAME run_test_framework COMMAND test_framework)
SETUP_TARGET_FOR_COVERAGE(test_framework_cov test_framework ${CMAKE_BINARY_DIR}/coverage/test_framework/test_framework)

