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
find_package(LIBZIP REQUIRED)

add_library(celix_cxx_Framework SHARED
        src/Framework.cc
        src/BundleContext.cc
        src/Bundle.cc
)
target_include_directories(celix_cxx_Framework PRIVATE src)
target_include_directories(celix_cxx_Framework PUBLIC include)
target_link_libraries(celix_cxx_Framework PRIVATE glog::glog libzip::libzip)
#NOTE because of libzil libbz2 and libz is also needed. maybe more to other form of resources (i.e. tar)
target_link_libraries(celix_cxx_Framework PUBLIC celix::cxx::Registry bz2 z UUID::lib)

add_library(celix::cxx::Framework ALIAS celix_cxx_Framework)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()