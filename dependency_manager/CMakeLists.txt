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
             set(DM_COMP_OPT "-fPIC")
        endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
      endif(CMAKE_UNAME)
    endif(UNIX AND NOT WIN32)

    add_library(dependency_manager_static STATIC 
    	src/dm_component_impl
    	src/dm_service_dependency
    	src/dm_event
    	src/dm_dependency_manager_impl
        src/dm_activator
    )
    set_target_properties(dependency_manager_static PROPERTIES OUTPUT_NAME "celix_dependency_manager_static")
    target_link_libraries(dependency_manager_static Celix::framework)
    target_compile_options(dependency_manager_static PRIVATE ${DM_COMP_OPT})

    add_library(dependency_manager_so SHARED
    	src/dm_component_impl
    	src/dm_service_dependency
     	src/dm_event
        src/dm_dependency_manager_impl
        src/dm_activator
    )
    set_target_properties(dependency_manager_so PROPERTIES OUTPUT_NAME "celix_dependency_manager_so")
    set_target_properties(dependency_manager_so PROPERTIES SOVERSION 1)

    if (APPLE)
        target_link_libraries(dependency_manager_so Celix::framework "-undefined dynamic_lookup")
    else()
        target_link_libraries(dependency_manager_so Celix::framework)
    endif()

    target_include_directories(dependency_manager_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
        $<INSTALL_INTERFACE:include/celix/dependency_manager>
    )
    target_include_directories(dependency_manager_static PRIVATE src)
    target_include_directories(dependency_manager_so PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
        $<INSTALL_INTERFACE:include/celix/dependency_manager>
    )
    target_include_directories(dependency_manager_so PRIVATE src)

    add_celix_bundle(dm_shell
        SYMBOLIC_NAME "apache_celix_dm_shell"
        VERSION "1.0.0"
        NAME "Apache Celix DM Shell Commands"
        SOURCES
            src/dm_shell_activator
            src/dm_shell_list_command
    )
    target_include_directories(dm_shell PRIVATE api src)
    target_link_libraries(dm_shell PRIVATE Celix::shell_api)

    install_celix_bundle(dm_shell EXPORT celix)
    install(TARGETS dependency_manager_so dependency_manager_static EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dependency_manager)
    install(DIRECTORY api/ DESTINATION include/celix/dependency_manager COMPONENT dependency_manager)

    unset(DM_COMP_OPT)


    #Setup target aliases to match external usage
    add_library(Celix::dm_shell ALIAS dm_shell)
    add_library(Celix::dependency_manager_static ALIAS dependency_manager_static)
    add_library(Celix::dependency_manager_so ALIAS dependency_manager_so)
endif (DEPENDENCY_MANAGER)
