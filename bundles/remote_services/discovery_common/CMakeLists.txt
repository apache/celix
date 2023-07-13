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

celix_subproject(RSA_DISCOVERY_COMMON "Option to enable building the RSA discovery common library" ON)
if (RSA_DISCOVERY_COMMON)
    find_package(CURL REQUIRED)
    find_package(LibXml2 REQUIRED)
    find_package(civetweb REQUIRED)

    add_library(rsa_discovery_common OBJECT
            src/discovery.c
            src/discovery_activator.c
            src/endpoint_descriptor_reader.c
            src/endpoint_descriptor_writer.c
            src/endpoint_discovery_poller.c
            src/endpoint_discovery_server.c
            )
    target_include_directories(rsa_discovery_common PUBLIC include
            PRIVATE src ${LIBXML2_INCLUDE_DIR})
    celix_target_hide_symbols(rsa_discovery_common)
    celix_deprecated_utils_headers(rsa_discovery_common)
    celix_deprecated_framework_headers(rsa_discovery_common)
    target_link_libraries(rsa_discovery_common
            PUBLIC Celix::framework Celix::utils Celix::c_rsa_spi Celix::log_helper
            PRIVATE CURL::libcurl civetweb::civetweb ${LIBXML2_LIBRARIES})

    #Setup target aliases to match external usage
    add_library(Celix::rsa_discovery_common ALIAS rsa_discovery_common)
endif ()