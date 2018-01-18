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

find_package(FFI REQUIRED)
find_package(Jansson REQUIRED)

set(SOURCES
	src/dyn_common.c
	src/dyn_type.c
	src/dyn_function.c
	src/dyn_interface.c
	src/dyn_message.c
	src/json_serializer.c
	src/json_rpc.c
)

add_library(celix_dfi SHARED ${SOURCES})
set_target_properties(celix_dfi PROPERTIES OUTPUT_NAME "celix_dfi")
target_include_directories(celix_dfi PUBLIC include ${JANSSON_INCLUDE_DIRS})
target_include_directories(celix_dfi PRIVATE src ${FFI_INCLUDE_DIRS})
target_link_libraries(celix_dfi PUBLIC ${JANSSON_LIBRARY})
target_link_libraries(celix_dfi PRIVATE Celix::utils ${FFI_LIBRARIES})
set_target_properties(celix_dfi PROPERTIES "SOVERSION" 1)

add_library(celix_dfi_static STATIC ${SOURCES})
set_target_properties(celix_dfi_static PROPERTIES OUTPUT_NAME "celix_dfi_static")
target_include_directories(celix_dfi_static PUBLIC include ${JANSSON_INCLUDE_DIRS})
target_include_directories(celix_dfi_static PRIVATE src ${FFI_INCLUDE_DIRS})
target_link_libraries(celix_dfi_static PUBLIC ${JANSSON_LIBRARY})
target_link_libraries(celix_dfi_static PRIVATE Celix::utils ${FFI_LIBRARIES})

install(TARGETS celix_dfi celix_dfi_static DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)

install(TARGETS celix_dfi DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)
FILE(GLOB files "include/*.h")
INSTALL(FILES ${files} DESTINATION include/celix/dfi COMPONENT framework)

#Alias setup to match external usage
add_library(Celix::dfi ALIAS celix_dfi)
add_library(Celix::dfi_static ALIAS celix_dfi_static)

if (ENABLE_TESTING)
    find_package(CppUTest REQUIRED)
        
    include_directories(${CPPUTEST_INCLUDE_DIR})

	SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
    SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/dfi" "${PROJECT_BINARY_DIR}/utils")

	add_executable(test_dfi
		test/dyn_type_tests.cpp
		test/dyn_function_tests.cpp
		test/dyn_closure_tests.cpp
		test/dyn_interface_tests.cpp
		test/dyn_message_tests.cpp
		test/json_serializer_tests.cpp
		test/json_rpc_tests.cpp
		test/run_tests.cpp
	)
	target_link_libraries(test_dfi PRIVATE Celix::dfi Celix::utils ${FFI_LIBRARIES} ${CPPUTEST_LIBRARY})
	target_include_directories(test_dfi PRIVATE ${FFI_INCLUDE_DIRS})

    file(COPY ${CMAKE_CURRENT_LIST_DIR}/test/schemas DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/test/descriptors DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

	add_test(NAME run_test_dfi COMMAND test_dfi)
	SETUP_TARGET_FOR_COVERAGE(test_dfi_cov test_dfi ${CMAKE_BINARY_DIR}/coverage/test_dfi/test_dfi)
endif(ENABLE_TESTING)

