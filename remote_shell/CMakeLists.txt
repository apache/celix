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
celix_subproject(REMOTE_SHELL "Option to enable building the Remote Shell bundles" OFF DEPS LAUNCHER SHELL)
if (REMOTE_SHELL)

	SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_remote_shell")
	SET_HEADERS("Bundle-Name: Apache Celix Remote Shell") 
	

    bundle(remote_shell SOURCES 
    	private/src/activator 
    	private/src/connection_listener 
    	private/src/shell_mediator 
    	private/src/remote_shell
	)
    
 
    include_directories("private/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/shell/public/include")
    
    target_link_libraries(remote_shell celix_framework apr-1 aprutil-1)
    
    package(remote_shell)
    
    #INSTALL(FILES ${PROJECT_BINARY_DIR}/bundles/remote_shell.zip DESTINATION share/celix/bundles COMPONENT framework)
    
    #deploy("remote_shell" BUNDLES shell remote_shell shell_tui log_service)
endif (REMOTE_SHELL)