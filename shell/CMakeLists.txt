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
celix_subproject(SHELL "Option to enable building the Shell bundles" ON DEPS LAUNCHER LOG_SERVICE)
if (SHELL)
	find_package(CURL REQUIRED)

    add_celix_bundle(shell
        SYMBOLIC_NAME "apache_celix_shell"
        VERSION "2.1.0"
        NAME "Apache Celix Shell"

        SOURCES

          private/src/activator
          private/src/shell
          private/src/lb_command
          private/src/start_command
          private/src/stop_command
          private/src/install_command
          private/src/update_command
          private/src/uninstall_command
          private/src/log_command
          private/src/inspect_command
          private/src/help_command

          ${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c

    )
    
    install_celix_bundle(shell
    	HEADERS
    		public/include/shell.h public/include/command.h public/include/shell_constants.h
	)

	include_directories("public/include")
	include_directories("private/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
	include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(shell celix_framework ${CURL_LIBRARIES})
endif (SHELL)
