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
celix_subproject(EXAMPLES "Option to enable building the Examples" "OFF" DEPS FRAMEWORK DEPENDENCY_MANAGER2 LAUNCHER SHELL_TUI LOG_SERVICE LOG_WRITER)
if (EXAMPLES)
    add_subdirectory(hello_world)
    add_subdirectory(hello_world_test)

    if (NOT ANDROID)
    	add_subdirectory(mongoose)
    endif()
 
    add_subdirectory(whiteboard)
    add_subdirectory(echo_service)
    add_subdirectory(producer_consumer)
    add_subdirectory(dm_example)
    
    add_subdirectory(osgi-in-action/chapter04-correct-lookup)
    add_subdirectory(osgi-in-action/chapter04-correct-listener)
    add_subdirectory(osgi-in-action/chapter01-greeting-example)
    #add_subdirectory(osgi-in-action/chapter04-paint-example) chapter4 example is still based on APR
    add_subdirectory(locking)
    
    add_subdirectory(embedding)

    add_deploy(chapter01-greeting-example-d BUNDLES shell shell_tui log_service chapter01-greeting-example-client chapter01-greeting-example)
    add_deploy(chapter04-correct-lookup-d BUNDLES shell shell_tui log_service chapter04-correct-lookup)
    add_deploy(chapter04-correct-listener-d BUNDLES shell shell_tui log_service chapter04-correct-listener)

    #deploy("hello_world" BUNDLES shell shell_tui apache_celix_examples_hello_world hello_world_test log_service log_writer)
    add_deploy("wb" BUNDLES tracker publisherA publisherB shell shell_tui log_service log_writer)
    add_deploy("wb_dp" BUNDLES tracker_depman publisherA publisherB shell shell_tui log_service log_writer dm_shell)
    add_deploy("echo" BUNDLES echo_server echo_client shell shell_tui)
    add_deploy("producer_consumer" BUNDLES producer consumer database shell shell_tui)
    if (NOT ANDROID)
        add_deploy("mongoose_deploy" BUNDLES shell shell_tui log_service mongoose)
    endif ()

endif(EXAMPLES)
