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
add_celix_bundle(bundle_with_bad_export NO_ACTIVATOR VERSION 1.0.0)
celix_bundle_headers(bundle_with_bad_export "Export-Library: $<SEMICOLON>")
add_celix_bundle(simple_cxx_bundle SOURCES src/HelloWorldCxxActivator.cc VERSION 1.0.0)
celix_bundle_libs(simple_cxx_bundle "PRIVATE" TRUE Celix::framework)
add_celix_bundle(simple_cxx_dep_man_bundle SOURCES src/HelloWorldCxxActivatorWithDepMan.cc VERSION 1.0.0)
add_celix_bundle(cmp_test_bundle SOURCES src/CmpTestBundleActivator.cc VERSION 1.0.0)
add_celix_bundle(cond_test_bundle SOURCES src/CondTestBundleActivator.cc VERSION 1.0.0)
add_subdirectory(subdir) #simple_test_bundle4, simple_test_bundle5 and sublib
add_celix_bundle(celix_err_test_bundle SOURCES src/activator_with_celix_err.c VERSION 1.0.0)

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
    src/CelixFrameworkTestSuite.cc
    src/CelixFrameworkUtilsTestSuite.cc
    src/MultipleFrameworkTestSuite.cc
    src/CelixBundleContextBundlesTestSuite.cc
    src/CelixBundleContextServicesTestSuite.cc
    src/DependencyManagerTestSuite.cc
    src/CxxBundleContextTestSuite.cc
    src/HelloWorldCxxActivator.cc
    src/FrameworkFactoryTestSuite.cc
    src/CxxBundleActivatorTestSuite.cc
    src/BundleArchiveTestSuite.cc
    src/CelixLauncherTestSuite.cc
    src/CelixBundleCacheTestSuite.cc
    src/ScheduledEventTestSuite.cc
    src/FrameworkBundleTestSuite.cc
    src/ManifestTestSuite.cc
)

add_executable(test_framework ${CELIX_FRAMEWORK_TEST_SOURCES})
# framework_cut makes error injector work
target_link_libraries(test_framework PRIVATE framework_cut GTest::gtest GTest::gtest_main)
celix_deprecated_framework_headers(test_framework)

#Create bundle set definitions
celix_target_bundle_set_definition(test_framework NAME BUNDLE_EMPTY_TEST_SET)
celix_target_bundle_set_definition(test_framework NAME BUNDLE_TEST_SET
        #Note this test set has two version fo bundle1, this is done for testing purpose.
        simple_test_bundle1 simple_test_bundle3 simple_test_bundle1 simple_test_bundle2
)


add_celix_bundle_dependencies(test_framework
        simple_test_bundle1
        simple_test_bundle2 simple_test_bundle3 simple_test_bundle4
        simple_test_bundle5 bundle_with_exception bundle_with_bad_export
        unresolvable_bundle simple_cxx_bundle simple_cxx_dep_man_bundle cmp_test_bundle
        celix_err_test_bundle)
target_include_directories(test_framework PRIVATE ../src)
celix_deprecated_utils_headers(test_framework)

celix_get_bundle_file(simple_test_bundle1 SIMPLE_TEST_BUNDLE1)
celix_get_bundle_file(simple_test_bundle2 SIMPLE_TEST_BUNDLE2)
celix_get_bundle_file(simple_test_bundle3 SIMPLE_TEST_BUNDLE3)
celix_get_bundle_file(simple_test_bundle4 SIMPLE_TEST_BUNDLE4)
celix_get_bundle_file(simple_test_bundle5 SIMPLE_TEST_BUNDLE5)
celix_get_bundle_filename(simple_test_bundle4 SIMPLE_TEST_BUNDLE4_FILENAME)
celix_get_bundle_filename(simple_test_bundle5 SIMPLE_TEST_BUNDLE5_FILENAME)

celix_get_bundle_filename(bundle_with_exception BUNDLE_WITH_EXCEPTION)
celix_get_bundle_file(bundle_with_bad_export BUNDLE_WITH_BAD_EXPORT)
celix_get_bundle_filename(unresolvable_bundle UNRESOLVABLE_BUNDLE)

celix_get_bundle_file(simple_cxx_bundle SIMPLE_CXX_BUNDLE_LOC)
celix_get_bundle_file(simple_cxx_dep_man_bundle SIMPLE_CXX_DEP_MAN_BUNDLE_LOC)
celix_get_bundle_file(cmp_test_bundle CMP_TEST_BUNDLE_LOC)
celix_get_bundle_file(cond_test_bundle COND_TEST_BUNDLE_LOC)

