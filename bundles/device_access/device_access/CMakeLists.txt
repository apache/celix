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


add_library(device_access_api INTERFACE)
target_include_directories(device_access_api INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include/celix/device_access>
)

add_celix_bundle(device_manager
	SYMBOLIC_NAME "apache_celix_device_manager"
	VERSION "0.0.2"
	NAME "Apache Celix Device Access Device Manager"
	GROUP "Celix/DeviceAccess"
	SOURCES
		src/activator
		src/driver_attributes
		src/device_manager
		src/driver_loader
		src/driver_matcher
)
target_link_libraries(device_manager PRIVATE Celix::log_helper)

target_include_directories(device_manager PRIVATE src)
target_link_libraries(device_manager PRIVATE device_access_api)

include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")

install(TARGETS device_access_api EXPORT celix COMPONENT device_access)
install(DIRECTORY include/ DESTINATION include/celix/device_access COMPONENT device_access)
install_celix_bundle(device_manager EXPORT celix COMPONENT device_access)

#Setup target aliases to match external usage
add_library(Celix::device_access_api ALIAS device_access_api)
add_library(Celix::device_manager ALIAS device_manager)

