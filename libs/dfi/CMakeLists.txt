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

find_package(libffi REQUIRED)
find_package(jansson REQUIRED)

set(SOURCES
	src/dyn_common.c
	src/dyn_type_common.c
	src/dyn_type.c
	src/dyn_avpr_type.c
	src/dyn_function.c
	src/dyn_avpr_function.c
	src/dyn_interface.c
	src/dyn_avpr_interface.c
	src/dyn_message.c
	src/json_serializer.c
	src/json_rpc.c
	src/avrobin_serializer.c
)

add_library(dfi SHARED ${SOURCES})
set_target_properties(dfi PROPERTIES OUTPUT_NAME "celix_dfi")
target_include_directories(dfi PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	$<INSTALL_INTERFACE:include/celix/dfi>
)

target_link_libraries(dfi PRIVATE libffi::libffi)
target_link_libraries(dfi PUBLIC jansson::jansson)
target_link_libraries(dfi PRIVATE Celix::utils)
set_target_properties(dfi PROPERTIES "SOVERSION" 1)

install(TARGETS dfi EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dfi
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/dfi)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/dfi COMPONENT dfi)

#Alias setup to match external usage
add_library(Celix::dfi ALIAS dfi)

if (ENABLE_TESTING)
	add_subdirectory(gtest)
endif(ENABLE_TESTING)

