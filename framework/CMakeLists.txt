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

celix_subproject(FRAMEWORK "Option to build the framework shared library" "ON" DEPS UTILS)
if (FRAMEWORK) 
    cmake_minimum_required(VERSION 2.6)
    
    find_package(CURL REQUIRED)
    find_package(ZLIB REQUIRED)
    
    include(CPackComponent)
    
    cpack_add_component(framework
    	DISPLAY_NAME Framework
        DESCRIPTION "The Apache Celix framework library"
        REQUIRED
    )
    
    add_definitions(-DUSE_FILE32API)
    aux_source_directory("private/src" SRC)
    #include_directories(${CURL_INCLUDE_DIR})
    include_directories(${ZLIB_INCLUDE_DIR})
    include_directories("private/include")
    include_directories("public/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    add_library(celix_framework SHARED ${SRC})
    target_link_libraries(celix_framework celix_utils ${ZLIB_LIBRARY} ${APR_LIBRARY})
    
    install(TARGETS celix_framework DESTINATION lib COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    
endif (FRAMEWORK)
