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

celix_subproject(RSA_BUNDLES_DISCOVERY_SLP "Option to enable building the Discovery SLP bundle" ON DEPS LAUNCHER topology_manager remote_service_admin)
if (RSA_BUNDLES_DISCOVERY_SLP)
	find_package(Slp REQUIRED)

	include_directories(${SLP_INCLUDE_DIRS})
	include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/discovery_slp/private/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
	include_directories("private/include")

	SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_rsa_discovery_slp")
	SET_HEADERS("Bundle-Name: Apache Celix RSA SLP Discovery")

	bundle(discovery_slp SOURCES 
		private/src/discovery 
		private/src/discovery_activator
	)
	    
	install_bundle(discovery_slp)

	target_link_libraries(discovery_slp celix_framework ${APRUTIL_LIBRARY} ${SLP_LIBRARIES})

endif (RSA_BUNDLES_DISCOVERY_SLP)
