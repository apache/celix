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

#TODO find_package(DNS-SD REQUIRED)

include_directories("/usr/include") #TODO check if this has impact on the generated project indexer paths
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/discovery/private/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
include_directories(private/include)

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_rsa_discovery_bonjour")
SET_HEADERS("Bundle-Name: Apache Celix RSA BONJOUR Discovery")

bundle(discovery_bonjour SOURCES 
	private/src/discovery.c
	private/src/discovery_activator.c
)

target_link_libraries(discovery_bonjour celix_framework ${APRUTIL_LIBRARY}) # TODO ${SLP_LIBRARIES})

