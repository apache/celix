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

add_library(celix_ShellTuiBundle_static STATIC
        src/ShellTui.cc
)
target_include_directories(celix_ShellTuiBundle_static PUBLIC include)
target_link_libraries(celix_ShellTuiBundle_static PRIVATE Celix::cxx::Framework Celix::cxx::ShellApi)
target_compile_options(celix_ShellTuiBundle_static PRIVATE -fPIC)


#OR static lib, but then with all symbols to force constructor attribute
add_library(celix_ShellTuiBundle SHARED
        src/ShellTuiActivator.cc
)
target_include_directories(celix_ShellTuiBundle PRIVATE src)
target_link_libraries(celix_ShellTuiBundle PRIVATE celix_ShellTui_static Celix::cxx::ShellApi )
target_link_libraries(celix_ShellTuiBundle PUBLIC Celix::cxx::Framework)

add_library(Celix::cxx::ShellTuiBundle_static ALIAS celix_ShellTuiBundle_static)
add_library(Celix::cxx::ShellTuiBundle ALIAS celix_ShellTuiBundle)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif ()