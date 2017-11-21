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

add_bundle(org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_topology_manager"
    VERSION "1.0.0"
    SOURCES
    	private/src/pstm_activator.c
    	private/src/pubsub_topology_manager.c
    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_endpoint.c
    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_utils.c	
)

bundle_files(org.apache.celix.pubsub_topology_manager.PubSubTopologyManager
   ${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/include/pubsub_topic_info.descriptor
    DESTINATION "META-INF/descriptors/services"
)
target_link_libraries(org.apache.celix.pubsub_topology_manager.PubSubTopologyManager PRIVATE Celix::framework Celix::log_helper)


install_bundle(org.apache.celix.pubsub_topology_manager.PubSubTopologyManager)

