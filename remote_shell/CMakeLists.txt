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
celix_subproject(REMOTE_SHELL "Option to enable building the Remote Shell bundles" ON DEPS LAUNCHER SHELL_TUI)
if (REMOTE_SHELL)

    add_bundle(remote_shell
     	SYMBOLIC_NAME "apache_celix_remote_shell"
     	VERSION "0.0.1"
     	NAME: "Apache Celix Remote Shell"
		SOURCES
			private/src/activator
			private/src/connection_listener
			private/src/shell_mediator
			private/src/remote_shell

			${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c

			private/include/remote_shell.h
			private/include/connection_listener.h
	)
	
	install_bundle(remote_shell)
 
    include_directories("private/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/shell/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    
    target_link_libraries(remote_shell celix_framework)

    add_deploy("remote_shell_deploy" NAME "remote_shell"  BUNDLES shell remote_shell shell_tui log_service)
endif (REMOTE_SHELL)
