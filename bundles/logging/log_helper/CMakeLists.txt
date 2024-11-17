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

celix_subproject(LOG_HELPER "Option to enable building the log helper library" ON)
if (LOG_HELPER)
    add_library(log_helper STATIC src/celix_log_helper.c)
    set_target_properties(log_helper PROPERTIES OUTPUT_NAME "celix_log_utils")
    target_include_directories(log_helper PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
            )
    target_link_libraries(log_helper PUBLIC Celix::log_service_api Celix::framework PRIVATE Celix::utils)

    install(TARGETS log_helper EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT logging
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/log_helper)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/log_helper COMPONENT logging)

    celix_target_hide_symbols(log_helper)

    add_library(Celix::log_helper ALIAS log_helper)

    if (ENABLE_TESTING)
        if (EI_TESTS)
            add_subdirectory(error_injector)
        endif ()
        add_subdirectory(gtest)
    endif ()
endif ()
