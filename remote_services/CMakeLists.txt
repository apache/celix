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

#add_subdirectory(mongoose)
add_subdirectory(topology_manager)
add_subdirectory(remote_service_admin)
add_subdirectory(discovery)

add_subdirectory(example_service)
add_subdirectory(example_endpoint)
add_subdirectory(example_proxy)
add_subdirectory(calc_shell)

#deploy("remote-services" BUNDLES discovery topology_manager remote_service_admin example shell shell_tui log_service log_writer)
#deploy("remote-services-client" BUNDLES discovery topology_manager remote_service_admin shell shell_tui log_service log_writer calc_shell)
