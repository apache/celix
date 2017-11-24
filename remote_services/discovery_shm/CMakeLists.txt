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

	add_bundle(discovery_shm 
        VERSION 0.0.1
        SYMBOLIC_NAME "apache_celix_rsa_discovery_shm"
        NAME "Apache Celix RSA Discovery SHM"
        SOURCES
			src/discovery_shm.c
			src/discovery_shmWatcher.c
			src/discovery_impl.c
			$<TARGET_OBJECTS:discovery_common>
			$<TARGET_OBJECTS:civetweb>
	)
	target_include_directories(discovery_shm PRIVATE
			src
			${LIBXML2_INCLUDE_DIR}
			${CURL_INCLUDE_DIR}
			$<TARGET_PROPERTY:discovery_common,INCLUDE_DIRECTORIES>
			$<TARGET_PROPERTY:civetweb,INCLUDE_DIRECTORIES>
	)
	target_link_libraries(discovery_shm PRIVATE Celix::framework ${CURL_LIBRARIES} ${LIBXML2_LIBRARIES})

	install_bundle(discovery_shm)
		


endif (RSA_DISCOVERY_SHM)
