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

include_directories(
        ${FFI_INCLUDE_DIRS}
        ${JANSSON_INCLUDE_DIRS}
	    private/include
	    public/include
	    ${PROJECT_SOURCE_DIR}/utils/public/include
)

set(MEMSTREAM_SOURCES )
set(MEMSTREAM_INCLUDES )
if (APPLE OR ANDROID)
	set(MEMSTREAM_SOURCES ${PROJECT_SOURCE_DIR}/utils/private/src/memstream/open_memstream.c  ${PROJECT_SOURCE_DIR}/utils/private/src/memstream/fmemopen.c)
	set(MEMSTREAM_INCLUDES ${PROJECT_SOURCE_DIR}/utils/public/include/memstream/open_memstream.h ${PROJECT_SOURCE_DIR}/utils/public/include/memstream/fmemopen.h)
    include_directories(${PROJECT_SOURCE_DIR}/utils/public/include/memstream)
endif()

add_library(celix_dfi SHARED
    private/src/dyn_common.c
    private/src/dyn_type.c
    private/src/dyn_function.c
    private/src/dyn_interface.c
    private/src/dyn_message.c
    private/src/json_serializer.c
    private/src/json_rpc.c
    ${MEMSTREAM_SOURCES}

    public/include/dyn_common.h
    public/include/dyn_type.h
    public/include/dyn_function.h
    public/include/dyn_interface.h
    public/include/dyn_message.h
    public/include/json_serializer.h
    public/include/json_rpc.h
    ${MEMSTREAM_INCLUDES}
)
set_target_properties(celix_dfi PROPERTIES "SOVERSION" 1)
set_target_properties(celix_dfi PROPERTIES "VERSION" 1.1.0)
target_link_libraries(celix_dfi celix_utils ${FFI_LIBRARIES} ${JANSSON_LIBRARY})

install(TARGETS celix_dfi DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)
FILE(GLOB files "public/include/*.h" ${MEMSTREAM_INCLUDES})
INSTALL(FILES ${files} DESTINATION include/celix/dfi COMPONENT framework)


if (ENABLE_TESTING)
    find_package(CppUTest REQUIRED)
        
    include_directories(${CPPUTEST_INCLUDE_DIR})

	SET(CMAKE_SKIP_BUILD_RPATH  FALSE) #TODO needed?
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) #TODO needed?
    SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/dfi" "${PROJECT_BINARY_DIR}/utils")

	add_executable(test_dfi
		private/test/dyn_type_tests.cpp
		private/test/dyn_function_tests.cpp
		private/test/dyn_closure_tests.cpp
		private/test/dyn_interface_tests.cpp
		private/test/dyn_message_tests.cpp
		private/test/json_serializer_tests.cpp
		private/test/json_rpc_tests.cpp
		private/test/run_tests.cpp
	)
	target_link_libraries(test_dfi celix_dfi ${FFI_LIBRARIES} ${CPPUTEST_LIBRARY})

    file(COPY ${CMAKE_CURRENT_LIST_DIR}/private/test/schemas DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/private/test/descriptors DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

	add_test(NAME run_test_dfi COMMAND test_dfi)
	SETUP_TARGET_FOR_COVERAGE(test_dfi_cov test_dfi ${CMAKE_BINARY_DIR}/coverage/test_dfi/test_dfi)
endif(ENABLE_TESTING)

