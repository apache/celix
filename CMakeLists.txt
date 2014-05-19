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
project (Celix C CXX)

cmake_policy(SET CMP0012 NEW)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_NAME_DIR "@rpath")

SET(CMAKE_BUILD_TYPE "Debug")
IF(UNIX)
	SET(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 ${CMAKE_C_FLAGS}")
ENDIF()
IF(WIN32)
	SET(CMAKE_C_FLAGS "-D_CRT_SECURE_NO_WARNINGS ${CMAKE_C_FLAGS}")
ENDIF()

# Set version for the framework package/release
SET(CELIX_MAJOR "1")
SET(CELIX_MINOR "0")
SET(CELIX_MICRO "0")
SET(CELIX_QUALIFIER "incubating")

enable_testing()

include(cmake/Includes.cmake)
include(cmake/installation.cmake)
include(UseDoxygen)  

# Default bundle version
set(DEFAULT_VERSION 0.0.1.incubating)

## New sub project must use a buildoption to be able to enable/disable the project using the CMake Editor
## Sub projects depending on another sub project automatically enable these dependencies
## For this to work, the order off "add_subdirectory" has to be correct (ie backwards)
add_subdirectory(examples)
add_subdirectory(device_access)
add_subdirectory(deployment_admin)
add_subdirectory(remote_services)
add_subdirectory(remote_shell)
add_subdirectory(shell_tui)
add_subdirectory(shell)
add_subdirectory(log_writer)
add_subdirectory(log_service)

add_subdirectory(event_admin)
add_subdirectory(dependency_manager)

add_subdirectory(launcher)
add_subdirectory(framework)
add_subdirectory(utils)

add_subdirectory(libraries)

deploy_targets()
