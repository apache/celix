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
cmake_minimum_required(VERSION 2.6)

include(FindZLIB)
IF(ZLIB_FOUND)
	MESSAGE(STATUS "Looking for ZLIB - found")
ELSE(ZLIB_FOUND)
	MESSAGE(FATAL_ERROR "Looking for ZLIB - not found")
ENDIF(ZLIB_FOUND)

include(FindCURL)
IF(CURL_FOUND)
	MESSAGE(STATUS "Looking for CURL - found")
ELSE(CURL_FOUND)
	MESSAGE(FATAL_ERROR "Looking for CURL - not found")
ENDIF(CURL_FOUND)

include(UseDoxygen)

add_definitions(-DUSE_FILE32API)
aux_source_directory("private/src" SRC)
include_directories(${CURL_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIR})
include_directories("private/include")
include_directories("public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
add_library(framework SHARED ${SRC})
target_link_libraries(framework utils ${ZLIB_LIBRARY} ${APR_LIBRARY} ${APRUTIL_LIBRARY} ${CURL_LIBRARY})

install(TARGETS framework DESTINATION lib COMPONENT framework)
FILE(GLOB files "private/include/*.h")
INSTALL(FILES ${files} DESTINATION include COMPONENT framework)
