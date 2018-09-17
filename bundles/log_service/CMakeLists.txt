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

celix_subproject(LOG_SERVICE "Option to enable building the Log Service bundles" ON DEPS framework)
if (LOG_SERVICE)

	add_library(log_service_api INTERFACE)
	target_include_directories(log_service_api INTERFACE
			$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
			$<INSTALL_INTERFACE:include/celix/log_service>
	)
	install(TARGETS log_service_api EXPORT celix)
	install(DIRECTORY include/ DESTINATION include/celix/log_service)

	add_library(log_helper STATIC src/log_helper.c)
	set_target_properties(log_helper PROPERTIES OUTPUT_NAME "celix_log_helper")
	target_include_directories(log_helper PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/loghelper_include>
			$<INSTALL_INTERFACE:include/celix/log_helper>
	)
	target_link_libraries(log_helper PUBLIC Celix::framework log_service_api)
	install(TARGETS log_helper EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT log_service)
	install(DIRECTORY loghelper_include/ DESTINATION include/celix/log_helper COMPONENT log_service)

    add_celix_bundle(log_service
    	SYMBOLIC_NAME "apache_celix_log_service"
    	NAME "Apache Celix Log Service"
		GROUP "Celix/Logging"
    	VERSION "1.1.0"
    	SOURCES
			src/log
			src/log_entry
			src/log_factory
			src/log_service_impl
			src/log_service_activator
			src/log_reader_service_impl
    )
    target_include_directories(log_service PRIVATE src)
    target_link_libraries(log_service PRIVATE log_service_api)
    install_celix_bundle(log_service EXPORT celix COMPONENT log_service)

    #Setup target aliases to match external usage
    add_library(Celix::log_service_api ALIAS log_service_api)
    add_library(Celix::log_service ALIAS log_service)
    add_library(Celix::log_helper ALIAS log_helper)
endif (LOG_SERVICE)
