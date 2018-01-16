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

celix_subproject(RSA_DISCOVERY_SHM "Option to enable building the Discovery (SHM) bundle" OFF)
if (RSA_DISCOVERY_SHM)
find_package(CURL REQUIRED)
	find_package(LibXml2 REQUIRED)
	
	include_directories("${CURL_INCLUDE_DIR}")
	include_directories("${LIBXML2_INCLUDE_DIR}")
	include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/private/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/discovery/private/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/discovery_shm/private/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
	include_directories("private/include")

	add_celix_bundle(discovery_shm
        VERSION 0.0.1
        SYMBOLIC_NAME "apache_celix_rsa_discovery_shm"
        NAME "Apache Celix RSA Discovery SHM"
        SOURCES
		private/src/discovery_shm
		private/src/discovery_shmWatcher
		private/src/discovery_impl
		${PROJECT_SOURCE_DIR}/remote_services/discovery/private/src/discovery_activator.c
		${PROJECT_SOURCE_DIR}/remote_services/discovery/private/src/discovery.c
		${PROJECT_SOURCE_DIR}/remote_services/discovery/private/src/endpoint_descriptor_reader.c
		${PROJECT_SOURCE_DIR}/remote_services/discovery/private/src/endpoint_descriptor_writer.c
		${PROJECT_SOURCE_DIR}/remote_services/discovery/private/src/endpoint_discovery_poller.c
		${PROJECT_SOURCE_DIR}/remote_services/discovery/private/src/endpoint_discovery_server.c
	    ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/endpoint_description.c
	    ${PROJECT_SOURCE_DIR}/remote_services/utils/private/src/civetweb.c
	    ${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c
	)

	install_celix_bundle(discovery_shm)
		
	target_link_libraries(discovery_shm celix_framework ${CURL_LIBRARIES} ${LIBXML2_LIBRARIES})

endif (RSA_DISCOVERY_SHM)
