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

add_celix_bundle(org.apache.celix.pubsub_publisher.MpPublisher
    SYMBOLIC_NAME "apache_celix_pubsub_mp_publisher"
    VERSION "1.0.0"
    SOURCES 
    	private/src/mp_pub_activator.c
    	private/src/mp_publisher.c
)
target_link_libraries(org.apache.celix.pubsub_publisher.MpPublisher PRIVATE Celix::framework Celix::pubsub_api)
target_include_directories(org.apache.celix.pubsub_publisher.MpPublisher PRIVATE private/include)

celix_bundle_files(org.apache.celix.pubsub_publisher.MpPublisher
	${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/mp_pubsub/msg_descriptors/msg_ew.descriptor
	${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/mp_pubsub/msg_descriptors/msg_ide.descriptor
	${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/mp_pubsub/msg_descriptors/msg_kinematics.descriptor
	DESTINATION "META-INF/descriptors"
)

celix_bundle_files(org.apache.celix.pubsub_publisher.MpPublisher
	${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/keys/publisher
    DESTINATION "META-INF/keys"
)

celix_bundle_files(org.apache.celix.pubsub_publisher.MpPublisher
	${PROJECT_SOURCE_DIR}/bundles/pubsub/examples/keys/subscriber/public
    DESTINATION "META-INF/keys/subscriber"
)
