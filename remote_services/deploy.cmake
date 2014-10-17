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
is_enabled(REMOTE_SERVICE_ADMIN)
if (REMOTE_SERVICE_ADMIN)
    deploy("remote-services-cfg" BUNDLES discovery_configured topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer
                                 ENDPOINTS 
                                    org.apache.celix.calc.api.Calculator_endpoint
                                    org.apache.celix.calc.api.Calculator2_endpoint)
    deploy("remote-services-cfg-client" BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_configured
                                        ENDPOINTS org.apache.celix.calc.api.Calculator_proxy)

	#TODO for remote-service-client the discovery should be added as last. If this is not done, 
	#discovery will discover services before the topology manager is registered as 
	#endpoint listener and services will be lost. This needs further study.

    is_enabled(RSA_BUNDLES_DISCOVERY_BONJOUR)
    if (RSA_BUNDLES_DISCOVERY_BONJOUR)
        deploy("remote-services-bj" BUNDLES discovery_bonjour topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer 
                                    ENDPOINTS org.apache.celix.calc.api.Calculator_endpoint)
        deploy("remote-services-bj-client" BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_bonjour
                                           ENDPOINTS org.apache.celix.calc.api.Calculator_proxy)
    endif (RSA_BUNDLES_DISCOVERY_BONJOUR)


    is_enabled(RSA_BUNDLES_DISCOVERY_SLP)
    if (RSA_BUNDLES_DISCOVERY_SLP)
        deploy("remote-services-slp" BUNDLES discovery_slp topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer
                                     ENDPOINTS org.apache.celix.calc.api.Calculator_endpoint)
        deploy("remote-services-slp-client" BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_slp
                                            ENDPOINTS org.apache.celix.calc.api.Calculator_proxy)
    endif (RSA_BUNDLES_DISCOVERY_SLP)


    is_enabled(RSA_BUNDLES_REMOTE_SERVICE_ADMIN_SHM)
    if (RSA_BUNDLES_REMOTE_SERVICE_ADMIN_SHM)
        is_enabled(RSA_BUNDLES_DISCOVERY_SHM)
        if (RSA_BUNDLES_DISCOVERY_SHM)
            deploy("remote-services-shm" BUNDLES discovery_shm topology_manager remote_service_admin_shm calculator shell shell_tui log_service log_writer 
                                         ENDPOINTS org.apache.celix.calc.api.Calculator_endpoint)
            deploy("remote-services-shm-client" BUNDLES topology_manager remote_service_admin_shm shell shell_tui log_service log_writer calculator_shell discovery_shm
                                                ENDPOINTS org.apache.celix.calc.api.Calculator_proxy)
        endif (RSA_BUNDLES_DISCOVERY_SHM)
    endif (RSA_BUNDLES_REMOTE_SERVICE_ADMIN_SHM)

    is_enabled(RSA_BUNDLES_DISCOVERY_ETCD)
    if (RSA_BUNDLES_DISCOVERY_ETCD)
        deploy("remote-services-etcd" BUNDLES discovery_etcd topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer
                                     ENDPOINTS org.apache.celix.calc.api.Calculator_endpoint)
        deploy("remote-services-etcd-client" BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_etcd
                                            ENDPOINTS org.apache.celix.calc.api.Calculator_proxy)
    endif (RSA_BUNDLES_DISCOVERY_ETCD)

endif (REMOTE_SERVICE_ADMIN)
