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

include_directories("private/include")
include_directories("${PROJECT_SOURCE_DIR}/framework/public/include")
include_directories("${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/include")
include_directories("${PROJECT_SOURCE_DIR}/pubsub/api/pubsub")

add_bundle(org.apache.celix.pubsub_publisher.PoiPublisher
    SYMBOLIC_NAME "apache_celix_pubsub_poi_publisher"
    VERSION "1.0.0"
    SOURCES 
    	private/src/ps_pub_activator.c
    	private/src/pubsub_publisher.c
    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_utils.c	
)

bundle_files(org.apache.celix.pubsub_publisher.PoiPublisher
		${PROJECT_SOURCE_DIR}/pubsub/examples/pubsub/msg_descriptors/msg_poi1.descriptor
		${PROJECT_SOURCE_DIR}/pubsub/examples/pubsub/msg_descriptors/msg_poi2.descriptor
    DESTINATION "META-INF/descriptors"
)

bundle_files(org.apache.celix.pubsub_publisher.PoiPublisher
		${PROJECT_SOURCE_DIR}/pubsub/examples/pubsub/msg_descriptors/poi1.properties
		${PROJECT_SOURCE_DIR}/pubsub/examples/pubsub/msg_descriptors/poi2.properties
    DESTINATION "META-INF/topics/pub"
)

bundle_files(org.apache.celix.pubsub_publisher.PoiPublisher
		${PROJECT_SOURCE_DIR}/pubsub/examples/keys/publisher
    DESTINATION "META-INF/keys"
)

bundle_files(org.apache.celix.pubsub_publisher.PoiPublisher
		${PROJECT_SOURCE_DIR}/pubsub/examples/keys/subscriber/public
    DESTINATION "META-INF/keys/subscriber"
)
