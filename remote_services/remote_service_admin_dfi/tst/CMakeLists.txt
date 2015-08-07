#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
	${PROJECT_SOURCE_DIR}/launcher/public/include
    ${CPPUTEST_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/framework/public/include
    ${PROJECT_SOURCE_DIR}/utils/public/include
)

#if (FRAMEWORK_TESTS) TODO
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
    SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/framework" "${PROJECT_BINARY_DIR}/utils")

	add_executable(test_rsa_dfi
	    run_tests.cpp
	    rsa_tests.cpp

	    ${PROJECT_SOURCE_DIR}/launcher/private/src/launcher.c #TODO move to libframework
	    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c
	)
	target_link_libraries(test_rsa_dfi ${CPPUTEST_LIBRARY} celix_framework celix_utils ${CURL_LIBRARIES})

	get_property(rsa_bundle_file TARGET remote_service_admin_dfi PROPERTY BUNDLE)
	get_property(calc_bundle_file TARGET calculator PROPERTY BUNDLE)
    configure_file(config.properties.in config.properties @ONLY)

    add_dependencies(test_rsa_dfi remote_service_admin_dfi calculator)


	add_test(NAME run_test_rsa_dfi COMMAND test_rsa_dfi)
	SETUP_TARGET_FOR_COVERAGE(test_rsa_dfi_cov test_rsa_dfi ${CMAKE_BINARY_DIR}/coverage/rsa_dfi)
#endif()
