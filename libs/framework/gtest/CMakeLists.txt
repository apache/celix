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
celix_bundle_name(simple_test_bundle1 "Simple Test Bundle")
celix_bundle_group(simple_test_bundle1 "test/group")
celix_bundle_description(simple_test_bundle1 "Test Description")

add_celix_bundle(simple_test_bundle2 NO_ACTIVATOR VERSION 1.0.0)
add_celix_bundle(simple_test_bundle3 NO_ACTIVATOR VERSION 1.0.0)
add_celix_bundle(bundle_with_exception SOURCES src/nop_activator.c VERSION 1.0.0)
add_celix_bundle(simple_cxx_bundle SOURCES src/HelloWorldCxxActivator.cc VERSION 1.0.0)
add_celix_bundle(cmp_test_bundle SOURCES src/CmpTestBundleActivator.cc)
add_subdirectory(subdir) #simple_test_bundle4, simple_test_bundle5 and sublib

add_celix_bundle(unresolveable_bundle SOURCES src/nop_activator.c VERSION 1.0.0)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(POSTFIX ${CMAKE_DEBUG_POSTFIX})
endif()
target_link_libraries(unresolveable_bundle PRIVATE "-L${CMAKE_CURRENT_BINARY_DIR}/subdir -lsublib${POSTFIX}")
if(NOT APPLE)
    set_target_properties(unresolveable_bundle PROPERTIES LINK_FLAGS -Wl,--no-as-needed)
endif()
add_dependencies(unresolveable_bundle sublib)

add_executable(test_framework
        src/single_framework_test.cpp
        src/multiple_frameworks_test.cpp
        src/bundle_context_bundles_tests.cpp
        src/bundle_context_services_test.cpp
        src/DependencyManagerTestSuite.cc
        src/CxxBundleContextTestSuite.cc
        src/HelloWorldCxxActivator.cc
        src/CxxFrameworkFactoryTestSuite.cc
        src/CxxBundleActivatorTestSuite.cc)
target_link_libraries(test_framework PRIVATE Celix::framework CURL::libcurl GTest::gtest GTest::gtest_main)
add_celix_bundle_dependencies(test_framework
        simple_test_bundle1
        simple_test_bundle2 simple_test_bundle3 simple_test_bundle4
        simple_test_bundle5 bundle_with_exception unresolveable_bundle simple_cxx)
target_include_directories(test_framework PRIVATE ../src)

celix_get_bundle_file(simple_cxx_bundle SIMPLE_CXX_BUNDLE_LOC)
celix_get_bundle_file(cmp_test_bundle CMP_TEST_BUNDLE_LOC)
target_compile_definitions(test_framework PRIVATE
        SIMPLE_TEST_BUNDLE1_LOCATION="$<TARGET_PROPERTY:simple_test_bundle1,BUNDLE_FILE>"
        SIMPLE_TEST_BUNDLE2_LOCATION="$<TARGET_PROPERTY:simple_test_bundle2,BUNDLE_FILE>"
        SIMPLE_TEST_BUNDLE3_LOCATION="$<TARGET_PROPERTY:simple_test_bundle3,BUNDLE_FILE>"
        SIMPLE_TEST_BUNDLE4_LOCATION="$<TARGET_PROPERTY:simple_test_bundle4,BUNDLE_FILENAME>"
        SIMPLE_TEST_BUNDLE5_LOCATION="$<TARGET_PROPERTY:simple_test_bundle5,BUNDLE_FILENAME>"
        TEST_BUNDLE_WITH_EXCEPTION_LOCATION="$<TARGET_PROPERTY:bundle_with_exception,BUNDLE_FILE>"
        TEST_BUNDLE_UNRESOLVEABLE_LOCATION="$<TARGET_PROPERTY:unresolveable_bundle,BUNDLE_FILE>"
        SIMPLE_CXX_BUNDLE_LOC="${SIMPLE_CXX_BUNDLE_LOC}"
        CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
)

configure_file(config.properties.in config.properties @ONLY)
configure_file(framework1.properties.in framework1.properties @ONLY)
configure_file(framework2.properties.in framework2.properties @ONLY)

add_test(NAME test_framework COMMAND test_framework)
setup_target_for_coverage(test_framework SCAN_DIR ..)

#Setting standard to C++11 and testing C++ dependency manager to ensure that this still support C++11.
#This ensure that the C++11 dependency manager is backwards compatible with Celix 2.2.1
set(CMAKE_CXX_STANDARD 11)
add_executable(test_dep_man_with_cxx11
        src/DependencyManagerTestSuite.cc
)
target_link_libraries(test_dep_man_with_cxx11 PRIVATE Celix::framework CURL::libcurl GTest::gtest GTest::gtest_main)
setup_target_for_coverage(test_dep_man_with_cxx11 SCAN_DIR ..)


