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
        googletest_project
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.8.1
        UPDATE_DISCONNECTED TRUE
        PREFIX ${CMAKE_BINARY_DIR}/gtest
        INSTALL_COMMAND ""
)

ExternalProject_Get_Property(googletest_project source_dir binary_dir)

file(MAKE_DIRECTORY ${source_dir}/googletest/include)
add_library(gtest IMPORTED STATIC GLOBAL)
add_dependencies(gtest googletest_project)
set_target_properties(gtest PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/gtest/libgtest.a"
        INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/googletest/include"
)

file(MAKE_DIRECTORY ${source_dir}/googlemock/include)
add_library(gmock IMPORTED STATIC GLOBAL)
add_dependencies(gmock googletest_project)
set_target_properties(gmock PROPERTIES
        IMPORTED_LOCATION "${binary_dir}/googlemock/libgmock.a"
        INTERFACE_INCLUDE_DIRECTORIES "${source_dir}/googlemock/include"
)
