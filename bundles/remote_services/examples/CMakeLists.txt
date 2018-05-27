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
    add_subdirectory(calculator_api)
    add_subdirectory(calculator_service)
    add_subdirectory(calculator_shell)


#    TODO refactor shm remote service admin to use dfi
#    if (BUILD_RSA_REMOTE_SERVICE_ADMIN_SHM AND BUILD_RSA_DISCOVERY_SHM)
#        add_celix_container(remote-services-shm
#            NAME "server"
#            GROUP "remote-services/remote-services-shm"
#            BUNDLES discovery_shm topology_manager remote_service_admin_shm calculator shell shell_tui log_service log_writer
#        )
#        celix_container_bundles_dir(remote-services-shm DIR_NAME "endpoints"
#            BUNDLES org.apache.celix.calc.api.Calculator_endpoint
#        )
#
#        add_celix_container(remote-services-shm-client
#            NAME "client"
#            GROUP "remote-services/remote-services-shm"
#            BUNDLES topology_manager remote_service_admin_shm shell shell_tui log_service log_writer calculator_shell discovery_shm
#        )
#        celix_container_bundles_dir(remote-services-shm-client DIR_NAME "endpoints"
#            BUNDLES org.apache.celix.calc.api.Calculator_proxy
#        )
#    endif ()


    if (BUILD_RSA_DISCOVERY_ETCD AND BUILD_RSA_REMOTE_SERVICE_ADMIN_DFI)
        add_celix_container(remote-services-dfi
            NAME "server"
            GROUP "remote-services/remote-services-dfi"
            BUNDLES Celix::rsa_discovery_etcd Celix::rsa_topology_manager Celix::rsa_dfi calculator Celix::shell Celix::shell_tui Celix::log_service Celix::log_writer_stdout
        )

        add_celix_container("remote-services-dfi-client"
            NAME "client"
            GROUP "remote-services/remote-services-dfi"
            BUNDLES Celix::rsa_topology_manager Celix::rsa_dfi Celix::shell Celix::shell_tui Celix::log_service Celix::log_writer_stdout calculator_shell Celix::rsa_discovery_etcd
        )
    endif ()
endif (RSA_EXAMPLES)


