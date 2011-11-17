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

add_definitions(-DUSE_FILE32API)

include_directories("/opt/local/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/deployment_admin/private/include")

link_directories("/opt/local/lib")
bundle(deployment_admin SOURCES
	private/src/deployment_package 
	private/src/deployment_admin
	private/src/deployment_admin_activator
	private/src/ioapi
	private/src/miniunz
	private/src/unzip
)
target_link_libraries(deployment_admin framework curl)

deploy("deployment-admin" BUNDLES deployment_admin shell shell_tui log_service log_writer)
