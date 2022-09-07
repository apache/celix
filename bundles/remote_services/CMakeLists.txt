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

celix_subproject(REMOTE_SERVICE_ADMIN "Option to enable building the Remote Service Admin Service bundles" ON)
if (REMOTE_SERVICE_ADMIN)

    add_subdirectory(thpool)
    add_subdirectory(remote_services_api)
    add_subdirectory(rsa_spi)
    add_subdirectory(rsa_common)
    add_subdirectory(rsa_dfi_utils)
    add_subdirectory(civetweb)
    add_subdirectory(discovery_common)
    add_subdirectory(discovery_configured)
    add_subdirectory(discovery_etcd)
    add_subdirectory(examples)
    add_subdirectory(topology_manager)
    add_subdirectory(remote_service_admin_dfi)
    add_subdirectory(rsa_rpc_json)
    add_subdirectory(remote_service_admin_shm_v2)

    celix_subproject(RSA_SHM "Option to enable building the Discovery (SHM) bundle" OFF)
    if (RSA_SHM)
        add_subdirectory(discovery_shm)
        # TODO refactor shm rsa to use dfi
        # TODO make it work on Apple
        #add_subdirectory(remote_service_admin_shm)
    endif ()

    if (BUILD_RSA_DISCOVERY_ETCD AND BUILD_RSA_REMOTE_SERVICE_ADMIN_DFI AND BUILD_SHELL AND BUILD_SHELL_TUI AND BUILD_LOG_SERVICE)
        add_celix_container(remote-services-dfi
                NAME "server"
                GROUP "remote-services/remote-services-dfi"
                BUNDLES
                Celix::rsa_discovery_etcd
                Celix::rsa_topology_manager
                Celix::rsa_dfi
                Celix::shell
                Celix::shell_tui
                Celix::log_admin

                celix_remote_interceptors_example
                calculator
                PROPERTIES
                RSA_PORT=18888
                )

        add_celix_container(remote-services-dfi-client
                NAME "client"
                GROUP "remote-services/remote-services-dfi"
                BUNDLES
                Celix::rsa_topology_manager
                Celix::rsa_dfi
                Celix::shell
                Celix::shell_tui
                Celix::log_admin
                Celix::rsa_discovery_etcd
                celix_remote_interceptors_example
                calculator_shell
                PROPERTIES
                RSA_PORT=28888
                )
    endif()
    
    if (BUILD_RSA_DISCOVERY_CONFIGURED AND BUILD_RSA_REMOTE_SERVICE_ADMIN_SHM_V2 AND BUILD_RSA_JSON_RPC AND BUILD_SHELL AND BUILD_SHELL_TUI AND BUILD_LOG_SERVICE)
        add_celix_container(remote-services-shm-server
                NAME "server"
                GROUP "remote-services/remote-services-shm"
                BUNDLES
                Celix::rsa_discovery
                Celix::rsa_topology_manager
                Celix::rsa_shm
                Celix::rsa_json_rpc
                Celix::shell
                Celix::shell_tui
                Celix::log_admin

                celix_remote_interceptors_example
                calculator
                PROPERTIES
                )

        add_celix_container(remote-services-shm-client
                NAME "client"
                GROUP "remote-services/remote-services-shm"
                BUNDLES
                Celix::rsa_discovery
                Celix::rsa_topology_manager
                Celix::rsa_shm
                Celix::rsa_json_rpc
                Celix::shell
                Celix::shell_tui
                Celix::log_admin
                celix_remote_interceptors_example
                calculator_shell
                PROPERTIES
                )
    endif()
    
endif (REMOTE_SERVICE_ADMIN)


