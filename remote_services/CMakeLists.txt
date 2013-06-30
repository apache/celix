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

celix_subproject(REMOTE_SERVICE_ADMIN "Option to enable building the Remote Service Admin Service bundles" OFF DEPS LAUNCHER shell_tui log_writer)
if (REMOTE_SERVICE_ADMIN)
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

    add_subdirectory(utils)
    
    add_subdirectory(topology_manager)
    add_subdirectory(remote_service_admin)
    add_subdirectory(discovery_slp)
    #add_subdirectory(discovery_bonjour)
    
    add_subdirectory(example_service)
    add_subdirectory(example_endpoint)
    add_subdirectory(example_proxy)
    add_subdirectory(calc_shell)
endif (REMOTE_SERVICE_ADMIN)
