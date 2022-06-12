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

add_library(log_service_api INTERFACE)
target_include_directories(log_service_api INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)
target_link_libraries(log_service_api INTERFACE Celix::utils Celix::framework)

if (CELIX_INSTALL_DEPRECATED_API)
	target_include_directories(log_service_api INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include_v2>
	)
	install(DIRECTORY include_v2/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/log_service)
endif ()

install(TARGETS log_service_api EXPORT celix
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/log_service)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/log_service)

add_library(Celix::log_service_api ALIAS log_service_api)
