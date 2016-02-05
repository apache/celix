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

celix_subproject(RSA_EXAMPLES "Option to enable building the RSA examples" ON DEPS LAUNCHER shell_tui log_writer RSA_TOPOLOGY_MANAGER)
if (RSA_EXAMPLES)
    add_subdirectory(calculator_service)

    add_subdirectory(calculator_endpoint)
    add_subdirectory(calculator_endpoint2)

    add_subdirectory(calculator_proxy)
    add_subdirectory(calculator_proxy2)

    add_subdirectory(calculator_shell)

    if(BUILD_RSA_REMOTE_SERVICE_ADMIN_HTTP AND BUILD_RSA_DISCOVERY_CONFIGURED)
        add_deploy(remote-services-cfg-server 
            NAME "server"
            GROUP "remote-services/remote-services-cfg"
            BUNDLES discovery_configured topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer
            PROPERTIES
                RSA_PORT=8001
                DISCOVERY_CFG_POLL_ENDPOINTS=http://localhost:8082/org.apache.celix.discovery.configured
                DISCOVERY_CFG_SERVER_PORT=8081
        )
        deploy_bundles_dir(remote-services-cfg-server DIR_NAME "endpoints" BUNDLES
            org.apache.celix.calc.api.Calculator_endpoint
            org.apache.celix.calc.api.Calculator2_endpoint
        )

        add_deploy(remote-services-cfg-client
            NAME "client"
            GROUP "remote-services/remote-services-cfg"
            BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_configured
            PROPERTIES
                RSA_PORT=8002
                DISCOVERY_CFG_POLL_ENDPOINTS=http://localhost:8081/org.apache.celix.discovery.configured
                DISCOVERY_CFG_SERVER_PORT=8082
        )
        deploy_bundles_dir(remote-services-cfg-client DIR_NAME "endpoints"
            BUNDLES org.apache.celix.calc.api.Calculator_proxy org.apache.celix.calc.api.Calculator2_proxy
        )
    endif()

    if (BUILD_RSA_REMOTE_SERVICE_ADMIN_SHM AND BUILD_RSA_DISCOVERY_SHM)
        add_deploy(remote-services-shm 
            NAME "server"
            GROUP "remote-services/remote-services-shm"
            BUNDLES discovery_shm topology_manager remote_service_admin_shm calculator shell shell_tui log_service log_writer
        )
        deploy_bundles_dir(remote-services-shm DIR_NAME "endpoints"
            BUNDLES org.apache.celix.calc.api.Calculator_endpoint
        )

        add_deploy(remote-services-shm-client 
            NAME "client"
            GROUP "remote-services/remote-services-shm"
            BUNDLES topology_manager remote_service_admin_shm shell shell_tui log_service log_writer calculator_shell discovery_shm
        )
        deploy_bundles_dir(remote-services-shm-client DIR_NAME "endpoints"
            BUNDLES org.apache.celix.calc.api.Calculator_proxy
        )
    endif ()

    if (BUILD_RSA_DISCOVERY_ETCD AND BUILD_RSA_REMOTE_SERVICE_ADMIN_HTTP)
        add_deploy(remote-services-etcd 
            NAME "server"
            GROUP "remote-services/remote-services-etcd"
            BUNDLES discovery_etcd topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer
        )
        deploy_bundles_dir(remote-services-etcd DIR_NAME "endpoints"
            BUNDLES org.apache.celix.calc.api.Calculator_endpoint
        )

        add_deploy("remote-services-etcd-client" 
            NAME "client"
            GROUP "remote-services/remote-services-etcd"
            BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_etcd
        )
        deploy_bundles_dir(remote-services-etcd-client DIR_NAME "endpoints"
            BUNDLES org.apache.celix.calc.api.Calculator_proxy
        )
    endif ()
endif (RSA_EXAMPLES)


