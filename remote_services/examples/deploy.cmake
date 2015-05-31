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
is_enabled(RSA_EXAMPLES)
if (RSA_EXAMPLES)
    is_enabled(RSA_DISCOVERY_ETCD)
    if (RSA_DISCOVERY_ETCD)
        deploy("remote-services-etcd" BUNDLES discovery_etcd topology_manager remote_service_admin_http calculator shell shell_tui log_service log_writer)
        deploy("remote-services-etcd-client" BUNDLES topology_manager remote_service_admin_http shell shell_tui log_service log_writer calculator_shell discovery_etcd)
    endif ()

endif (RSA_EXAMPLES)
