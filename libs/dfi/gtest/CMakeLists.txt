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


add_executable(test_dfi
		src/dyn_example_functions.c
		src/dyn_type_tests.cpp
		src/dyn_function_tests.cpp
		src/dyn_closure_tests.cpp
		src/dyn_interface_tests.cpp
		src/dyn_message_tests.cpp
		src/json_serializer_tests.cpp
		src/json_rpc_tests.cpp
		src/dyn_common_tests.cc
)


target_include_directories(test_dfi PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)
target_link_libraries(test_dfi PRIVATE dfi_cut Celix::utils libffi::libffi jansson::jansson GTest::gtest GTest::gtest_main)
celix_deprecated_utils_headers(test_dfi)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/descriptors DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME run_test_dfi COMMAND test_dfi)
setup_target_for_coverage(test_dfi SCAN_DIR ..)

if (EI_TESTS)
	add_executable(test_dfi_with_ei
			src/dyn_interface_ei_tests.cc
			src/dyn_common_ei_tests.cc
			src/dyn_type_ei_tests.cc
			src/dyn_message_ei_tests.cc
			src/dyn_function_ei_tests.cc
	)
	target_link_libraries(test_dfi_with_ei PRIVATE
			dfi_cut
			Celix::malloc_ei
			Celix::stdio_ei
			Celix::string_ei
			Celix::ffi_ei
			GTest::gtest GTest::gtest_main
	)
	add_test(NAME run_test_dfi_with_ei COMMAND test_dfi_with_ei)
	setup_target_for_coverage(test_dfi_with_ei SCAN_DIR ..)
endif ()
