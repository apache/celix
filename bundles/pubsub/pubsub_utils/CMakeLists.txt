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

add_library(pubsub_utils STATIC
        src/pubsub_utils.c
        src/pubsub_utils_url.c
        src/pubsub_serializer_handler.c
        src/pubsub_serialization_provider.c
        src/pubsub_matching.c
)

set_target_properties(pubsub_utils PROPERTIES OUTPUT_NAME "celix_pubsub_utils")
target_include_directories(pubsub_utils PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)
target_link_libraries(pubsub_utils PUBLIC Celix::framework Celix::pubsub_api Celix::log_helper Celix::shell_api)
target_link_libraries(pubsub_utils PRIVATE Celix::pubsub_spi)

add_library(Celix::pubsub_utils ALIAS pubsub_utils)

install(TARGETS pubsub_utils EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT pubsub
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/pubsub_utils)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/pubsub_utils COMPONENT pubsub)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif(ENABLE_TESTING)