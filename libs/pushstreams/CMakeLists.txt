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
    #act as a top level project. Making the Celix::CelixPushStreams useable
    #stand-alone

    cmake_minimum_required (VERSION 3.11)
    project(celix_PushStreams
            VERSION 1.0.0
            LANGUAGES CXX
    )

    include(GNUInstallDirs)

    set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

    set(PUSHSTREAMS_STANDALONE ON)
else ()
    set(PUSHSTREAMS_DEFAULT_ON ${CELIX_CXX})
    celix_subproject(PUSHSTREAMS "Option to build the PushStreams library" ${PUSHSTREAMS_DEFAULT_ON})
endif ()

if (PUSHSTREAMS OR PUSHSTREAMS_STANDALONE)
    find_package(Threads)

    add_library(PushStreams INTERFACE)
    target_include_directories(PushStreams INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
        $<INSTALL_INTERFACE:include/celix/pushstreams>
    )
    target_link_libraries(PushStreams INTERFACE Threads::Threads)
    add_library(Celix::PushStreams ALIAS PushStreams)

    add_executable(PushStreamExamples src/PushStreamExamples.cc)
    target_compile_options(PushStreamExamples PRIVATE -std=c++17)
    target_link_libraries(PushStreamExamples PRIVATE Celix::PushStreams)

    if (ENABLE_TESTING AND NOT PUSHSTREAMS_STANDALONE)
#        add_subdirectory(gtest)
    endif()

    install(TARGETS PushStreams EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY api/ DESTINATION include/celix/pushstreams)

    if (PUSHSTREAMS_STANDALONE)
        install(EXPORT celix NAMESPACE Celix:: DESTINATION share/CelixPushStreams/cmake FILE Targets.cmake COMPONENT cmake)
        install(FILES
                "cmake/CelixPushStreamsConfig.cmake"
                "cmake/CelixPushStreamsConfigVersion.cmake"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CelixPushStreams" COMPONENT cmake)
    endif ()

endif ()
