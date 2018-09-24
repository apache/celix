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

add_library(pubsub_spi STATIC
        src/pubsub_utils_match.c
        src/pubsub_endpoint.c
        src/pubsub_utils.c
)
set_target_properties(pubsub_spi PROPERTIES OUTPUT_NAME "celix_pubsub_spi")
target_include_directories(pubsub_spi PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
		$<INSTALL_INTERFACE:include/celix/pubsub_spi>
)
target_link_libraries(pubsub_spi PUBLIC Celix::framework Celix::pubsub_api)

set_target_properties(pubsub_spi PROPERTIES TOPIC_INFO_DESCRIPTOR ${CMAKE_CURRENT_LIST_DIR}/include/pubsub_topic_info.descriptor)
#TODO how to make this descriptor available for imported targets? $<INSTALL_INTERFACE:include/celix/pubsub_spi/pubsub_topic_info.descriptor>

add_library(Celix::pubsub_spi ALIAS pubsub_spi)

install(TARGETS pubsub_spi EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT pubsub)
install(DIRECTORY include/ DESTINATION include/celix/pubsub_spi COMPONENT pubsub)
