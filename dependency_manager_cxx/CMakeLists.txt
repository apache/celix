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

celix_subproject(DEPENDENCY_MANAGER_CXX "Option to build the C++ dependency manager static library" ON DEPS framework DEPENDENCY_MANAGER)
if (DEPENDENCY_MANAGER_CXX)

    #set -Wall, -Werror locally, currenctly cpputest contains warnings
    set(CMAKE_CXX_FLAGS "-Wall -Werror -fno-rtti -fno-exceptions ${CMAKE_CXX_FLAGS}")

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

    include_directories(include)
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/dependency_manager/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/dependency_manager/private/include")

    set(DM_DIR "${PROJECT_SOURCE_DIR}/dependency_manager")

    add_library(dependency_manager_cxx_static STATIC
    	${DM_DIR}/private/src/dm_component_impl.c
        ${DM_DIR}/private/src/dm_service_dependency.c
        ${DM_DIR}/private/src/dm_event.c
        ${DM_DIR}/private/src/dm_dependency_manager_impl.c

        src/DependencyManager.cc
        src/Component.cc
        src/ServiceDependency.cc
        src/dm_activator_base_cxx.cc
    )
    #set_target_properties(dependency_manager_cxx_static PROPERTIES SOVERSION 1)
    target_link_libraries(dependency_manager_cxx_static celix_framework)

    install(
        DIRECTORY
            include/celix
        DESTINATION
            include/celix/dependency_manager_cxx
        COMPONENT
            dependency_manager_cxx
    )

    install(TARGETS dependency_manager_cxx_static DESTINATION lib COMPONENT dependency_manager_cxx)
endif (DEPENDENCY_MANAGER_CXX)
