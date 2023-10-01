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

set(CMAKE_CXX_STANDARD 17)

#Add active/inactive test component bundles
add_celix_bundle(ActiveComponentBundle SOURCES src/ActiveComponentBundle.cc VERSION 1.0.0)
add_celix_bundle(InactiveComponentBundle SOURCES src/InactiveComponentBundle.cc VERSION 1.0.0)

add_executable(test_components_ready src/ComponentsReadyTestSuite.cc)
target_link_libraries(test_components_ready PRIVATE
        Celix::framework
        Celix::components_ready_api
        GTest::gtest
        GTest::gtest_main
)
if (ENABLE_TESTING_ON_CI)
    target_compile_definitions(test_components_ready PRIVATE TESTING_ON_CI=1)
endif ()
celix_target_bundle_set_definition(test_components_ready NAME COMPONENTS_READY_CHECK_BUNDLE_SET Celix::components_ready_check)
celix_target_bundle_set_definition(test_components_ready NAME ACTIVE_CMP_TEST_BUNDLE_SET ActiveComponentBundle)
celix_target_bundle_set_definition(test_components_ready NAME INACTIVE_CMP_TEST_BUNDLE_SET InactiveComponentBundle)

add_test(NAME test_components_ready COMMAND test_components_ready)
setup_target_for_coverage(test_components_ready SCAN_DIR ..)

if (EI_TESTS)
    add_executable(test_components_ready_with_ei src/ComponentsReadyWithErrorInjectionTestSuite.cc)
    target_link_libraries(test_components_ready_with_ei PRIVATE
            framework_cut
            components_ready_cut
            Celix::components_ready_api
            Celix::malloc_ei
            Celix::threads_ei
            Celix::bundle_ctx_ei
            Celix::properties_ei
            GTest::gtest
            GTest::gtest_main
    )

    add_test(NAME test_components_ready_with_ei COMMAND test_components_ready_with_ei)
    setup_target_for_coverage(test_components_ready_with_ei SCAN_DIR ..)
endif ()
