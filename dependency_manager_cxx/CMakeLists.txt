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

    if(UNIX AND NOT WIN32)
        find_program(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin )
        if(CMAKE_UNAME)
            exec_program(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
            set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} CACHE INTERNAL "processor type (i386 and x86_64)")
            if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
                set(DM_COMP_OPT -fPIC)
            endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        endif(CMAKE_UNAME)
    endif(UNIX AND NOT WIN32)

    add_library(dependency_manager_cxx_static STATIC
            ${CMAKE_SOURCE_DIR}/dependency_manager/src/dm_component_impl
            ${CMAKE_SOURCE_DIR}/dependency_manager/src/dm_service_dependency
            ${CMAKE_SOURCE_DIR}/dependency_manager/src/dm_event
            ${CMAKE_SOURCE_DIR}/dependency_manager/src/dm_dependency_manager_impl
            src/dm_activator.cc
    )
    target_include_directories(dependency_manager_cxx_static PUBLIC include ${CMAKE_SOURCE_DIR}/dependency_manager/api)
    target_include_directories(dependency_manager_cxx_static PRIVATE src)
    target_compile_options(dependency_manager_cxx_static PRIVATE ${DM_COMP_OPT})

    if (APPLE)
        target_link_libraries(dependency_manager_cxx_static Celix::framework "-undefined dynamic_lookup")
    else()
        target_link_libraries(dependency_manager_cxx_static Celix::framework)
    endif()

    install(
        DIRECTORY
            include/celix
        DESTINATION
            include/celix/dependency_manager_cxx
        COMPONENT
            dependency_manager_cxx
    )
    install(TARGETS dependency_manager_cxx_static DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT dependency_manager_cxx)

    unset(DM_COMP_OPT)


    #Setup target aliases to match external usage
    add_library(Celix::dependency_manager_cxx_static ALIAS dependency_manager_cxx_static)
endif (DEPENDENCY_MANAGER_CXX)
