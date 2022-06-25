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

celix_subproject(SHELL_BONJOUR "Option to enable building the Bonjour Shell (shell access by chat clients)" OFF DEPS SHELL)
if (SHELL_BONJOUR)
	message(WARNING "Celix::bonjour_shell is considered unstable, because develop of this bundle has been dormant for a while")
	find_package(LibXml2 REQUIRED)
	
	#TODO create/add FindDNS_SD.cmake and use it (with required)
	find_library(DNS_SD_LIB NAMES dns_sd dns_services)
	find_path(DNS_SD_INCLUDE_DIR dns_sd.h)

	set(BUNDLE_SYMBOLICNAME "bonjour_shell")
	set(BUNDLE_VERSION "0.1.0")
	set(BUNDLE_NAME "bonjour_shell")

	add_celix_bundle(bonjour_shell
		VERSION "1.0.0"
		FILENAME celix_bonjour_shell
		GROUP "Celix/Shell"
		SOURCES
		 	private/src/activator.c
		 	private/src/bonjour_shell.c
	)
	add_library(Celix::bonjour_shell ALIAS bonjour_shell)


	target_include_directories(bonjour_shell PRIVATE
			"${PROJECT_SOURCE_DIR}/utils/public/include"
			"${LIBXML2_INCLUDE_DIR}"
			"${DNS_SD_INCLUDE_DIR}"
			private/include
	)
	target_link_libraries(bonjour_shell PRIVATE ${LIBXML2_LIBRARIES} ${DNS_SD_LIB} Celix::shell_api)

	add_celix_container("bonjour_shell_deploy" BUNDLES
		Celix::shell
		bonjour_shell
		PROPERTIES "bonjour.shell.id=Apache Celix"
	)
endif (SHELL_BONJOUR)

