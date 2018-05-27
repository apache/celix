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

add_celix_bundle(celix_pubsub_discovery_etcd
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_discovery_etcd"
    VERSION "1.0.0"
    SOURCES
        src/psd_activator.c
        src/pubsub_discovery_impl.c
        src/etcd_common.c
        src/etcd_watcher.c
        src/etcd_writer.c
)
target_include_directories(celix_pubsub_discovery_etcd PRIVATE src)
target_include_directories(celix_pubsub_discovery_etcd SYSTEM PRIVATE 
	${CURL_INCLUDE_DIR}
	${JANSSON_INCLUDE_DIR}
)
target_link_libraries(celix_pubsub_discovery_etcd PRIVATE Celix::pubsub_spi Celix::framework Celix::etcdlib_static ${CURL_LIBRARIES} ${JANSSON_LIBRARIES})

install_celix_bundle(celix_pubsub_discovery_etcd EXPORT celix COMPONENT pubsub)

add_library(Celix::pubsub_discovery_etcd ALIAS celix_pubsub_discovery_etcd)
