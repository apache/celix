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

find_package(APR REQUIRED)

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_device_manager")
SET_HEADERS("Bundle-Name: Apache Celix Device Access Device Manager") 

bundle(device_manager SOURCES 
	private/src/activator
	private/src/driver_attributes 
	private/src/device_manager
	private/src/driver_loader
	private/src/driver_matcher
    
    private/include/device_manager.h
    private/include/driver_attributes.h
    private/include/driver_loader.h
    private/include/driver_matcher.h
)

FILE(GLOB SERVICE_HEADERS public/include/*.h)

#bundle_add(device_manager ${SERVICE_HEADERS} DESTINATION services)
#bundle_add(device_manager 
#	public/include DESTINATION public/include
#	PATTERN .svn EXCLUDE)

#bundle_install(device_manager COMPONENT device_access)

install(FILES ${SERVICE_HEADERS} DESTINATION include/celix/device_access)


include_directories("${PROJECT_SOURCE_DIR}/framework/public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")

include_directories("public/include")
include_directories("private/include")

target_link_libraries(device_manager celix_framework celix_utils ${APR_LIBRARY})
