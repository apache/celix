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


#TODO refactor and improve impl
#celix_subproject(CONFIG_ADMIN "Option to enable building the Config Admin Service bundle and its examples" OFF DEPS framework launcher shell_tui log_writer)
set(CONFIG_AMDIN FALSE)
if (CONFIG_ADMIN)

	add_subdirectory(service)
    add_subdirectory(example)
	
	if (ENABLE_TESTING)
		find_package(CppUTest REQUIRED)

	    include_directories(SYSTEM PRIVATE ${CppUTest_INCLUDE_DIR})
	    add_subdirectory(config_admin_tst)
#		find_package(CppUTest REQUIRED)
#
#	    include_directories(${CUNIT_INCLUDE_DIRS})
#	    include_directories(${CppUTest_INCLUDE_DIR})
#	    include_directories("${PROJECT_SOURCE_DIR}/config_admin/service/public/include")
#	    include_directories("${PROJECT_SOURCE_DIR}/config_admin/service/private/include")
#	    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
#	    
#	    add_executable(config_admin_test config_admin_tst/config_admin_test.cpp)
#	    target_link_libraries(config_admin_test celix_utils ${CppUTest_LIBRARY} pthread)
#	    
#	    
#		add_test(NAME run_config_admin_test COMMAND config_admin_test)
#      	setup_target_for_coverage(config_admin_test config_admin_test ${CMAKE_BINARY_DIR}/coverage/config_admin_test/config_admin_test)

   endif(ENABLE_TESTING)
	

   add_celix_container(config_admin_deploy
       NAME "config_admin"
       BUNDLES 
        config_admin 
        Celix::shell
	    Celix::shell_tui
        log_service 
        log_writer 
        config_admin_example
   )

endif (CONFIG_ADMIN)
