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
find_package(Jansson REQUIRED)

include_directories("${CURL_INCLUDE_DIR}")
include_directories("${JANSSON_INCLUDE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/include")
include_directories("${PROJECT_SOURCE_DIR}/pubsub/api/pubsub")
include_directories("${PROJECT_SOURCE_DIR}/etcdlib/public/include")
include_directories("private/include")
include_directories("public/include")

add_bundle(org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_discovery_etcd"
    VERSION "1.0.0"
    SOURCES
		private/src/psd_activator.c
		private/src/pubsub_discovery_impl.c
		private/src/etcd_common.c
		private/src/etcd_watcher.c
		private/src/etcd_writer.c
		${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_endpoint.c
		${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_utils.c
)

target_link_libraries(org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery celix_framework celix_utils etcdlib_static ${CURL_LIBRARIES} ${JANSSON_LIBRARIES})
install_bundle(org.apache.celix.pubsub_discovery.etcd.PubsubDiscovery)
