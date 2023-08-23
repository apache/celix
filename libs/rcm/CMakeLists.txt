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

celix_subproject(RCM "Option to enable building the Requirement-Capability-Model library" ON)
if (RCM)
    set(RCM_SOURCES src/celix_resource.c src/celix_capability.c src/celix_requirement.c)
    set(RCM_PUBLIC_LIBS Celix::utils)
    SET(RCM_PRIVATE_LIBS )

    add_library(rcm STATIC ${RCM_SOURCES})
    set_target_properties(rcm PROPERTIES
            OUTPUT_NAME "celix_rcm"
            VERSION 0.0.1
            SOVERSION 0)
    target_link_libraries(rcm PUBLIC ${RCM_PUBLIC_LIBS} PRIVATE ${RCM_PRIVATE_LIBS})
    target_include_directories(rcm PRIVATE src PUBLIC include)
    celix_target_hide_symbols(rcm)
    add_library(Celix::rcm ALIAS rcm)

    if (ENABLE_TESTING)
        add_library(rcm_cut STATIC ${RCM_SOURCES})
        target_link_libraries(rcm_cut PUBLIC ${RCM_PUBLIC_LIBS} PRIVATE ${RCM_PRIVATE_LIBS})
        target_include_directories(rcm_cut PRIVATE src PUBLIC include)

        add_subdirectory(gtest)
    endif ()
endif ()
