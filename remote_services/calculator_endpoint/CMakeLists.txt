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

find_package(Jansson REQUIRED)

include_directories(${JANSSON_INCLUDE_DIRS})
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/calculator_endpoint/private/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/calculator_service/public/include")

SET(BUNDLE_SYMBOLICNAME "apache_celix_remoting_calculator_endpoint")
SET(BUNDLE_VERSION "0.0.1")

bundle(org.example.api.Calculator_endpoint SOURCES 
	private/src/calculator_endpoint_activator
	private/src/calculator_endpoint_impl.c
    
    private/include/calculator_endpoint_impl.h
)

target_link_libraries(org.example.api.Calculator_endpoint celix_framework ${JANSSON_LIBRARIES})
