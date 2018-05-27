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

celix_subproject(REMOTE_SERVICE_ADMIN "Option to enable building the Remote Service Admin Service bundles" OFF)
if (REMOTE_SERVICE_ADMIN)
    add_subdirectory(remote_services_api)
    add_subdirectory(examples)

    add_subdirectory(rsa_spi)
    add_subdirectory(rsa_common)
    add_subdirectory(civetweb)

    add_subdirectory(topology_manager)

    add_subdirectory(discovery_common)
    add_subdirectory(discovery_configured)
    add_subdirectory(discovery_etcd)
    add_subdirectory(discovery_shm)

    #TODO refactor shm rsa to use dfi
    #add_subdirectory(remote_service_admin_shm)
    add_subdirectory(remote_service_admin_dfi)

endif (REMOTE_SERVICE_ADMIN)


