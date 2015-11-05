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

include_directories(
	../dynamic_function_interface
)

add_executable(test_dfi
	dyn_type_tests.cpp
	dyn_function_tests.cpp
	dyn_closure_tests.cpp
	dyn_interface_tests.cpp
	dyn_message_tests.cpp
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

