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


find_package(CURL REQUIRED)


add_celix_bundle(celix_pubsub_websocket_example
    SYMBOLIC_NAME "apache_celix_pubsub_websocket_example"
    VERSION "1.0.0"
    SOURCES
        private/src/ps_websocket_activator.c
        private/src/pubsub_websocket_example.c
)
target_link_libraries(celix_pubsub_websocket_example PRIVATE Celix::framework Celix::pubsub_api ${CURL_LIBRARIES})
target_include_directories(celix_pubsub_websocket_example PRIVATE private/include)

celix_bundle_files(celix_pubsub_websocket_example
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/msg_poi1.descriptor
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/msg_poi2.descriptor
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/msg_poiCmd.descriptor
    DESTINATION "META-INF/descriptors"
)

celix_bundle_files(celix_pubsub_websocket_example
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/poi1.properties
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/poi2.properties
    DESTINATION "META-INF/topics/pub"
)
celix_bundle_files(celix_pubsub_websocket_example
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/poiCmd.properties
    DESTINATION "META-INF/topics/sub"
)

celix_bundle_files(celix_pubsub_websocket_example
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/keys/publisher
    DESTINATION "META-INF/keys"
)

celix_bundle_files(celix_pubsub_websocket_example
    ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/keys/subscriber/public
    DESTINATION "META-INF/keys/subscriber"
)

celix_bundle_add_dir(celix_pubsub_websocket_example resources DESTINATION ".")

celix_bundle_headers(celix_pubsub_websocket_example
    "X-Web-Resource: /example$<SEMICOLON>/resources"
)
