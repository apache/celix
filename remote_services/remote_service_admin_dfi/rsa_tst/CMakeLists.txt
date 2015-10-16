#
# Licensed under Apache License v2. See LICENSE for more information.
#


include_directories(
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include
    bundle
)

add_subdirectory(bundle)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils")

add_executable(test_rsa_dfi
    run_tests.cpp
    rsa_tests.cpp
    rsa_client_server_tests.cpp

    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c
)
target_link_libraries(test_rsa_dfi celix_framework celix_utils ${CURL_LIBRARIES} ${CPPUTEST_LIBRARY})

get_property(rsa_bundle_file TARGET remote_service_admin_dfi PROPERTY BUNDLE)
get_property(calc_bundle_file TARGET calculator PROPERTY BUNDLE)
get_property(calculator_shell_bundle_file TARGET calculator_shell PROPERTY BUNDLE)
get_property(discovery_configured_bundle_file TARGET discovery_configured PROPERTY BUNDLE)
get_property(topology_manager_bundle_file TARGET topology_manager PROPERTY BUNDLE)
get_property(tst_bundle_file TARGET rsa_dfi_tst_bundle PROPERTY BUNDLE)

configure_file(config.properties.in config.properties @ONLY)
configure_file(client.properties.in client.properties @ONLY)
configure_file(server.properties.in server.properties @ONLY)

add_dependencies(test_rsa_dfi remote_service_admin_dfi calculator)

add_test(NAME run_test_rsa_dfi COMMAND test_rsa_dfi)
SETUP_TARGET_FOR_COVERAGE(test_rsa_dfi_cov test_rsa_dfi ${CMAKE_BINARY_DIR}/coverage/rsa_dfi)

