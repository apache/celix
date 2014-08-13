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

find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)

include_directories("${CURL_INCLUDE_DIR}")
include_directories("${LIBXML2_INCLUDE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/private/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/discovery_configured/private/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
include_directories(private/include)

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_rsa_discovery_configured")
SET_HEADERS("Bundle-Name: Apache Celix RSA Configured Discovery")

bundle(discovery_configured SOURCES 
	private/src/discovery.c
	private/src/discovery_activator.c
    private/src/endpoint_descriptor_reader.c
    private/src/endpoint_descriptor_writer.c
	private/src/endpoint_discovery_poller.c
    private/src/endpoint_discovery_server.c
    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c
    ${PROJECT_SOURCE_DIR}/remote_services/utils/private/src/civetweb.c
)
    
install_bundle(discovery_configured)

target_link_libraries(discovery_configured celix_framework ${CURL_LIBRARIES} ${LIBXML2_LIBRARIES} ${APRUTIL_LIBRARY})

if (RSA_ENDPOINT_TEST_READER)
    add_executable(descparser 
        private/src/endpoint_descriptor_reader.c
        ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c)
        
    target_link_libraries(descparser ${LIBXML2_LIBRARIES} celix_framework celix_utils)
endif (RSA_ENDPOINT_TEST_READER)

if (RSA_ENDPOINT_TEST_WRITER)
    add_executable(descwriter 
        private/src/endpoint_descriptor_writer.c
        ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c)
        
    target_link_libraries(descwriter ${LIBXML2_LIBRARIES} celix_framework celix_utils)
endif(RSA_ENDPOINT_TEST_WRITER)