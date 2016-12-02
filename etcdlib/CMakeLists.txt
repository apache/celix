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

find_package(CURL REQUIRED)
find_package(Jansson REQUIRED)

include_directories(
        ${CURL_INCLUDE_DIR}
        ${JANSSON_INCLUDE_DIRS}
        private/include
        public/include
)

add_library(etcdlib SHARED
    private/src/etcd.c
)

set_target_properties(etcdlib PROPERTIES "SOVERSION" 1)
target_link_libraries(etcdlib ${CURL_LIBRARIES} ${JANSSON_LIBRARY})

add_library(etcdlib_static  STATIC
    private/src/etcd.c
)

set_target_properties(etcdlib_static PROPERTIES "SOVERSION" 1)
target_link_libraries(etcdlib_static ${CURL_LIBRARIES} ${JANSSON_LIBRARY})


install(TARGETS etcdlib  etcdlib_static  DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)
FILE(GLOB files "public/include/*.h")
INSTALL(FILES ${files} DESTINATION include/celix/etcdlib COMPONENT framework)
