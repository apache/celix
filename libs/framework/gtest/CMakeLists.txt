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
add_celix_bundle(simple_cxx_dep_man_bundle SOURCES src/HelloWorldCxxActivatorWithDepMan.cc VERSION 1.0.0)
add_celix_bundle(cmp_test_bundle SOURCES src/CmpTestBundleActivator.cc)
add_subdirectory(subdir) #simple_test_bundle4, simple_test_bundle5 and sublib

add_celix_bundle(unresolvable_bundle SOURCES src/nop_activator.c VERSION 1.0.0)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(POSTFIX ${CMAKE_DEBUG_POSTFIX})
endif()
target_link_libraries(unresolvable_bundle PRIVATE "-L${CMAKE_CURRENT_BINARY_DIR}/subdir -lsublib${POSTFIX}")
if(NOT APPLE)
    set_target_properties(unresolvable_bundle PROPERTIES LINK_FLAGS -Wl,--no-as-needed)
endif()
add_dependencies(unresolvable_bundle sublib)

set(CELIX_FRAMEWORK_TEST_SOURCES
    src/single_framework_test.cpp
    src/CelixFrameworkUtilsTestSuite.cc
    src/multiple_frameworks_test.cpp
    src/bundle_context_bundles_tests.cpp
    src/bundle_context_services_test.cpp
    src/DependencyManagerTestSuite.cc
    src/CxxBundleContextTestSuite.cc
    src/HelloWorldCxxActivator.cc
    src/CxxFrameworkFactoryTestSuite.cc
    src/CxxBundleActivatorTestSuite.cc
)

add_executable(test_framework ${CELIX_FRAMEWORK_TEST_SOURCES})
target_link_libraries(test_framework PRIVATE Celix::framework CURL::libcurl GTest::gtest GTest::gtest_main)


#embed bundle in the test_framework executable
celix_target_embedded_bundles(test_framework simple_test_bundle1 simple_test_bundle2)

#Create bundle set definitions
celix_target_bundle_set_definition(test_framework NAME BUNDLE_EMPTY_TEST_SET)
celix_target_bundle_set_definition(test_framework NAME BUNDLE_TEST_SET
        #Note this test set has two version fo bundle1, this is done for testing purpose.
        simple_test_bundle1 simple_test_bundle3 simple_test_bundle1 simple_test_bundle2
)


add_celix_bundle_dependencies(test_framework
        simple_test_bundle1
        simple_test_bundle2 simple_test_bundle3 simple_test_bundle4
        simple_test_bundle5 bundle_with_exception unresolveable_bundle simple_cxx_bundle simple_cxx_dep_man_bundle cmp_test_bundle)
target_include_directories(test_framework PRIVATE ../src)

celix_get_bundle_file(simple_test_bundle1 SIMPLE_TEST_BUNDLE1)
celix_get_bundle_file(simple_test_bundle2 SIMPLE_TEST_BUNDLE2)
celix_get_bundle_file(simple_test_bundle3 SIMPLE_TEST_BUNDLE3)
celix_get_bundle_file(simple_test_bundle4 SIMPLE_TEST_BUNDLE4)
celix_get_bundle_file(simple_test_bundle5 SIMPLE_TEST_BUNDLE5)
celix_get_bundle_filename(simple_test_bundle4 SIMPLE_TEST_BUNDLE4_FILENAME)
celix_get_bundle_filename(simple_test_bundle5 SIMPLE_TEST_BUNDLE5_FILENAME)

celix_get_bundle_filename(bundle_with_exception BUNDLE_WITH_EXCEPTION)
celix_get_bundle_filename(unresolvable_bundle UNRESOLVABLE_BUNDLE)

celix_get_bundle_file(simple_cxx_bundle SIMPLE_CXX_BUNDLE_LOC)
celix_get_bundle_file(simple_cxx_dep_man_bundle SIMPLE_CXX_DEP_MAN_BUNDLE_LOC)
celix_get_bundle_file(cmp_test_bundle CMP_TEST_BUNDLE_LOC)

configure_file(config.properties.in config.properties @ONLY)
configure_file(framework1.properties.in framework1.properties @ONLY)
configure_file(framework2.properties.in framework2.properties @ONLY)
configure_file(install_and_start_bundles.properties.in install_and_start_bundles.properties @ONLY)

target_compile_definitions(test_framework PRIVATE
        SIMPLE_TEST_BUNDLE1_LOCATION="${SIMPLE_TEST_BUNDLE1}"
        SIMPLE_TEST_BUNDLE2_LOCATION="${SIMPLE_TEST_BUNDLE2}"
        SIMPLE_TEST_BUNDLE3_LOCATION="${SIMPLE_TEST_BUNDLE3}"
        SIMPLE_TEST_BUNDLE4_LOCATION="${SIMPLE_TEST_BUNDLE4_FILENAME}"
        SIMPLE_TEST_BUNDLE5_LOCATION="${SIMPLE_TEST_BUNDLE5_FILENAME}"
        TEST_BUNDLE_WITH_EXCEPTION_LOCATION="${BUNDLE_WITH_EXCEPTION}"
        TEST_BUNDLE_UNRESOLVABLE_LOCATION="${UNRESOLVABLE_BUNDLE}"
        SIMPLE_CXX_BUNDLE_LOC="${SIMPLE_CXX_BUNDLE_LOC}"
        CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
        SIMPLE_CXX_DEP_MAN_BUNDLE_LOC="${SIMPLE_CXX_DEP_MAN_BUNDLE_LOC}"
        CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
        INSTALL_AND_START_BUNDLES_CONFIG_PROPERTIES_FILE="${CMAKE_CURRENT_BINARY_DIR}/install_and_start_bundles.properties"
)

