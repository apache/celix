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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_NAME_DIR "@rpath")

SET(CMAKE_BUILD_TYPE "Debug")

# see https://public.kitware.com/Bug/view.php?id=15696
IF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} EQUAL 3.3 AND ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    message( FATAL_ERROR "Building Celix using CMake 3.3 and makefiles is not supported due to a bug in the Makefile Generator (see Bug 15696). Please change the used CMake version - both, CMake 3.2 and CMake 3.4 are working fine. Or use a different generator (e.g. Ninja)." )
ENDIF()

IF(UNIX)
	SET(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 -Wall ${CMAKE_C_FLAGS}")
ENDIF()
IF(UNIX AND NOT APPLE) 
	SET(CMAKE_C_FLAGS "-pthread ${CMAKE_C_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-pthread ${CMAKE_EXE_LINKER_FLAGS}")
ENDIF()
IF(WIN32)
	SET(CMAKE_C_FLAGS "-D_CRT_SECURE_NO_WARNINGS ${CMAKE_C_FLAGS}")
ENDIF()

# Set version for the framework package/release
SET(CELIX_MAJOR "1")
SET(CELIX_MINOR "0")
SET(CELIX_MICRO "0")
SET(CELIX_QUALIFIER "")

option(ENABLE_TESTING "Enables unit/bundle testing" FALSE)

if (ENABLE_TESTING)
	enable_testing()
endif()


include(CMakeCelix)
include(cmake_celix/installation)
include(UseDoxygen)  

# Default bundle version
set(DEFAULT_VERSION 1.0.0)

option("WITH_APR" "Build Celix with APR support" "OFF")
if(${WITH_APR})
	find_package(APR REQUIRED)

	include_directories(${APR_INCLUDE_DIR})
	include_directories(${APRUTIL_INCLUDE_DIR})
endif()
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION include/celix COMPONENT framework)

## New sub project must use a buildoption to be able to enable/disable the project using the CMake Editor
## Sub projects depending on another sub project automatically enable these dependencies
## For this to work, the order off "add_subdirectory" has to be correct (ie backwards)
add_subdirectory(examples)
add_subdirectory(device_access)
add_subdirectory(deployment_admin)
add_subdirectory(remote_services)
add_subdirectory(remote_shell)
add_subdirectory(shell_bonjour)
add_subdirectory(shell_tui)
add_subdirectory(shell)
add_subdirectory(log_writer)
add_subdirectory(log_service)

add_subdirectory(event_admin)
add_subdirectory(dependency_manager)

add_subdirectory(launcher)
add_subdirectory(framework)
add_subdirectory(utils)

deploy_targets()
