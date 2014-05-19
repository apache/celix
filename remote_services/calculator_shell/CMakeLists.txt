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

SET(BUNDLE_SYMBOLICNAME "apache_celix_remoting_calculator_shell")
SET(BUNDLE_VERSION "0.0.1")

bundle(calculator_shell SOURCES
	private/src/add_command 
	private/src/sub_command
	private/src/sqrt_command
	private/src/calculator_shell_activator
    
    private/include/add_command.h
    private/include/sqrt_command.h
    private/include/sub_command.h
    ${PROJECT_SOURCE_DIR}/shell/public/src/command.c
)

include_directories("${PROJECT_SOURCE_DIR}/remote_services/calculator_shell/private/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/remote_services/calculator_service/public/include")
include_directories("${PROJECT_SOURCE_DIR}/shell/public/include")
target_link_libraries(calculator_shell celix_framework)

