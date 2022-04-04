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

set(PUSHSTREAMS_DEFAULT_ON ${CELIX_CXX})
celix_subproject(PUSHSTREAMS "Option to build the PushStreams library" ${PUSHSTREAMS_DEFAULT_ON} DEPS PROMISES)

if (PUSHSTREAMS)
    find_package(Threads)
    add_library(PushStreams INTERFACE)
    target_include_directories(PushStreams INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
    )
    target_link_libraries(PushStreams INTERFACE Threads::Threads)
    target_link_libraries(PushStreams INTERFACE Celix::Promises)
    add_library(Celix::PushStreams ALIAS PushStreams)

    add_executable(PushStreamExamples src/PushStreamExamples.cc)
    target_compile_options(PushStreamExamples PRIVATE -std=c++17)
    target_link_libraries(PushStreamExamples PRIVATE Celix::PushStreams)

    if (ENABLE_TESTING)
        add_subdirectory(gtest)
    endif()

    install(TARGETS PushStreams EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/pushstreams)
    install(DIRECTORY api/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/pushstreams)
endif ()
