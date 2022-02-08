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
    add_subdirectory(examples)
    add_subdirectory(deprecated_rsa_spi)
    add_subdirectory(rsa_common)
    add_subdirectory(civetweb)
    add_subdirectory(topology_manager)
    add_subdirectory(discovery_common)
    add_subdirectory(discovery_configured)
    add_subdirectory(discovery_etcd)
    add_subdirectory(remote_service_admin_dfi)

    if (APPLE)
        set(RSA_SHM_DEFAULT_ENABLED OFF)
    else ()
        set(RSA_SHM_DEFAULT_ENABLED ON)
    endif ()

    celix_subproject(RSA_SHM "Option to enable building the Discovery (SHM) bundle" ${RSA_SHM_DEFAULT_ENABLED})
    if (RSA_SHM)
        add_subdirectory(discovery_shm)
        #TODO refactor shm rsa to use dfi
        #add_subdirectory(remote_service_admin_shm)
    endif ()

    if (BUILD_RSA_DISCOVERY_ETCD AND BUILD_RSA_REMOTE_SERVICE_ADMIN_DFI)
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
                Celix::rsa_dfi Celix::shell
                Celix::shell_tui
                Celix::log_admin
                Celix::rsa_discovery_etcd
                celix_remote_interceptors_example
                calculator_shell
                PROPERTIES
                RSA_PORT=28888
                )
    endif()
endif (REMOTE_SERVICE_ADMIN)


