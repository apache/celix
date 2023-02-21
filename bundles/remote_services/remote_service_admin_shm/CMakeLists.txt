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
celix_subproject(RSA_REMOTE_SERVICE_ADMIN_SHM "Option to enable building the Remote Service Admin Service SHM bundle" OFF)
if (RSA_REMOTE_SERVICE_ADMIN_SHM)

	find_package(libuuid REQUIRED)


	link_directories("/opt/local/lib")
	include_directories("/opt/local/include")
	include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/framework/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/framework/private/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/include")	
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin_shm/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin_shm/private/include")
	include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")

	add_celix_bundle(remote_service_admin_shm
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_remote_service_admin_shm"
        NAME "Apache Celix Remote Service Admin SHM"
		GROUP "Celix/RSA"
        SOURCES

		private/src/remote_service_admin_impl
        private/src/remote_service_admin_activator
        ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/export_registration_impl
        ${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/private/src/import_registration_impl
        ${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c
	)
	target_link_libraries(remote_service_admin_shm Celix::framework)

       install_celix_bundle(remote_service_admin_shm EXPORT celix COMPONENT rsa)
       add_library(Celix::remote_service_admin_shm ALIAS remote_service_admin_shm)

		if (ENABLE_TESTING)
			find_package(CppUTest REQUIRED)
			include_directories(SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR})
			add_subdirectory(private/test)
		endif()

endif (RSA_REMOTE_SERVICE_ADMIN_SHM)

