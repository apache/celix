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

add_celix_bundle(topology_manager_disc_mock_bundle
    VERSION 0.0.1
    SOURCES
        disc_mock_activator.c
        disc_mock_service.c
)
target_include_directories(topology_manager_disc_mock_bundle PRIVATE
        $<TARGET_PROPERTY:Celix::rsa_discovery_common,INCLUDE_DIRECTORIES>
        )
target_link_libraries(topology_manager_disc_mock_bundle PRIVATE Celix::framework)
celix_deprecated_framework_headers(topology_manager_disc_mock_bundle)
celix_deprecated_utils_headers(topology_manager_disc_mock_bundle)
