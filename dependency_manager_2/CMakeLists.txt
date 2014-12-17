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

celix_subproject(DEPENDENCY_MANAGER2 "Option to build the dependency manager static library" "OFF" DEPS framework)
if (DEPENDENCY_MANAGER2) 
    # Add -fPIC for x86_64 Unix platforms; this lib will be linked to a shared lib
    if(UNIX AND NOT WIN32)
      find_program(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin )
      if(CMAKE_UNAME)
        exec_program(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
        set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} CACHE INTERNAL "processor type (i386 and x86_64)")
         if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
          add_definitions(-fPIC)
        endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
      endif(CMAKE_UNAME)
    endif(UNIX AND NOT WIN32)
    
    CELIX_ADD_COMPONENT(dependency_manager2
    	DISPLAY_NAME Dependency Manager
        DESCRIPTION "The Apache Celix dependency manager (static) library"
        GROUP all
    )
    
    add_library(dependency_manager2 STATIC 
    	private/src/dm_component_impl)
   	include_directories("public/include")
   	include_directories("private/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    target_link_libraries(dependency_manager2 celix_framework)
    
    install(
    	FILES
    		public/include/dependency_activator_base.h 
    		public/include/service.h public/include/dependency_manager.h 
    		public/include/service_component_private.h 
    		public/include/service_component.h 
    		public/include/service_dependency.h 
		DESTINATION 
			include/celix/dependency_manager 
		COMPONENT 
			dependency_manager2
	)
    install(TARGETS dependency_manager2 DESTINATION lib COMPONENT dependency_manager)
endif (DEPENDENCY_MANAGER2)
