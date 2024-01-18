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

set(EVENT_ADMIN_SRC
        src/celix_event_admin_activator.c
        src/celix_event_admin.c
        src/celix_event.c
        src/celix_event_adapter.c
        )

set(EVENT_ADMIN_DEPS
        Celix::event_admin_api
        Celix::log_helper
        Celix::framework
        Celix::utils
        )

add_celix_bundle(event_admin
    SYMBOLIC_NAME "apache_celix_event_admin"
    VERSION "1.0.0"
    NAME "Apache Celix Event Admin"
    GROUP "Celix/event_admin"
    FILENAME celix_event_admin
    SOURCES
    ${EVENT_ADMIN_SRC}
)

target_link_libraries(event_admin PRIVATE ${EVENT_ADMIN_DEPS})

target_include_directories(event_admin PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

install_celix_bundle(event_admin EXPORT celix COMPONENT event_admin)

#Setup target aliases to match external usage
add_library(Celix::event_admin ALIAS event_admin)

if (ENABLE_TESTING)
    add_library(event_admin_cut STATIC ${EVENT_ADMIN_SRC})
    target_include_directories(event_admin_cut PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
    target_link_libraries(event_admin_cut PUBLIC ${EVENT_ADMIN_DEPS})
    add_subdirectory(gtest)
endif(ENABLE_TESTING)


