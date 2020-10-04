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

if (NOT COMMAND celix_subproject)
    #If COMMAND celix_subproject is not defined, this CMakeLists will
    #act as a top level project. Making the Celix::Promise useable
    #stand-alone

    cmake_minimum_required (VERSION 3.11)
    project(celix_Promise
            VERSION 1.0.0
            LANGUAGES CXX
    )

    include(GNUInstallDirs)

    set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

    set(PROMISE_STANDALONE ON)
else ()
    celix_subproject(PROMISE "Option to build the Promise library" ON)
endif ()

if (PROMISE OR PROMISE_STANDALONE)

    find_package(TBB QUIET)
    if (NOT TBB_FOUND)
        #NOTE: TBB does not yet deliver a TBBConfig.cmake on Ubuntu 18, using a FindTBB.cmake file
        set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    endif ()
    find_package(TBB REQUIRED)
    find_package(Threads)

    add_library(Promise INTERFACE)
    target_include_directories(Promise INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
        $<INSTALL_INTERFACE:include/celix/promise>
    )
    target_link_libraries(Promise INTERFACE TBB::tbb Threads::Threads)
    target_compile_options(Promise INTERFACE -frtti -std=c++17) #Note -frtti needed for TBB
    add_library(Celix::Promise ALIAS Promise)

    add_executable(PromiseExamples src/PromiseExamples.cc)
    target_link_libraries(PromiseExamples PRIVATE Celix::Promise)

    if (ENABLE_TESTING AND NOT PROMISE_STANDALONE)
        add_subdirectory(gtest)
    endif()

    install(TARGETS Promise EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY api/ DESTINATION include/celix/promise)

    if (PROMISE_STANDALONE)
        install(EXPORT celix NAMESPACE Celix:: DESTINATION share/CelixPromise/cmake FILE Targets.cmake COMPONENT cmake)
        install(FILES
                "cmake/CelixPromiseConfig.cmake"
                "cmake/CelixPromiseConfigVersion.cmake"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CelixPromise" COMPONENT cmake)
    endif ()

endif ()
