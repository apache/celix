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

# - Config file for the Apache Celix framework
# It defines the following variables
#  CELIX_CMAKE_MODULES_DIR - The directory containing the Celix CMake modules
#  CELIX_INCLUDE_DIRS      - include directories for the Celix framework
#  CELIX_LIBRARIES         - libraries to link against
#  CELIX_LAUNCHER          - The Celix launcher

# relative install dir from lib/CMake/Celix.
get_filename_component(REL_INSTALL_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(REL_INSTALL_DIR "${REL_INSTALL_DIR}" PATH)
get_filename_component(REL_INSTALL_DIR "${REL_INSTALL_DIR}" PATH)
get_filename_component(REL_INSTALL_DIR "${REL_INSTALL_DIR}" PATH)

include("${REL_INSTALL_DIR}/share/celix/cmake/cmake_celix/UseCelix.cmake") #adds celix commands (e.g. add_celix_bundle)
include("${REL_INSTALL_DIR}/share/celix/cmake/Targets.cmake") #imports lib and exe targets (e.g. Celix::framework)

include("${REL_INSTALL_DIR}/share/celix/cmake/CelixTargets.cmake")

# The rest is added to ensure backwards compatiblity with project using the cmake lib/include var instead of targets.
set(CELIX_CMAKE_MODULES_DIR ${REL_INSTALL_DIR}/share/celix/cmake/Modules)

set(CELIX_FRAMEWORK_INCLUDE_DIR "${REL_INSTALL_DIR}/include/celix")
set(CELIX_UTILS_INCLUDE_DIR "${REL_INSTALL_DIR}/include/utils")
set(CELIX_DFI_INCLUDE_DIR "${REL_INSTALL_DIR}/include/dfi")

set(CELIX_LIBRARIES Celix::framework Celix::utils Celix::dfi)
set(CELIX_INCLUDE_DIRS
  $<TARGET_PROPERTY:Celix::framework,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:Celix::utils,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:Celix::dfi,INTERFACE_INCLUDE_DIRECTORIES>
)

set(CELIX_FRAMEWORK_LIBRARY Celix::framework)
set(CELIX_UTILS_LIBRARY Celix::utils)
set(CELIX_DFI_LIBRARY Celix::dfi)
set(CELIX_LAUNCHER Celix::launcher)

if (TARGET Celix::etcdlib)
  set(CELIX_ETCD_INCLUDE_DIRS $<TARGET_PROPERTY:Celix::etcdlib,INTERFACE_INCLUDE_DIRECTORIES>)
  set(CELIX_ETCD_LIB Celix::etcdlib)
endif ()

if (TARGET Celix::dependency_manager_so)
  set(CELIX_DM_LIB Celix::dependency_manager_so)
  set(CELIX_DM_INCLUDE_DIR $<TARGET_PROPERTY:Celix::dependency_manager_so,INTERFACE_INCLUDE_DIRECTORIES>)
  set(CELIX_DM_STATIC_LIB Celix::dependency_manager_static)
endif ()
if (TARGET Celix::dependency_manager_cxx)
  set(CELIX_DM_STATIC_CXX_LIB Celix::dependency_manager_cxx)
  set(CELIX_DM_CXX_STATIC_LIB $<TARGET_PROPERTY:Celix::dependency_manager_cxx,INTERFACE_INCLUDE_DIRECTORIES>)
endif ()

set(CELIX_BUNDLES_DIR ${REL_INSTALL_DIR}/share/celix/bundles)
set(CELIX_SHELL_BUNDLE ${CELIX_BUNDLES_DIR}/shell.zip)
set(CELIX_SHELL_TUI_BUNDLE ${CELIX_BUNDLES_DIR}/shell_tui.zip)

include(CMakeFindDependencyMacro)

find_dependency(ZLIB REQUIRED) #Needed by framework
find_dependency(UUID REQUIRED) #Needed by framework
find_dependency(CURL REQUIRED) #Needed by framework (used for curl initialization)
find_dependency(LIBZIP) #Needed by utils
find_dependency(Jansson REQUIRED) #Needed by dfi, etcdlib, remote services, pubsub

if (TARGET Celix::dfi)
  find_dependency(FFI REQUIRED)
endif()
if (TARGET Celix::RsaConfiguredDiscovery)
  find_dependency(RapidJSON REQUIRED)
endif ()
if (TARGET Celix::rsa_discovery_common)
  find_dependency(LibXml2 REQUIRED)
endif ()
if (TARGET Celix::celix_pubsub_admin_zmq OR TARGET Celix::celix_pubsub_admin_zmq_v2)
  find_dependency(ZMQ REQUIRED)
  find_dependency(CZMQ REQUIRED)
endif ()
if (TARGET Celix::pubsub_admin_nanomsg)
  find_dependency(NanoMsg REQUIRED)
endif ()
