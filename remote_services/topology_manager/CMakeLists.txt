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
celix_subproject(RSA_TOPOLOGY_MANAGER "Option to enable building the Remote Service Admin Service SHM bundle" ON DEPS REMOTE_SERVICE_ADMIN_DFI)
if (RSA_TOPOLOGY_MANAGER)
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/topology_manager/private/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/endpoint_listener/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/remote_service_admin/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/remote_services/topology_manager/public/include")

    add_bundle(topology_manager SOURCES
        private/src/topology_manager
        private/src/scope
        private/src/activator

        private/include/topology_manager.h
        public/include/tm_scope.h
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_rs_topology_manager"
        NAME
            "Apache Celix RS Topology Manager"
    )
    target_link_libraries(topology_manager PRIVATE Celix::log_helper)

    install_bundle(topology_manager)

    if (ENABLE_TESTING)
        find_package(CppUTest REQUIRED)
	find_package(Jansson REQUIRED)
 
	include_directories("${JANSSON_INCLUDE_DIR}")
        include_directories(${CPPUTEST_INCLUDE_DIR})
        include_directories(${CPPUTEST_EXT_INCLUDE_DIR})
        add_subdirectory(tms_tst)
   endif (ENABLE_TESTING)
endif (RSA_TOPOLOGY_MANAGER)
