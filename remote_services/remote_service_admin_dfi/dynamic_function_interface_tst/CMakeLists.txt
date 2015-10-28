#
# Licensed under Apache License v2. See LICENSE for more information.
#
include_directories(
	../dynamic_function_interface
)

add_executable(test_dfi
	dyn_type_tests.cpp
	dyn_function_tests.cpp
	dyn_closure_tests.cpp
	dyn_interface_tests.cpp
	json_serializer_tests.cpp
	json_rpc_tests.cpp
	run_tests.cpp
)
target_link_libraries(test_dfi dfi ${CPPUTEST_LIBRARY})

add_custom_target(copy-input
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/schemas schemas
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/descriptors descriptors
)
add_dependencies(test_dfi copy-input)

add_test(NAME run_test_dfi COMMAND test_dfi)
SETUP_TARGET_FOR_COVERAGE(test_dfi_cov test_dfi ${CMAKE_BINARY_DIR}/coverage/test_dfi/test_dfi)

