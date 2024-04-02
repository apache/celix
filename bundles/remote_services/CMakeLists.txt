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
    add_subdirectory(remote_services_api)
    add_subdirectory(rsa_spi)
    add_subdirectory(rsa_common)
    add_subdirectory(rsa_utils)
    add_subdirectory(rsa_dfi_utils)
    add_subdirectory(discovery_common)
    add_subdirectory(discovery_configured)
    add_subdirectory(discovery_etcd)
    add_subdirectory(discovery_zeroconf)
    add_subdirectory(examples)
    add_subdirectory(topology_manager)
    add_subdirectory(remote_service_admin_dfi)
    add_subdirectory(rsa_rpc_json)
    add_subdirectory(remote_service_admin_shm_v2)

    if (BUILD_RSA_DISCOVERY_ETCD AND BUILD_RSA_REMOTE_SERVICE_ADMIN_DFI AND BUILD_SHELL AND BUILD_SHELL_TUI AND BUILD_LOG_SERVICE AND BUILD_LAUNCHER)
        add_celix_container(remote-services-dfi
                LAUNCHER Celix::launcher
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
                USE_CONFIG
                PROPERTIES
                RSA_PORT=18888
                )

        add_celix_container(remote-services-dfi-client
                LAUNCHER Celix::launcher
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
                USE_CONFIG
                PROPERTIES
                RSA_PORT=28888
                )
    endif()
    
    if (BUILD_RSA_DISCOVERY_CONFIGURED AND BUILD_RSA_REMOTE_SERVICE_ADMIN_SHM_V2 AND BUILD_RSA_JSON_RPC AND BUILD_SHELL AND BUILD_SHELL_TUI AND BUILD_LOG_SERVICE AND BUILD_LAUNCHER)
        add_celix_container(remote-services-shm-server
                LAUNCHER Celix::launcher
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
                USE_CONFIG
                PROPERTIES
                )

        add_celix_container(remote-services-shm-client
                LAUNCHER Celix::launcher
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
                USE_CONFIG
                PROPERTIES
                )
    endif()

    if (BUILD_RSA_DISCOVERY_ZEROCONF AND BUILD_RSA_REMOTE_SERVICE_ADMIN_DFI AND BUILD_RSA_REMOTE_SERVICE_ADMIN_SHM_V2 AND BUILD_RSA_JSON_RPC AND BUILD_SHELL AND BUILD_SHELL_TUI AND BUILD_LOG_SERVICE AND BUILD_LAUNCHER)
        add_celix_container(remote-services-zeroconf-server
                LAUNCHER Celix::launcher
                NAME "server"
                GROUP "remote-services/remote-services-zeroconf"
                BUNDLES
                Celix::rsa_discovery_zeroconf
                Celix::rsa_topology_manager
                Celix::rsa_shm
                Celix::rsa_json_rpc
                Celix::rsa_dfi
                Celix::shell
                Celix::shell_tui
                Celix::log_admin
                celix_remote_interceptors_example
                calculator
                USE_CONFIG
                PROPERTIES
                CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT=true
                RSA_PORT=19999
                CELIX_RSA_INTERFACES_OF_PORT_19999=lo
                )

        add_celix_container(remote-services-zeroconf-client
                LAUNCHER Celix::launcher
                NAME "client"
                GROUP "remote-services/remote-services-zeroconf"
                BUNDLES
                Celix::rsa_discovery_zeroconf
                Celix::rsa_topology_manager
                Celix::rsa_shm
                Celix::rsa_json_rpc
                Celix::rsa_dfi
                Celix::shell
                Celix::shell_tui
                Celix::log_admin
                celix_remote_interceptors_example
                calculator_shell
                USE_CONFIG
                PROPERTIES
                CELIX_RSA_DFI_DYNAMIC_IP_SUPPORT=true
                RSA_PORT=29999
                CELIX_RSA_INTERFACES_OF_PORT_29999=lo
                )
    endif()
    
endif (REMOTE_SERVICE_ADMIN)


