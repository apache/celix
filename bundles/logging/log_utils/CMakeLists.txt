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


add_library(log_utils STATIC
		src/celix_log_utils.c
		src/celix_log_helper.c
)
set_target_properties(log_utils PROPERTIES OUTPUT_NAME "celix_log_utils")
target_include_directories(log_utils PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
		$<INSTALL_INTERFACE:include/celix/log_utils>
)
target_link_libraries(log_utils PUBLIC Celix::utils Celix::log_service_api)
install(TARGETS log_utils EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT logging)
install(DIRECTORY include/ DESTINATION include/celix/log_utils COMPONENT logging)

add_library(Celix::log_utils ALIAS log_utils)

if (ENABLE_TESTING)
	add_subdirectory(gtest)
endif()
