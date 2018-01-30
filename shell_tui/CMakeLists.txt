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
celix_subproject(SHELL_TUI "Option to enable building the Shell Textual User Interface bundles" ON DEPS LAUNCHER SHELL)
if (SHELL_TUI)

    add_celix_bundle(shell_tui
    	SYMBOLIC_NAME "apache_celix_shell_tui"
    	VERSION "1.1.0"
    	NAME "Apache Celix Shell TUI"
    	SOURCES 
    		private/src/activator 
    		private/src/shell_tui
    		private/src/history
	)
	
	target_include_directories(shell_tui PRIVATE
			"${PROJECT_SOURCE_DIR}/utils/public/include"
			private/include
	)
    target_link_libraries(shell_tui PRIVATE Celix::shell_api)

	install_celix_bundle(shell_tui EXPORT celix)

	#Alias setup to match external usage
	add_library(Celix::shell_tui ALIAS shell_tui)
endif (SHELL_TUI)
