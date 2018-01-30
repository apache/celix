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

add_celix_bundle(dm_example_cxx_phase2a
    SYMBOLIC_NAME phase2a_cxx
    VERSION 0.0.1
    SOURCES
        src/Phase2aActivator.cc
        src/Phase2aCmp.cc
)
target_include_directories(dm_example_cxx_phase2a PRIVATE src)
target_link_libraries(dm_example_cxx_phase2a PRIVATE Celix::log_service_api dm_example_cxx_api)

add_celix_bundle(dm_example_cxx_phase2b
        SYMBOLIC_NAME phase2b_cxx
        VERSION 0.0.1
        SOURCES
        src/Phase2bActivator.cc
        src/Phase2bCmp.cc
        )
target_include_directories(dm_example_cxx_phase2b PRIVATE src)
target_link_libraries(dm_example_cxx_phase2b PRIVATE Celix::log_service_api dm_example_cxx_api)

IF(APPLE)
    target_link_libraries(dm_example_cxx_phase2a PRIVATE -Wl,-all_load Celix::dependency_manager_cxx_static)
    target_link_libraries(dm_example_cxx_phase2b PRIVATE -Wl,-all_load Celix::dependency_manager_cxx_static)
else()
    if(ENABLE_ADDRESS_SANITIZER)
        #With asan there can be undefined symbols
        target_link_libraries(dm_example_cxx_phase2a PRIVATE -Wl,--whole-archive Celix::dependency_manager_cxx_static -Wl,--no-whole-archive)
        target_link_libraries(dm_example_cxx_phase2b PRIVATE -Wl,--whole-archive Celix::dependency_manager_cxx_static -Wl,--no-whole-archive)
    else()
        target_link_libraries(dm_example_cxx_phase2a PRIVATE -Wl,--whole-archive Celix::dependency_manager_cxx_static -Wl,--no-whole-archive)
        target_link_libraries(dm_example_cxx_phase2b PRIVATE -Wl,--whole-archive Celix::dependency_manager_cxx_static -Wl,--no-whole-archive)

    endif()
endif()
