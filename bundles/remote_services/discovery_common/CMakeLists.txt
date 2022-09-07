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

add_library(rsa_discovery_common OBJECT
		src/discovery.c
		src/discovery_activator.c
		src/endpoint_descriptor_reader.c
		src/endpoint_descriptor_writer.c
		src/endpoint_discovery_poller.c
		src/endpoint_discovery_server.c
)
target_include_directories(rsa_discovery_common PUBLIC
		include src
		$<TARGET_PROPERTY:Celix::framework,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Celix::utils,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Celix::log_helper,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:Celix::c_rsa_spi,INTERFACE_INCLUDE_DIRECTORIES>
		$<TARGET_PROPERTY:civetweb,INCLUDE_DIRECTORIES>
		${CURL_INCLUDE_DIRS}
		${LIBXML2_INCLUDE_DIR})

#Setup target aliases to match external usage
add_library(Celix::rsa_discovery_common ALIAS rsa_discovery_common)
