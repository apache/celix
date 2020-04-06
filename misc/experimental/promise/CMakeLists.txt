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


celix_subproject(PROMISE "Option to build the Promise library" ON)
if (PROMISE)

    find_package(TBB QUIET)
    if (NOT TBB_FOUND)
        #NOTE: TBB does not yet deliver a TBBConfig.cmake on Ubuntu 18, using a FindTBB.cmake file
        set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    endif ()
    find_package(TBB REQUIRED)

    add_library(celix_Promise INTERFACE)
    target_include_directories(celix_Promise INTERFACE api)
    target_link_libraries(celix_Promise INTERFACE TBB::tbb)
    target_compile_options(celix_Promise INTERFACE -frtti) #Note needed for TBB
    add_library(Celix::Promise ALIAS celix_Promise)

    add_executable(PromiseExamples src/PromiseExamples.cc)
    target_link_libraries(PromiseExamples PRIVATE Celix::Promise)

    if (ENABLE_TESTING)
        add_subdirectory(gtest)
    endif()

endif ()