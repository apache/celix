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
	add_celix_bundle(rsa_discovery_etcd
			VERSION 0.9.0
			SYMBOLIC_NAME "apache_celix_rsa_discovery_etcd"
			NAME "Apache Celix RSA Discovery ETCD"
			GROUP "Celix/RSA"
			FILENAME celix_rsa_discovery_etcd
			SOURCES
			src/discovery_impl.c
			src/etcd_watcher.c
			)
	target_link_libraries(rsa_discovery_etcd PRIVATE Celix::rsa_discovery_common Celix::log_helper Celix::etcdlib_static Celix::rsa_common)
	target_include_directories(rsa_discovery_etcd PRIVATE src)
	celix_deprecated_framework_headers(rsa_discovery_etcd)
	celix_deprecated_utils_headers(rsa_discovery_etcd)

	install_celix_bundle(rsa_discovery_etcd EXPORT celix COMPONENT rsa)
	#Setup target aliases to match external usage
	add_library(Celix::rsa_discovery_etcd ALIAS rsa_discovery_etcd)
endif (RSA_DISCOVERY_ETCD)
