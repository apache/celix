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

if (CELIX_INSTALL_DEPRECATED_API)
	set(SOURCES src/celix_log_helper.c src/log_helper.c)
else ()
	set(SOURCES src/celix_log_helper.c)
endif ()

add_library(log_helper STATIC ${SOURCES})
set_target_properties(log_helper PROPERTIES OUTPUT_NAME "celix_log_utils")
target_include_directories(log_helper PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
	$<INSTALL_INTERFACE:include/celix/log_helper>
)
target_link_libraries(log_helper PUBLIC Celix::utils Celix::log_service_api)
install(TARGETS log_helper EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT logging)
install(DIRECTORY include/ DESTINATION include/celix/log_helper COMPONENT logging)

if (CELIX_INSTALL_DEPRECATED_API)
	target_include_directories(log_helper PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include_v1>
		$<INSTALL_INTERFACE:include/celix/log_helper>
	)
	install(DIRECTORY include_v1/ DESTINATION include/celix/log_helper)
endif ()

add_library(Celix::log_helper ALIAS log_helper)

if (ENABLE_TESTING)
	add_subdirectory(gtest)
endif()
