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


find_package(LibXml2 REQUIRED)

add_library(discovery_common STATIC
		src/discovery.c
		src/discovery_activator.c
		src/endpoint_descriptor_reader.c
		src/endpoint_descriptor_writer.c
		src/endpoint_discovery_poller.c
		src/endpoint_discovery_server.c
        src/civetweb.c
)
target_include_directories(discovery_common PUBLIC include)
target_include_directories(discovery_common PRIVATE src ${LIBXML2_INCLUDE_DIR})
target_link_libraries(discovery_common PRIVATE ${LIBXML2_LIBRARIES} Celix::framework)
target_link_libraries(discovery_common PUBLIC Celix::log_helper Celix::remote_service_admin_api)