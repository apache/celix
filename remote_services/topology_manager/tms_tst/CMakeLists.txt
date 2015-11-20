#
# Licensed under Apache License v2. See LICENSE for more information.
#


include_directories(
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include
    ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include
    disc_mock
    bundle
)

add_subdirectory(bundle)
add_subdirectory(disc_mock)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils")

add_executable(test_tm_scoped
    run_tests.cpp
    tms_tests.cpp
    
   ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c
)
target_link_libraries(test_tm_scoped celix_framework celix_utils ${CPPUTEST_LIBRARY} ${JANSSON_LIBRARY})

add_dependencies(test_tm_scoped remote_service_admin_dfi topology_manager calculator)

#get_property(rsa_bundle_file TARGET remote_service_admin_dfi PROPERTY BUNDLE)
# Not nice, but only (?) solution to prevent circular dependency between this test and rsa_dfi_test
set_property(GLOBAL PROPERTY temp_rsa_bundle_file ${PROJECT_BINARY_DIR}/remote_services/remote_service_admin_dfi/rsa/remote_service_admin_dfi.zip)
get_property(rsa_bundle_file GLOBAL PROPERTY temp_rsa_bundle_file)
get_property(calc_bundle_file TARGET calculator PROPERTY BUNDLE)
get_property(topology_manager_bundle_file TARGET topology_manager PROPERTY BUNDLE)
get_property(disc_mock_bundle_file TARGET topology_manager_disc_mock_bundle PROPERTY BUNDLE)
get_property(test_bundle_file TARGET topology_manager_test_bundle PROPERTY BUNDLE)

configure_file(config.properties.in config.properties @ONLY)
configure_file(config_import.properties.in config_import.properties @ONLY)
configure_file(scope.json scope.json @ONLY)
configure_file(scope2.json scope2.json @ONLY)
configure_file(scope3.json scope3.json @ONLY)
configure_file(scope4.json scope4.json @ONLY)


add_test(NAME run_test_tm_scoped COMMAND test_tm_scoped)
SETUP_TARGET_FOR_COVERAGE(test_tm_scoped_cov test_tm_scoped ${CMAKE_BINARY_DIR}/coverage/test_tm_scoped/test_tm_scoped)

