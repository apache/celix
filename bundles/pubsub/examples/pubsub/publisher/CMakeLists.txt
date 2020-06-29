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

add_celix_bundle(celix_pubsub_poi_publisher
    SYMBOLIC_NAME "apache_celix_pubsub_poi_publisher"
    VERSION "1.0.0"
    SOURCES 
        private/src/ps_pub_activator.c
        private/src/pubsub_publisher.c
)

target_link_libraries(celix_pubsub_poi_publisher PRIVATE Celix::framework Celix::pubsub_api)
target_include_directories(celix_pubsub_poi_publisher PRIVATE private/include)

celix_bundle_files(celix_pubsub_poi_publisher
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/msg_poi1.descriptor
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/msg_poi2.descriptor
    DESTINATION "META-INF/descriptors"
)

celix_bundle_files(celix_pubsub_poi_publisher
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/poi1.properties
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/pubsub/msg_descriptors/poi2.properties
    DESTINATION "META-INF/topics/pub"
)

celix_bundle_files(celix_pubsub_poi_publisher
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/keys/publisher
    DESTINATION "META-INF/keys"
)

celix_bundle_files(celix_pubsub_poi_publisher
        ${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/keys/subscriber/public
    DESTINATION "META-INF/keys/subscriber"
)
