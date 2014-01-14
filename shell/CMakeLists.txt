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
celix_subproject(SHELL "Option to enable building the Shell bundles" "OFF" DEPS LAUNCHER LOG_SERVICE)
if (SHELL)
	find_package(CURL REQUIRED)
	
	SET_HEADER(BUNDLE_VERSION "1.0.0.incubating")
	SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_shell")
	SET_HEADERS("Bundle-Name: Apache Celix Shell") 

    bundle(shell SOURCES
    	public/src/command
     
    	private/src/shell 
    	private/src/ps_command 
    	private/src/start_command 
    	private/src/stop_command
    	private/src/install_command 
    	private/src/update_command
    	private/src/uninstall_command 
    	private/src/log_command
    	private/src/inspect_command
    	private/src/help_command
        
        private/include/inspect_command.h
        private/include/install_command.h
        private/include/log_command.h
        private/include/ps_command.h
        private/include/shell_private.h
        private/include/start_command.h
        private/include/stop_command.h
        private/include/uninstall_command.h
        private/include/update_command.h
        private/include/help_command.h
    )
    
    install_bundle(shell
    	HEADERS
    		public/include/shell.h public/include/command.h public/include/command_impl.h
    	RESOURCES 
    		public/src/command.c
	)

	include_directories("public/include")
	include_directories("private/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
	include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(shell celix_framework ${CURL_LIBRARIES})
endif (SHELL)