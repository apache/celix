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

celix_subproject(PUBSUB_DISCOVERY_ETCD "Option to enable building the PubSub Discovery (ETCD) bundle" ON DEPS CELIX_ETCDLIB)
if (PUBSUB_DISCOVERY_ETCD)
    find_package(CURL REQUIRED)
    find_package(jansson REQUIRED)

    add_celix_bundle(celix_pubsub_discovery_etcd
            BUNDLE_SYMBOLICNAME "apache_celix_pubsub_discovery_etcd"
            VERSION "1.1.0"
            GROUP "Celix/PubSub"
            SOURCES
            src/psd_activator.c
            src/pubsub_discovery_impl.c
            )
    target_include_directories(celix_pubsub_discovery_etcd PRIVATE src)
    target_link_libraries(celix_pubsub_discovery_etcd PRIVATE
            Celix::framework Celix::etcdlib_static
            Celix::shell_api Celix::log_helper
            CURL::libcurl jansson::jansson
            )
    target_link_libraries(celix_pubsub_discovery_etcd PRIVATE Celix::pubsub_spi Celix::pubsub_utils)

    install_celix_bundle(celix_pubsub_discovery_etcd EXPORT celix COMPONENT pubsub)

    add_library(Celix::celix_pubsub_discovery_etcd ALIAS celix_pubsub_discovery_etcd)
endif ()
