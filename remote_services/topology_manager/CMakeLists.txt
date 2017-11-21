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

    add_bundle(topology_manager
        SOURCES
            src/topology_manager
            src/scope
            src/activator
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_rs_topology_manager"
        NAME "Apache Celix RS Topology Manager"
    )
    target_include_directories(topology_manager PRIVATE src)
    target_include_directories(topology_manager PUBLIC include)
    target_link_libraries(topology_manager PRIVATE Celix::log_helper remote_service_admin_api)

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
