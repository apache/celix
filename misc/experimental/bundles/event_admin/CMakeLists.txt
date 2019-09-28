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

celix_subproject(EVENT_ADMIN "Option to enable building the Event Admin Service bundles" OFF DEPS LAUNCHER LOG_SERVICE SHELL shell_tui)
if(EVENT_ADMIN)

	#add_subdirectory(utils)
	add_subdirectory(event_admin)
	add_subdirectory(event_handler)
	add_subdirectory(event_publisher)

	#deploy("event_admin_bundle" BUNDLES event_admin shell shell_tui log_service log_writer)
  #  deploy("event_publisher_example" BUNDLES event_admin event_publisher shell shell_tui log_service log_writer)
#	deploy("event_handler_example" BUNDLES event_admin event_handler shell shell_tui log_service log_writer)
#	deploy("event_admin_example" BUNDLES event_admin event_publisher event_handler shell shell_tui log_service log_writer)
	add_celix_container(event_admin_service
			NAME "event_admin_service"
			GROUP "event_admin/event_admin"
			BUNDLES event_admin shell shell_tui log_service log_writer
			)
	add_celix_container(event_handler_consumer
			NAME "event_handler_consumer"
			GROUP "event_handler"
			BUNDLES event_handler event_admin shell shell_tui log_service log_writer)
	add_celix_container(event_publisher_consumer
			NAME "event_publisher_consumer"
			GROUP "event_publisher"
			BUNDLES event_publisher event_handler event_admin shell shell_tui log_service log_writer)
endif(EVENT_ADMIN)
