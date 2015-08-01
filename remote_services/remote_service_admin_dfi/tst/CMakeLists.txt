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

	add_executable(rsa_dfi_tests
	    ${PROJECT_SOURCE_DIR}/launcher/private/src/launcher.c
	    run_tests.cpp
	    rsa_tests.cpp
	)
	target_link_libraries(rsa_dfi_tests ${CPPUTEST_LIBRARY} celix_framework celix_utils ${CURL_LIBRARIES})

	get_property(rsa_bundle_file TARGET remote_service_admin_dfi PROPERTY BUNDLE)
	configure_file(config.properties.in config.properties @ONLY)

	add_test(NAME run_rsa_dfi_tests COMMAND rsa_dfi_tests)
	SETUP_TARGET_FOR_COVERAGE(rsa_dfi_tests_cov rsa_dfi_tests ${CMAKE_BINARY_DIR}/coverage/rsa_dfi)
#endif()
