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

cmake_minimum_required (VERSION 3.2)
cmake_policy(SET CMP0012 NEW)
cmake_policy(SET CMP0042 NEW)

project (Celix C CXX)

include(GNUInstallDirs)                                                                                                                                                                             

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#Setup vars conform the FindCelix setup, so that examples and supporting cmake command can assume these variables are set
set(CELIX_FOUND true)
set(CELIX_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/framework/public/include ${CMAKE_SOURCE_DIR}/utils/public/include)
set(CELIX_LIBRARIES celix_framework celix_utils celix_dfi)
set(CELIX_LAUNCHER celix)
set(CELIX_FRAMEWORK_LIBRARY celix_framework)
set(CELIX_UTILS_LIBRARY celix_utils)
set(CELIX_DFI_LIBRARY celix_dfi)
set(CELIX_DM_LIB dependency_manager_so)
set(CELIX_DM_STATIC_LIB dependency_manager_static)
set(CELIX_DM_STATIC_CXX_LIB dependency_manager_cxx_static)
set(CELIX_PROJECT true) #Note this var is not set by FindCelix and can be used to test if this is the celix project or a project using celix

set(CMAKE_BUILD_TYPE "Debug")

# see https://public.kitware.com/Bug/view.php?id=15696
IF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} EQUAL 3.3 AND ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    message( FATAL_ERROR "Building Celix using CMake 3.3 and makefiles is not supported due to a bug in the Makefile Generator (see Bug 15696). Please change the used CMake version - both, CMake 3.2 and CMake 3.4 are working fine. Or use a different generator (e.g. Ninja)." )
ENDIF()

IF (ANDROID)
    set(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 -Wall ${CMAKE_C_FLAGS}")
ELSE ()
    set(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 -Wall -Werror -fPIC ${CMAKE_C_FLAGS}") #TODO add -Wextra
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -fpermissive -Wextra -Weffc++ -fno-rtti -fno-exceptions ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${CMAKE_CXX_FLAGS}")
ENDIF()

IF(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
ELSE ()
    set(CMAKE_C_FLAGS "-pthread ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-pthread ${CMAKE_EXE_LINKER_FLAGS}")
ENDIF()

# Set version for the framework package/release
set(CELIX_MAJOR "2")
set(CELIX_MINOR "1")
set(CELIX_MICRO "0")
set(CELIX_QUALIFIER "")

option(ENABLE_TESTING "Enables unit/bundle testing" FALSE)

if (ENABLE_TESTING)
	enable_testing()
endif()


include(CMakeCelix)
include(UseDoxygen)  

# Default bundle version
set(DEFAULT_VERSION 1.0.0)

#utils, dfi and etcdlib are standalone
#(e.g. no dependency on celix framework
add_subdirectory(utils)
add_subdirectory(dfi)
add_subdirectory(etcdlib)

add_subdirectory(framework)

include_directories(framework/public/include)
add_subdirectory(launcher)
#add_subdirectory(config_admin) config admin is unstable
add_subdirectory(device_access)
add_subdirectory(deployment_admin)
add_subdirectory(remote_services)
add_subdirectory(remote_shell)
add_subdirectory(shell_bonjour)
add_subdirectory(shell_tui)
add_subdirectory(shell)
add_subdirectory(log_writer)
add_subdirectory(log_service)
add_subdirectory(pubsub)

#add_subdirectory(event_admin)# event_admin is unstable
add_subdirectory(dependency_manager)
add_subdirectory(dependency_manager_cxx)

#Example as last, because some example will check if underlining options are enabled
add_subdirectory(examples)
