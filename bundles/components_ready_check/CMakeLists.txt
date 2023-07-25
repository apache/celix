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

celix_subproject(COMPONENTS_READY_CHECK "Bundle that checks if all components becomes active and reports this using a condition service" ON)
if (COMPONENTS_READY_CHECK)

    add_library(components_ready_api INTERFACE)
    target_include_directories(components_ready_api INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>)
    add_library(Celix::components_ready_api ALIAS components_ready_api)
    install(TARGETS components_ready_api EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT components_ready
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix_components_ready_api)
    install(DIRECTORY api/ DESTINATION include/celix_components_ready_api COMPONENT components_ready)


    set(COMPONENTS_READY_CHECK_SRC src/celix_components_ready_check.c)
    set(COMPONENTS_READY_CHECK_DEPS Celix::framework Celix::components_ready_api)
    add_celix_bundle(components_ready_check
        SOURCES src/celix_components_ready_check_activator.c ${COMPONENTS_READY_CHECK_SRC}
        VERSION 1.0.0
        SYMBOLIC_NAME "apache_celix_components_ready_check"
        GROUP "Celix/Conditions"
        NAME "Apache Celix Components Ready Check"
        FILENAME celix_components_ready_check
    )
    target_include_directories(components_ready_check PRIVATE src)
    target_link_libraries(components_ready_check PRIVATE ${COMPONENTS_READY_CHECK_DEPS})
    add_library(Celix::components_ready_check ALIAS components_ready_check)
    install_celix_bundle(components_ready_check EXPORT celix COMPONENT components_ready)

    if (ENABLE_TESTING)
        add_library(components_ready_cut STATIC ${COMPONENTS_READY_CHECK_SRC})
        target_include_directories(components_ready_cut PUBLIC src)
        target_link_libraries(components_ready_cut PUBLIC ${COMPONENTS_READY_CHECK_DEPS})
        add_subdirectory(gtest)
    endif()
endif ()
