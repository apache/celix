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

include(ExternalProject)
ExternalProject_Add(
        spdlog_project
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.5.0
        UPDATE_DISCONNECTED TRUE
        PREFIX ${CMAKE_BINARY_DIR}/spdlog
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(spdlog_project source_dir binary_dir)

file(MAKE_DIRECTORY ${source_dir}/spdlog/include)
add_library(spdlog::spdlog IMPORTED STATIC GLOBAL)
add_dependencies(pdlog::spdlog spdlog_project)
set_target_properties(spdlog::spdlog PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/libspdlog.a"
        INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/include"
)

