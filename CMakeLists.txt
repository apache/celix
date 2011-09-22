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
#SET(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 ${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs")
SET(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 ${CMAKE_C_FLAGS}")
#SET(LDFLAGS "-fprofile-arcs -ftest-coverage")
SET(CMAKE_INSTALL_COMPONENT "framework")

GET_FILENAME_COMPONENT(__cmake_path ${CMAKE_COMMAND} PATH)
FIND_PROGRAM(CPACK_COMMAND cpack ${__cmake_path})
MESSAGE(STATUS "Found CPack at: ${CPACK_COMMAND}")
IF(NOT CPACK_COMMAND)
	MESSAGE(FATAL_ERROR "Need CPack!")
ENDIF(NOT CPACK_COMMAND)

SET(INSTALL_FW "${PROJECT_BINARY_DIR};Celix Framework;framework;/")
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/CPackConfigFW.in CPackConfigFW.cmake @ONLY)
ADD_CUSTOM_TARGET(install-fw
  ${CPACK_COMMAND} --config "CPackConfigFW.cmake"
  )

include(cmake/Includes.cmake)  

add_subdirectory(utils)
add_subdirectory(framework)
add_subdirectory(launcher)

add_subdirectory(dependency_manager)

add_subdirectory(shell)
add_subdirectory(shell_tui)

add_subdirectory(log_service)
add_subdirectory(log_writer)

add_subdirectory(examples)

include(target.cmake)
#enable_testing()
