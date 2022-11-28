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

    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${CMAKE_CXX_FLAGS_DEBUG}")

    set(PROMISES_STANDALONE ON)
else ()
    set(PROMISES_DEFAULT_ON ${CELIX_CXX})
    celix_subproject(PROMISES "Option to build the Promises library" ${PROMISES_DEFAULT_ON})
endif ()

if (PROMISES OR PROMISES_STANDALONE)
    message("The Celix::Promises implementation is still experiment and the api and behaviour will probably still change.")

    set(CMAKE_CXX_STANDARD 17)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads)

    add_library(Promises INTERFACE)
    target_include_directories(Promises INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
    )
    target_link_libraries(Promises INTERFACE Threads::Threads)
    add_library(Celix::Promises ALIAS Promises)

    add_executable(PromiseExamples src/PromiseExamples.cc)
    target_link_libraries(PromiseExamples PRIVATE Celix::Promises)

    if (ENABLE_TESTING AND NOT PROMISE_STANDALONE)
        add_subdirectory(gtest)
    endif()

    install(TARGETS Promises EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/promises)
    install(DIRECTORY api/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/promises)

    if (PROMISE_STANDALONE)
        install(EXPORT celix NAMESPACE Celix:: DESTINATION share/CelixPromises/cmake FILE Targets.cmake COMPONENT cmake)
        install(FILES
                "cmake/CelixPromisesConfig.cmake"
                "cmake/CelixPromisesConfigVersion.cmake"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CelixPromises" COMPONENT cmake)
    endif ()

endif ()
