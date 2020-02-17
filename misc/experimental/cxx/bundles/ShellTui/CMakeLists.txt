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

add_library(celix_ShellTui_static STATIC
        src/ShellTui.cc
)
target_include_directories(celix_ShellTui_static PUBLIC include)
target_link_libraries(celix_ShellTui_static PRIVATE celix::cxx::Framework glog::glog celix::ShellApi)
target_compile_options(celix_ShellTui_static PRIVATE -fPIC)


#OR static lib, but then with all symbols to force constructor attribute
add_library(celix_ShellTui SHARED
        src/ShellTuiActivator.cc
)
target_include_directories(celix_ShellTui PRIVATE src)
target_link_libraries(celix_ShellTui PRIVATE celix_ShellTui_static celix::ShellApi glog::glog)
target_link_libraries(celix_ShellTui PUBLIC celix::cxx::Framework)

add_library(celix::ShellTui_static ALIAS celix_ShellTui_static)
add_library(celix::ShellTui ALIAS celix_ShellTui)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()