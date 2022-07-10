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

add_library(shm_pool SHARED
        src/shm_pool.c
        src/shm_cache.c
        src/tlsf.c
        )

set_target_properties(shm_pool PROPERTIES OUTPUT_NAME "celix_shm_pool")
target_include_directories(shm_pool PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        )

target_include_directories(shm_pool PRIVATE src)

target_link_libraries(shm_pool
        PUBLIC
        Celix::framework
        )

set_target_properties(shm_pool PROPERTIES "SOVERSION" 1)

install(TARGETS shm_pool EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT rsa
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/shm_pool)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/shm_pool COMPONENT rsa)

#Alias setup to match external usage
add_library(Celix::shm_pool ALIAS shm_pool)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif(ENABLE_TESTING)