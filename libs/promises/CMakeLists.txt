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
    #act as a top level project. Making the Celix::Promises useable
    #stand-alone

    cmake_minimum_required (VERSION 3.11)
    project(celix_Promises
            VERSION 1.0.0
            LANGUAGES CXX
    )

    include(GNUInstallDirs)

    set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

    set(PROMISES_STANDALONE ON)
else ()
    set(PROMISES_DEFAULT_ON ${CELIX_CXX})
    celix_subproject(PROMISES "Option to build the Promises library" ${PROMISES_DEFAULT_ON})
endif ()

if (PROMISES OR PROMISES_STANDALONE)
    find_package(Threads)

    add_library(Promises INTERFACE)
    target_include_directories(Promises INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
        $<INSTALL_INTERFACE:include/celix/promises>
    )
    target_link_libraries(Promises INTERFACE Threads::Threads)
    target_compile_options(Promises INTERFACE -std=c++17)
    add_library(Celix::Promises ALIAS Promises)

    add_executable(PromiseExamples src/PromiseExamples.cc)
    target_link_libraries(PromiseExamples PRIVATE Celix::Promises)

    if (ENABLE_TESTING AND NOT PROMISE_STANDALONE)
        add_subdirectory(gtest)
    endif()

    install(TARGETS Promises EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY api/ DESTINATION include/celix/promises)

    if (PROMISE_STANDALONE)
        install(EXPORT celix NAMESPACE Celix:: DESTINATION share/CelixPromises/cmake FILE Targets.cmake COMPONENT cmake)
        install(FILES
                "cmake/CelixPromisesConfig.cmake"
                "cmake/CelixPromisesConfigVersion.cmake"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CelixPromises" COMPONENT cmake)
    endif ()

endif ()
