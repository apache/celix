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

add_library(error_injector INTERFACE error_injector.h)
target_include_directories(error_injector INTERFACE ${CMAKE_CURRENT_LIST_DIR})
# get caller address of the target function, into which errors are injected
target_link_libraries(error_injector INTERFACE dl)
target_link_options(error_injector INTERFACE -rdynamic)
if(NOT APPLE)
    target_compile_options(error_injector INTERFACE -Wno-frame-address)
else()
    target_compile_options(error_injector INTERFACE -Wno-error=frame-address)
endif()
add_subdirectory(malloc)
add_subdirectory(celix_properties)
