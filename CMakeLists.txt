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

cmake_minimum_required (VERSION 2.6)
project (Celix C)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS "-std=c99")
SET(CMAKE_C_FLAGS "-D_GNU_SOURCE")
SET(CMAKE_INSTALL_COMPONENT "framework")

ADD_CUSTOM_TARGET(install-fw
  ${CMAKE_COMMAND}
  -D "CMAKE_INSTALL_COMPONENT=framework"
  -D "CMAKE_INSTALL_PREFIX=test"
  -P "cmake_install.cmake"
  )
  
include(FindAPR)
IF(APR_FOUND)
	MESSAGE(STATUS "Looking for APR - found")
ELSE(APR_FOUND)
	MESSAGE(FATAL_ERROR "Looking for APR - not found")
ENDIF(APR_FOUND)
include_directories(${APR_INCLUDE_DIR})

include_directories("framework/private/include")
include_directories("framework/public/include")

add_subdirectory(utils)
add_subdirectory(framework)

include(Packaging.cmake)
add_subdirectory(dependency_manager)
add_subdirectory(launcher)
add_subdirectory(test)

add_subdirectory(shell)
add_subdirectory(shell_tui)

#add_subdirectory(receiver)
#add_subdirectory(receiver-2.0)
#add_subdirectory(sender)

add_subdirectory(deployer)
add_subdirectory(hello_world)
add_subdirectory(mongoose)
#add_subdirectory(echo_service/client)
#add_subdirectory(echo_service/server)
add_subdirectory(whiteboard)
#add_subdirectory(echo_service)

add_subdirectory(celix_test)

include(target.cmake)
enable_testing()