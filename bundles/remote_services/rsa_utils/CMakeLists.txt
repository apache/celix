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

set(RSA_UTILS_SRC
    src/celix_rsa_utils.c
)

add_library(rsa_utils STATIC ${RSA_UTILS_SRC})
set_target_properties(rsa_utils PROPERTIES OUTPUT_NAME "celix_rsa_utils")
target_include_directories(rsa_utils PRIVATE src)
target_include_directories(rsa_utils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_link_libraries(rsa_utils PUBLIC Celix::framework)

install(TARGETS rsa_utils EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT rsa
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa_utils)
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa_utils/
        COMPONENT rsa)

#Setup target aliases to match external usage
add_library(Celix::rsa_utils ALIAS rsa_utils)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()