add_test(NAME test_framework COMMAND test_framework)
setup_target_for_coverage(test_framework SCAN_DIR ..)

if (ENABLE_TESTING_DEPENDENCY_MANAGER_FOR_CXX11)
    #Setting standard to C++11 and testing C++ dependency manager to ensure that this still support C++11.
    #This ensure that the C++11 dependency manager is backwards compatible with Celix 2.2.1
    set(CMAKE_CXX_STANDARD 11)
    add_executable(test_dep_man_with_cxx11
            src/DependencyManagerTestSuite.cc
            )
    target_link_libraries(test_dep_man_with_cxx11 PRIVATE Celix::framework CURL::libcurl GTest::gtest GTest::gtest_main)
    target_compile_definitions(test_dep_man_with_cxx11 PRIVATE
            SIMPLE_CXX_DEP_MAN_BUNDLE_LOC="${SIMPLE_CXX_DEP_MAN_BUNDLE_LOC}"
            )
    add_test(NAME test_dep_man_with_cxx11 COMMAND test_dep_man_with_cxx11)
    setup_target_for_coverage(test_dep_man_with_cxx11 SCAN_DIR ..)

    #Also to ensure that CELIX_GEN_CXX_BUNDLE_ACTIVATOR still works for C++11 bundle activators with a
    #dependency manager argument, the HelloWorldCxxActivatorWithDepMan will be used to create a C++11 bundle
    add_celix_bundle(test_dep_man_bundle_activator_with_cxx11 SOURCES src/HelloWorldCxxActivatorWithDepMan.cc VERSION 1.0.0)
endif ()

if (ENABLE_TESTING_FOR_CXX14)
    #Setting standard to C++14 and testing the C++ framework headers to ensure that C++14 is also supported.
    set(CMAKE_CXX_STANDARD 14)

    add_executable(test_framework_with_cxx14 ${CELIX_FRAMEWORK_TEST_SOURCES})
    target_link_libraries(test_framework_with_cxx14 PRIVATE Celix::framework CURL::libcurl GTest::gtest GTest::gtest_main)
    add_celix_bundle_dependencies(test_framework_with_cxx14
            simple_test_bundle1
            simple_test_bundle2 simple_test_bundle3 simple_test_bundle4
            simple_test_bundle5 bundle_with_exception unresolveable_bundle simple_cxx_bundle simple_cxx_dep_man_bundle cmp_test_bundle)
    target_include_directories(test_framework_with_cxx14 PRIVATE ../src)

    #Also to ensure that CELIX_GEN_CXX_BUNDLE_ACTIVATOR still for C++11.
    add_celix_bundle(simple_cxx_bundle_with_cxx1 SOURCES src/HelloWorldCxxActivator.cc VERSION 1.0.0)
    add_celix_bundle(simple_cxx_dep_man_bundle_with_cxx1 SOURCES src/HelloWorldCxxActivatorWithDepMan.cc VERSION 1.0.0)
    add_celix_bundle_dependencies(test_framework_with_cxx14 simple_cxx_bundle_with_cxx1 simple_cxx_dep_man_bundle_with_cxx1)

    celix_get_bundle_file(simple_cxx_bundle_with_cxx1 SIMPLE_CXX_BUNDLE_WITH_CXX11_LOC)
    celix_get_bundle_file(simple_cxx_dep_man_bundle_with_cxx1 SIMPLE_CXX_DEP_MAN_WITH_CXX11_BUNDLE_LOC)

    #embed bundle in the test_framework executable
    celix_target_embedded_bundles(test_framework_with_cxx14 simple_test_bundle1 simple_test_bundle2)

    #Create bundle set definitions
    celix_target_bundle_set_definition(test_framework_with_cxx14 NAME BUNDLE_EMPTY_TEST_SET)
    celix_target_bundle_set_definition(test_framework_with_cxx14 NAME BUNDLE_TEST_SET
        #Note this test set has two version fo bundle1, this is done for testing purpose.
        simple_test_bundle1 simple_test_bundle3 simple_test_bundle1 simple_test_bundle2
    )

    target_compile_definitions(test_framework_with_cxx14 PRIVATE
            SIMPLE_TEST_BUNDLE1_LOCATION="${SIMPLE_TEST_BUNDLE1}"
            SIMPLE_TEST_BUNDLE2_LOCATION="${SIMPLE_TEST_BUNDLE2}"
            SIMPLE_TEST_BUNDLE3_LOCATION="${SIMPLE_TEST_BUNDLE3}"
            SIMPLE_TEST_BUNDLE4_LOCATION="${SIMPLE_TEST_BUNDLE4_FILENAME}"
            SIMPLE_TEST_BUNDLE5_LOCATION="${SIMPLE_TEST_BUNDLE5_FILENAME}"
            TEST_BUNDLE_WITH_EXCEPTION_LOCATION="${BUNDLE_WITH_EXCEPTION}"
            TEST_BUNDLE_UNRESOLVABLE_LOCATION="${UNRESOLVABLE_BUNDLE}"
            SIMPLE_CXX_BUNDLE_LOC="${SIMPLE_CXX_BUNDLE_WITH_CXX11_LOC}"
            CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
            SIMPLE_CXX_DEP_MAN_BUNDLE_LOC="${SIMPLE_CXX_DEP_MAN_WITH_CXX11_BUNDLE_LOC}"
            CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
            INSTALL_AND_START_BUNDLES_CONFIG_PROPERTIES_FILE="${CMAKE_CURRENT_BINARY_DIR}/install_and_start_bundles.properties"
    )

    add_test(NAME test_framework_with_cxx14 COMMAND test_framework_with_cxx14)
    setup_target_for_coverage(test_framework_with_cxx14 SCAN_DIR ..)
endif ()
