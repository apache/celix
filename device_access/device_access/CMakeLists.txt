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

add_bundle(device_manager
	SYMBOLIC_NAME "apache_celix_device_manager"
	VERSION "0.0.2"
	NAME "Apache Celix Device Access Device Manager"
	SOURCES
		private/src/activator
		private/src/driver_attributes
		private/src/device_manager
		private/src/driver_loader
		private/src/driver_matcher

		${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c

		private/include/device_manager.h
		private/include/driver_attributes.h
		private/include/driver_loader.h
		private/include/driver_matcher.h
)

install_bundle(device_manager
	HEADERS
		public/include/device.h
		public/include/driver_locator.h
		public/include/driver_selector.h
		public/include/driver.h
		public/include/match.h
)

include_directories("${PROJECT_SOURCE_DIR}/framework/public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")

include_directories("public/include")
include_directories("private/include")

target_link_libraries(device_manager celix_framework celix_utils)
