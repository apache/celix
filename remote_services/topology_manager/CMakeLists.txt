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
celix_subproject(RSA_TOPOLOGY_MANAGER "Option to enable building the Remote Service Admin Service SHM bundle" ON)
if (RSA_TOPOLOGY_MANAGER)
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/topology_manager/private/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")

    SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_rs_topology_manager")
    SET(BUNDLE_VERSION "0.0.1")
    SET_HEADERS("Bundle-Name: Apache Celix RS Topology Manager")

    bundle(topology_manager SOURCES
        private/src/topology_manager
        private/src/activator

        ${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c

        private/include/topology_manager.h
    )

    install_bundle(topology_manager)

    target_link_libraries(topology_manager celix_framework)
endif (RSA_TOPOLOGY_MANAGER)
