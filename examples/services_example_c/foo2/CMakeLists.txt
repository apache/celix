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
        private/include
)

add_celix_bundle(foo2
    SYMBOLIC_NAME foo2
    VERSION 1.0.0
    SOURCES
        private/src/foo2_activator
        private/src/foo2.c
)

IF(APPLE)
    target_link_libraries(foo2 PRIVATE -Wl,-all_load dependency_manager_static)
else()
    if(ENABLE_ADDRESS_SANITIZER)
        #With asan there can be undefined symbols
        target_link_libraries(foo2 PRIVATE -Wl,--whole-archive dependency_manager_static -Wl,--no-whole-archive)
    else()
        target_link_libraries(foo2 PRIVATE -Wl,--no-undefined -Wl,--whole-archive dependency_manager_static -Wl,--no-whole-archive)
    endif()
endif()
