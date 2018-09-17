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

    add_celix_bundle(remote_shell
     	SYMBOLIC_NAME "apache_celix_remote_shell"
     	VERSION "0.0.2"
     	NAME "Apache Celix Remote Shell"
		GROUP "Celix/Shell"
		SOURCES
			src/activator
			src/connection_listener
			src/shell_mediator
			src/remote_shell
	)

	target_include_directories(remote_shell PRIVATE src)
	target_link_libraries(remote_shell PRIVATE log_helper)

    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    
    target_link_libraries(remote_shell PRIVATE Celix::shell_api)

        install_celix_bundle(remote_shell EXPORT celix COMPONENT remote_shell)
	#Alias setup to match external usage
	add_library(Celix::remote_shell ALIAS remote_shell)

    add_celix_container("remote_shell_deploy" NAME "remote_shell"  BUNDLES Celix::shell Celix::remote_shell Celix::shell_tui log_service)
endif (REMOTE_SHELL)
