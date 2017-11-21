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

celix_subproject(RSA_DISCOVERY_ETCD "Option to enable building the Discovery (ETCD) bundle" ON)
if (RSA_DISCOVERY_ETCD)
	find_package(CURL REQUIRED)
	find_package(LibXml2 REQUIRED)
	find_package(Jansson REQUIRED)
    
	add_bundle(discovery_etcd 
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_rsa_discovery_etcd"
        NAME "Apache Celix RSA Discovery ETCD"
        SOURCES
			src/discovery_impl.c
	    	src/etcd_watcher.c
	)
	target_link_libraries(discovery_etcd PRIVATE Celix::log_helper)
	target_include_directories(discovery_etcd PRIVATE src )
	target_include_directories(discovery_etcd PRIVATE ${CURL_INCLUDE_DIR} ${JANSSON_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
	target_link_libraries(discovery_etcd PRIVATE ${CURL_LIBRARIES} ${LIBXML2_LIBRARIES} ${JANSSON_LIBRARIES} discovery_common)

	install_bundle(discovery_etcd)
endif (RSA_DISCOVERY_ETCD)
