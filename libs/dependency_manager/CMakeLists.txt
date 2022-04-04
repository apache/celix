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

#dummy libaries to ensure backward compatability with projects using the dependency manager libs/shell
add_library(dependency_manager_static STATIC src/dm_activator.c)
target_include_directories(dependency_manager_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
)
if (APPLE)
    target_link_libraries(dependency_manager_static Celix::framework "-undefined dynamic_lookup")
else()
    target_link_libraries(dependency_manager_static Celix::framework)
endif()

add_library(dependency_manager_so SHARED src/dm_activator.c)
target_include_directories(dependency_manager_so PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
)
if (APPLE)
    target_link_libraries(dependency_manager_so Celix::framework "-undefined dynamic_lookup")
else()
    target_link_libraries(dependency_manager_so Celix::framework)
endif()

#now part of the the shell bundle
add_library(dm_shell INTERFACE)

#Setup target aliases to match external usage
add_library(Celix::dm_shell ALIAS dm_shell)
add_library(Celix::dependency_manager_static ALIAS dependency_manager_static)
add_library(Celix::dependency_manager_so ALIAS dependency_manager_so)

#install dummy libs
install(TARGETS dependency_manager_static dependency_manager_so dm_shell EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix)

