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

add_library(celix_wire_protocol_v1_impl STATIC
    src/pubsub_wire_protocol_impl.c
    src/pubsub_wire_protocol_common.c
)
target_include_directories(celix_wire_protocol_v1_impl PRIVATE src)
target_link_libraries(celix_wire_protocol_v1_impl PRIVATE Celix::pubsub_spi )


add_celix_bundle(celix_pubsub_protocol_wire_v1
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_protocol_wire_v1"
    VERSION "1.0.0"
    GROUP "Celix/PubSub"
    SOURCES
        src/ps_wire_protocol_activator.c
)
target_include_directories(celix_pubsub_protocol_wire_v1 PRIVATE src)
target_link_libraries(celix_pubsub_protocol_wire_v1 PRIVATE Celix::pubsub_spi Celix::pubsub_utils)
target_link_libraries(celix_pubsub_protocol_wire_v1 PRIVATE celix_wire_protocol_v1_impl)

install_celix_bundle(celix_pubsub_protocol_wire_v1 EXPORT celix COMPONENT pubsub)

add_library(Celix::pubsub_protocol_wire_v1 ALIAS celix_pubsub_protocol_wire_v1)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif()