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

add_library(async_rsa_common INTERFACE)
target_include_directories(async_rsa_common INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include/celix/async_rsa>
        )
#set_target_properties(async_rsa_common PROPERTIES OUTPUT_NAME "celix_async_rsa_common")
#target_include_directories(async_rsa_common PRIVATE src)
#target_link_libraries(async_rsa_common PUBLIC Celix::framework)

#Setup target aliases to match external usage
add_library(Celix::async_rsa_common ALIAS async_rsa_common)
install(TARGETS async_rsa_common EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT async_rsa)
install(DIRECTORY include/ DESTINATION include/celix/async_rsa COMPONENT async_rsa)
