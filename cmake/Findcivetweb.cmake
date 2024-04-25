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

find_package(civetweb CONFIG QUIET)
if (NOT civetweb_FOUND)
    include(FetchContent)
    set(CIVETWEB_ENABLE_WEBSOCKETS TRUE CACHE BOOL "" FORCE)
    set(CIVETWEB_BUILD_TESTING FALSE CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS TRUE CACHE BOOL "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
    FetchContent_Declare(
            civetweb
            GIT_REPOSITORY https://github.com/civetweb/civetweb.git
#            GIT_REPOSITORY https://gitee.com/mirrors/civetweb.git
            GIT_TAG        d7ba35bbb649209c66e582d5a0244ba988a15159 # V1.16
    )
    FetchContent_MakeAvailable(civetweb)
    if (NOT TARGET civetweb::civetweb)
        add_library(civetweb::civetweb ALIAS civetweb-c-library)
    endif ()
endif()
