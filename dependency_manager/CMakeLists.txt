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

celix_subproject(DEPENDENCY_MANAGER "Option to build the dependency manager static library" ON DEPS framework)
if (DEPENDENCY_MANAGER)

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

    add_bundle(dm_shell
        SYMBOLIC_NAME "apache_celix_dm_shell"
        VERSION "1.0.0"
        NAME "Apache Celix DM Shell Commands"
        SOURCES
            private/src/dm_shell_activator
            private/src/dm_shell_list_command
    )
    target_link_libraries(dm_shell celix_framework celix_utils)


    add_library(dependency_manager_static STATIC 
    	private/src/dm_activator_base 
    	private/src/dm_component_impl 
    	private/src/dm_service_dependency
    	private/src/dm_event
    	private/src/dm_dependency_manager_impl
    )
    #set_target_properties(dependency_manager_static PROPERTIES SOVERSION 1)

    #add_library(dependency_manager_so SHARED 
    # 	private/src/dm_activator_base 
    #	private/src/dm_component_impl 
    #	private/src/dm_service_dependency
    # 	private/src/dm_event
    #    private/src/dm_dependency_manager_impl
    #)
    #set_target_properties(dependency_manager_so PROPERTIES SOVERSION 1)

    #if (APPLE) 
    #    target_link_libraries(dependency_manager_so celix_framework "-undefined dynamic_lookup")
    #else()
    #    target_link_libraries(dependency_manager_so celix_framework)
    #endif()

   	include_directories("public/include")
   	include_directories("private/include")
    include_directories("../shell/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    target_link_libraries(dependency_manager_static celix_framework)
    
    install(
    	FILES
    	    public/include/dm_activator.h
            public/include/dm_component.h
            public/include/dm_dependency_manager.h
            public/include/dm_service_dependency.h
            public/include/dm_info.h
		DESTINATION 
	    	include/celix/dependency_manager
		COMPONENT
			dependency_manager
	)
    install_bundle(dm_shell)
    install(TARGETS dependency_manager_static DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dependency_manager)
    #install(TARGETS dependency_manager_so DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dependency_manager)
endif (DEPENDENCY_MANAGER)
