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

add_library(c_rsa_spi INTERFACE)

target_include_directories(c_rsa_spi INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    )
target_link_libraries(c_rsa_spi INTERFACE Celix::framework)

install(TARGETS c_rsa_spi EXPORT celix COMPONENT rsa
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa COMPONENT rsa)

#Setup target aliases to match external usage
add_library(Celix::c_rsa_spi ALIAS c_rsa_spi)


#To be compatible with existing code, keep export target name 'Celix::deprecated_rsa_spi'
add_library(deprecated_rsa_spi INTERFACE)
target_link_libraries(deprecated_rsa_spi INTERFACE Celix::framework)
install(TARGETS deprecated_rsa_spi EXPORT celix COMPONENT rsa
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/rsa)