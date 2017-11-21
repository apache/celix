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

add_bundle(remote_service_admin_dfi 
    VERSION 0.9.0
    SYMBOLIC_NAME "apache_celix_remote_service_admin_dfi"
    NAME "Apache Celix Remote Service Admin Dynamic Function Interface (DFI)"
    SOURCES
        src/remote_service_admin_dfi.c
        src/remote_service_admin_activator.c
        src/export_registration_dfi.c
        src/import_registration_dfi.c
        src/dfi_utils.c
)
target_include_directories(remote_service_admin_dfi PRIVATE src)
target_link_libraries(remote_service_admin_dfi PRIVATE
        Celix::dfi Celix::log_helper remote_service_admin_common
        ${CURL_LIBRARIES} ${JANSSON_LIBRARIES})

install_bundle(remote_service_admin_dfi)
