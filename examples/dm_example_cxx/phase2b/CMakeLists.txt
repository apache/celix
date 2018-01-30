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

include_directories(
        ../phase2/include
        include
        ../api
)

add_celix_bundle(phase2b_cxx
    SYMBOLIC_NAME phase2b_cxx
    VERSION 0.0.1
    SOURCES
        src/Phase2bActivator.cc
        src/Phase2bCmp.cc
)

target_compile_options(phase2b_cxx PUBLIC -Wall -Wextra -Weffc++ -Werror)

IF(APPLE)
    target_link_libraries(phase2b_cxx celix_framework -Wl,-all_load dependency_manager_cxx_static)
else()
    if(ENABLE_ADDRESS_SANITIZER)
        #With asan there can be undefined symbols
        target_link_libraries(phase2b_cxx -Wl,--whole-archive dependency_manager_cxx_static -Wl,--no-whole-archive celix_framework)
    else()
        target_link_libraries(phase2b_cxx -Wl,--no-undefined -Wl,--whole-archive dependency_manager_cxx_static -Wl,--no-whole-archive celix_framework)
    endif()
endif()
