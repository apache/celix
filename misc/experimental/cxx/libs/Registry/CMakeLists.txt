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

find_package(UUID REQUIRED)

add_library(celix_cxx_Registry STATIC
        src/EmptyResourceBundle.cc
        src/Utils.cc
        src/Filter.cc
        src/FilterBuilder.cc
        src/Properties.cc
        src/ServiceRegistry.cc
        src/ComponentManager.cc
        src/ServiceTrackerImpl.cc
        src/ServiceRegistationImpl.cc
        )

target_include_directories(celix_cxx_Registry PRIVATE src)
target_include_directories(celix_cxx_Registry PUBLIC include)
target_link_libraries(celix_cxx_Registry PRIVATE spdlog::spdlog UUID::lib)
target_compile_options(celix_cxx_Registry PRIVATE -fPIC)

add_library(celix::cxx::Registry ALIAS celix_cxx_Registry)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()