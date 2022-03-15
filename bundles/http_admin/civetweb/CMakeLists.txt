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


#Create shared library
add_library(civetweb_shared SHARED
        src/civetweb.c
        )
target_include_directories(civetweb_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        )
add_library(Celix::civetweb_shared ALIAS civetweb_shared)

#Create static library
add_library(civetweb_static STATIC
        src/civetweb.c
)

target_include_directories(civetweb_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_compile_options(civetweb_shared PRIVATE -w -DUSE_WEBSOCKET)
target_compile_options(civetweb_static PRIVATE -w -DUSE_WEBSOCKET)

install(TARGETS civetweb_shared civetweb_static EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT http_admin
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/http_admin)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/http_admin)

