#
# Licensed under Apache License v2. See LICENSE for more information.
#


include_directories(
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
)


SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils")

add_executable(test_framework
    run_tests.cpp
    single_framework_test.cpp
    multiple_frameworks_test.cpp
)
target_link_libraries(test_framework celix_framework celix_utils ${CURL_LIBRARIES} ${CPPUTEST_LIBRARY})

configure_file(config.properties.in config.properties @ONLY)
configure_file(framework1.properties.in framework1.properties @ONLY)
configure_file(framework2.properties.in framework2.properties @ONLY)

add_test(NAME run_test_framework COMMAND test_framework)
SETUP_TARGET_FOR_COVERAGE(test_framework_cov test_framework ${CMAKE_BINARY_DIR}/coverage/framework)

