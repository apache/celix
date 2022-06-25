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

find_package(jansson REQUIRED)

add_celix_bundle(celix_pubsub_serializer_avrobin
        BUNDLE_SYMBOLICNAME "apache_celix_pubsub_serializer_avrobin"
        VERSION "1.1.0"
        GROUP "Celix/PubSub"
        SOURCES
        src/ps_avrobin_serializer_activator.c
        src/pubsub_avrobin_serializer_impl.c
        src/pubsub_avrobin_serialization_provider.c
)
target_include_directories(celix_pubsub_serializer_avrobin PRIVATE
        src
        ${JANSSON_INCLUDE_DIR}
)
set_target_properties(celix_pubsub_serializer_avrobin PROPERTIES INSTALL_RPATH "$ORIGIN")
target_link_libraries(celix_pubsub_serializer_avrobin PRIVATE Celix::framework Celix::dfi ${JANSSON_LIBRARIES} Celix::log_helper)
target_link_libraries(celix_pubsub_serializer_avrobin PRIVATE Celix::pubsub_spi Celix::pubsub_utils )

install_celix_bundle(celix_pubsub_serializer_avrobin EXPORT celix COMPONENT pubsub)

add_library(Celix::celix_pubsub_serializer_avrobin ALIAS celix_pubsub_serializer_avrobin)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif(ENABLE_TESTING)
