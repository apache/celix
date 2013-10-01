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

find_package(APR REQUIRED)
find_package(Jansson REQUIRED)

include_directories(${JANSSON_INCLUDE_DIRS})
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_remote_service_admin")
SET_HEADERS("Bundle-Name: Apache Celix Remote Service Admin")

bundle(remote_service_admin SOURCES 
	private/src/remote_service_admin_impl
	private/src/export_registration_impl
	private/src/import_registration_impl
	private/src/remote_service_admin_activator
	private/src/mongoose.c

    private/include/export_registration_impl.h
    private/include/import_registration_impl.h
    private/include/mongoose.h
    private/include/remote_service_admin_impl.h
)
    
install_bundle(remote_service_admin
	HEADERS
	    public/include/remote_endpoint_impl.h
		public/include/remote_endpoint.h
		public/include/remote_proxy.h
		public/include/remote_service_admin.h
)

target_link_libraries(remote_service_admin celix_framework ${APRUTIL_LIBRARY} ${JANSSON_LIBRARIES})
