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

    bundle(shell SOURCES shell command
    	ps_command start_command stop_command
    	install_command update_command
    	uninstall_command log_command
    	inspect_command)
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    target_link_libraries(shell celix_framework ${CURL_LIBRARIES})
    
    package(shell FILES shell.h command.h)
    
    FILE(GLOB files shell.h command.h)
    INSTALL(FILES ${files} DESTINATION include/celix/shell COMPONENT framework)
    INSTALL(FILES ${PROJECT_BINARY_DIR}/bundles/shell.zip DESTINATION bundles COMPONENT framework)
endif (SHELL)