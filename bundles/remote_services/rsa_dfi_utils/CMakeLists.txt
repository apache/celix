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

add_library(rsa_dfi_utils STATIC
    src/dfi_utils.c
    )
set_target_properties(rsa_dfi_utils PROPERTIES OUTPUT_NAME "celix_rsa_dfi_utils")
target_include_directories(rsa_dfi_utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        )
target_include_directories(rsa_dfi_utils PRIVATE src)
target_link_libraries(rsa_dfi_utils PUBLIC Celix::framework  Celix::log_helper Celix::dfi)

#Setup target aliases to match external usage
add_library(Celix::rsa_dfi_utils ALIAS rsa_dfi_utils)

install(TARGETS rsa_dfi_utils EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT rsa
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa_dfi_utils)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa_dfi_utils COMPONENT rsa)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif(ENABLE_TESTING)