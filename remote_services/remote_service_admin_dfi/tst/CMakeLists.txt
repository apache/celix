#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
	${PROJECT_SOURCE_DIR}/launcher/public
    ${CPPUTEST_INCLUDE_DIR}
)


#if (FRAMEWORK_TESTS) TODO
	add_executable(rsa_dfi_tests
		${PROJECT_SOURCE_DIR}/launcher/private/src/launcher
	    tst/run_tests.cpp
	)
	target_link_libraries(rsa_dfi_tests ${CPPUTEST_LIBRARY}) #TODO add celix/launcer etc

	get_property(rsa_bundle_file TARGET remote_service_admin_dfi PROPERTY BUNDLE)
	configure_file(config.properties.in config-rsa.properties @ONLY)

	add_test(NAME run_rsa_dfi_tests COMMAND rsa_dfi_tests)
	SETUP_TARGET_FOR_COVERAGE(rsa_dfi_tests_cov rsa_dfi_tests ${CMAKE_BINARY_DIR}/coverage/rsa_dfi)
#endif()
