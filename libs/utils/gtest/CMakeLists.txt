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

set(CELIX_UTIL_TEST_SOURCES_FOR_CXX_HEADERS
        src/CxxUtilsTestSuite.cc
        src/CxxPropertiesTestSuite.cc
        src/CxxFilterTestSuite.cc
)

add_executable(test_utils
        src/LogUtilsTestSuite.cc
        src/VersionRangeTestSuite.cc
        src/TimeUtilsTestSuite.cc
        src/HashMapTestSuite.cc
        src/ArrayListTestSuite.cc
        src/FileUtilsTestSuite.cc
        ${CELIX_UTIL_TEST_SOURCES_FOR_CXX_HEADERS}
)

target_link_libraries(test_utils PRIVATE Celix::utils GTest::gtest GTest::gtest_main)
target_include_directories(test_utils PRIVATE ../src) #for version_private (needs refactoring of test)

####### generating zip file used for testing ##########################################################################
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/zip_content/top.properties" CONTENT "level=1\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/zip_content/subdir/sub.properties" CONTENT "level=2\n")
set(TEST_ZIP_FILE "${CMAKE_CURRENT_BINARY_DIR}/test.zip")
if(ZIP_COMMAND)
    add_custom_command(OUTPUT ${TEST_ZIP_FILE}
        COMMAND ${ZIP_COMMAND} -rq ${TEST_ZIP_FILE} *
        COMMENT "Creating ${TEST_ZIP_FILE} for testing"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zip_content/top.properties ${CMAKE_CURRENT_BINARY_DIR}/zip_content/subdir/sub.properties
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/zip_content
    )
else()
    add_custom_command(OUTPUT ${TEST_ZIP_FILE}
        COMMAND ${JAR_COMMAND} -cf ${TEST_ZIP_FILE} -C ${CMAKE_CURRENT_BINARY_DIR}/zip_content .
        COMMENT "Creating ${TEST_ZIP_FILE} for testing"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zip_content/top.properties ${CMAKE_CURRENT_BINARY_DIR}/zip_content/subdir/sub.properties
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
add_custom_target(test_utils_resources DEPENDS ${TEST_ZIP_FILE})
add_dependencies(test_utils test_utils_resources)
target_compile_definitions(test_utils PRIVATE -DTEST_ZIP_LOCATION=\"${TEST_ZIP_FILE}\")
target_compile_definitions(test_utils PRIVATE -DTEST_A_FILE_LOCATION=\"${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt\")
target_compile_definitions(test_utils PRIVATE -DTEST_A_DIR_LOCATION=\"${CMAKE_CURRENT_SOURCE_DIR}\")

####### linking zip against test executable to text extract zip from data ###############################################
if (UNIX AND NOT APPLE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/embed_zip_linux.s.in ${CMAKE_BINARY_DIR}/embed_zip_linux.s @ONLY)
    target_sources(test_utils PRIVATE ${CMAKE_BINARY_DIR}/embed_zip_linux.s)
else ()
    target_link_libraries(test_utils PRIVATE -Wl,-sectcreate -Wl,resources -Wl,test_zip -Wl,${TEST_ZIP_FILE})
endif ()

add_test(NAME test_utils COMMAND test_utils)
setup_target_for_coverage(test_utils SCAN_DIR ..)

if (ENABLE_TESTING_FOR_CXX14)
    #Setting standard to C++14 and testing C++ Properties.h, Filter.h and Utils.h to ensure that C++14 is supported.
    set(CMAKE_CXX_STANDARD 14)
    add_executable(test_utils_cxx_headers_with_cxx14
            ${CELIX_UTIL_TEST_SOURCES_FOR_CXX_HEADERS}
    )
    target_link_libraries(test_utils_cxx_headers_with_cxx14 PRIVATE Celix::utils GTest::gtest GTest::gtest_main)
    add_test(NAME test_utils_cxx_headers_with_cxx14 COMMAND test_utils_cxx_headers_with_cxx14)
    setup_target_for_coverage(test_utils_cxx_headers_with_cxx14 SCAN_DIR ..)
endif ()

