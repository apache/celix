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

add_library(discovery_spi STATIC
        src/do_not_remove.cc)

set_target_properties(discovery_spi PROPERTIES OUTPUT_NAME "celix_discovery_spi")
target_include_directories(discovery_spi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include/celix/discovery_spi>)

install(TARGETS discovery_spi EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT async_rsa_discovery_configured)
install(DIRECTORY include/ DESTINATION include/celix/discovery_spi COMPONENT async_rsa_discovery_configured)

add_library(Celix::discovery_spi ALIAS discovery_spi)
