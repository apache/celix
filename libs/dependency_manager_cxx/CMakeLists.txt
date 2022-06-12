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

add_library(dependency_manager_cxx_static STATIC
        src/dm_activator.cc
)
set_target_properties(dependency_manager_cxx_static PROPERTIES OUTPUT_NAME "celix_dependency_manager_cxx_static")
target_compile_options(dependency_manager_cxx_static PRIVATE -fPIC)
target_compile_options(dependency_manager_cxx_static PRIVATE -Wno-deprecated-declarations)
if (APPLE)
    target_link_libraries(dependency_manager_cxx_static Celix::framework "-undefined dynamic_lookup")
else()
    target_link_libraries(dependency_manager_cxx_static Celix::framework)
endif()

install(TARGETS dependency_manager_cxx_static EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)

#Setup target aliases to match external usage
add_library(Celix::dependency_manager_cxx_static ALIAS dependency_manager_cxx_static)
