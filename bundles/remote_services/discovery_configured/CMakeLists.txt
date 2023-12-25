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

celix_subproject(RSA_DISCOVERY_CONFIGURED "Option to enable building the Discovery (Configured) bundle" ON)
if (RSA_DISCOVERY_CONFIGURED)
    add_celix_bundle(rsa_discovery
            VERSION 0.9.0
            SYMBOLIC_NAME "apache_celix_rsa_discovery"
            NAME "Apache Celix RSA Configured Discovery"
            FILENAME celix_rsa_discovery_configured
            SOURCES
            src/discovery_impl.c
            )
    target_include_directories(rsa_discovery PRIVATE src )
    target_link_libraries(rsa_discovery PRIVATE Celix::rsa_discovery_common Celix::log_helper Celix::rsa_common)
    celix_deprecated_framework_headers(rsa_discovery)
    celix_deprecated_utils_headers(rsa_discovery)

    install_celix_bundle(rsa_discovery EXPORT celix COMPONENT rsa)
    #Setup target aliases to match external usage
    add_library(Celix::rsa_discovery ALIAS rsa_discovery)
endif (RSA_DISCOVERY_CONFIGURED)
