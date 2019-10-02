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

include(GNUInstallDirs)
include(CMakeParseArguments)

set(CELIX_CMAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CELIX_CMAKE_DIRECTORY}/../Modules)


#Celix CMake function
include(${CELIX_CMAKE_DIRECTORY}/BundlePackaging.cmake)
include(${CELIX_CMAKE_DIRECTORY}/ContainerPackaging.cmake)
include(${CELIX_CMAKE_DIRECTORY}/DockerPackaging.cmake)
include(${CELIX_CMAKE_DIRECTORY}/Runtimes.cmake)
include(${CELIX_CMAKE_DIRECTORY}/Generic.cmake)

#find required packages
find_package(CURL REQUIRED) #framework, etcdlib
find_package(ZLIB REQUIRED) #framework
find_package(UUID REQUIRED) #framework
find_package(JANSSON REQUIRED) #etcdlib, dfi
find_package(FFI REQUIRED) #dfi

if (NOT TARGET ZLIB::ZLIB)
    message("Note ZLIB::ZLIB target not created by find_package(ZLIB). Creating one")
    add_library(ZLIB::ZLIB SHARED IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
            IMPORTED_LOCATION "${ZLIB_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIRS}"
            )
endif ()

if (NOT TARGET CURL::libcurl)
    message("Note CURL::libcurl target not created by find_package(CURL). Creating one")
    add_library(CURL::libcurl SHARED IMPORTED)
    set_target_properties(CURL::libcurl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIRS}"
            )
endif ()