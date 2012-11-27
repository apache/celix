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
celix_subproject(EXAMPLES "Option to enable building the Examples" "OFF" DEPS FRAMEWORK LAUNCHER SHELL_TUI LOG_WRITER DEPENDENCY_MANAGER)
if (EXAMPLES)
    add_subdirectory(hello_world)
    add_subdirectory(mongoose)
    
    add_subdirectory(whiteboard)
    add_subdirectory(echo_service)
    
    add_subdirectory(osgi-in-action/chapter04-correct-lookup)
    add_subdirectory(osgi-in-action/chapter04-correct-listener)
    add_subdirectory(osgi-in-action/chapter01-greeting-example)
    add_subdirectory(osgi-in-action/chapter04-paint-example)
    
    deploy(chapter01-greeting-example BUNDLES shell shell_tui log_service chapter01-greeting-example-client chapter01-greeting-example)
    deploy(chapter04-correct-listener BUNDLES shell shell_tui log_service chapter04-correct-listener)
    
    deploy("hello_world" BUNDLES shell shell_tui hello_world log_service)
    deploy("wb" BUNDLES tracker publisherA publisherB shell shell_tui log_service log_writer)
    deploy("wb_dp" BUNDLES tracker_depman publisherA publisherB shell shell_tui log_service log_writer)
    deploy("echo" BUNDLES echo_server echo_client shell shell_tui log_service log_writer)
endif(EXAMPLES)