configure_file(config.properties.in config.properties @ONLY)
configure_file(framework1.properties.in framework1.properties @ONLY)
configure_file(framework2.properties.in framework2.properties @ONLY)
configure_file(empty.properties.in empty.properties @ONLY)
configure_file(install_and_start_bundles.properties.in install_and_start_bundles.properties @ONLY)

celix_target_bundle_set_definition(test_framework NAME CELIX_ERR_TEST_BUNDLE celix_err_test_bundle)

target_compile_definitions(test_framework PRIVATE
        SIMPLE_TEST_BUNDLE1_LOCATION="${SIMPLE_TEST_BUNDLE1}"
        SIMPLE_TEST_BUNDLE2_LOCATION="${SIMPLE_TEST_BUNDLE2}"
        SIMPLE_TEST_BUNDLE3_LOCATION="${SIMPLE_TEST_BUNDLE3}"
        SIMPLE_TEST_BUNDLE4_LOCATION="${SIMPLE_TEST_BUNDLE4_FILENAME}"
        SIMPLE_TEST_BUNDLE5_LOCATION="${SIMPLE_TEST_BUNDLE5_FILENAME}"
        TEST_BUNDLE_WITH_EXCEPTION_LOCATION="${BUNDLE_WITH_EXCEPTION}"
        BUNDLE_WITH_BAD_EXPORT_LOCATION="${BUNDLE_WITH_BAD_EXPORT}"
        TEST_BUNDLE_UNRESOLVABLE_LOCATION="${UNRESOLVABLE_BUNDLE}"
        SIMPLE_CXX_BUNDLE_LOC="${SIMPLE_CXX_BUNDLE_LOC}"
        CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
        SIMPLE_CXX_DEP_MAN_BUNDLE_LOC="${SIMPLE_CXX_DEP_MAN_BUNDLE_LOC}"
        CMP_TEST_BUNDLE_LOC="${CMP_TEST_BUNDLE_LOC}"
        COND_TEST_BUNDLE_LOC="${COND_TEST_BUNDLE_LOC}"
        INSTALL_AND_START_BUNDLES_CONFIG_PROPERTIES_FILE="${CMAKE_CURRENT_BINARY_DIR}/install_and_start_bundles.properties"
)
if (ENABLE_TESTING_ON_CI)
    target_compile_definitions(test_framework PRIVATE TESTING_ON_CI=1)
endif ()
add_test(NAME test_framework COMMAND test_framework)
setup_target_for_coverage(test_framework SCAN_DIR ..)

if (EI_TESTS)
    add_executable(test_framework_with_ei
            src/BundleArchiveWithErrorInjectionTestSuite.cc
            src/CelixFrameworkUtilsErrorInjectionTestSuite.cc
            src/CelixBundleContextBundlesWithErrorTestSuite.cc
            src/CelixBundleCacheErrorInjectionTestSuite.cc
            src/ScheduledEventWithErrorInjectionTestSuite.cc
            src/FrameworkBundleWithErrorInjectionTestSuite.cc
            src/FrameworkFactoryWithErrorInjectionTestSuite.cc
            src/ManifestErrorInjectionTestSuite.cc
            src/CelixLauncherErrorInjectionTestSuite.cc
    )
    target_compile_definitions(test_framework_with_ei PRIVATE
            SIMPLE_TEST_BUNDLE1_LOCATION="${SIMPLE_TEST_BUNDLE1}"
            SIMPLE_CXX_BUNDLE_LOC="${SIMPLE_CXX_BUNDLE_LOC}"
    )
    add_celix_bundle_dependencies(test_framework_with_ei simple_test_bundle1 simple_cxx_bundle)
    celix_deprecated_utils_headers(test_framework_with_ei)
    target_link_libraries(test_framework_with_ei PRIVATE
            framework_cut
            Celix::malloc_ei
            Celix::utils_ei
            Celix::asprintf_ei
            Celix::dlfcn_ei
            Celix::unistd_ei
            Celix::string_hash_map_ei
            Celix::properties_ei
            Celix::stdlib_ei
            Celix::stat_ei
            Celix::threads_ei
            Celix::hmap_ei
            Celix::stdio_ei
            Celix::string_ei
            GTest::gtest GTest::gtest_main
    )

    add_test(NAME test_framework_with_ei COMMAND test_framework_with_ei)
    setup_target_for_coverage(test_framework_with_ei SCAN_DIR ..)
endif ()
