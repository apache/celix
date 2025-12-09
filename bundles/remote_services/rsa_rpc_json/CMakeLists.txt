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

celix_subproject(RSA_JSON_RPC "Option to enable building the Remote Service Admin JSON RPC bundle" ON)
if (RSA_JSON_RPC)

    find_package(jansson REQUIRED)

    set(RSA_JSON_RPC_SRC
            src/rsa_json_rpc_activator.c
            src/rsa_json_rpc_endpoint_impl.c
            src/rsa_json_rpc_impl.c
            src/rsa_json_rpc_proxy_impl.c
            )

    set(RSA_JSON_RPC_DEPS
            Celix::rsa_common
            Celix::rsa_dfi_utils
            Celix::c_rsa_spi
            Celix::dfi
            Celix::log_helper
            Celix::framework
            Celix::utils
            Celix::rsa_utils
            jansson::jansson
            )

    add_celix_bundle(rsa_json_rpc
        VERSION 2.0.0
        SYMBOLIC_NAME "apache_celix_rsa_json_rpc"
        NAME "Apache Celix Remote Service Admin JSON RPC"
        GROUP "Celix/RSA"
        FILENAME celix_rsa_json_rpc
        SOURCES
        ${RSA_JSON_RPC_SRC}
    )

    celix_deprecated_utils_headers(rsa_json_rpc)
    celix_deprecated_framework_headers(rsa_json_rpc)
    target_include_directories(rsa_json_rpc PRIVATE src)

    target_link_libraries(rsa_json_rpc PRIVATE ${RSA_JSON_RPC_DEPS})

    install_celix_bundle(rsa_json_rpc EXPORT celix COMPONENT rsa)
    add_library(Celix::rsa_json_rpc ALIAS rsa_json_rpc)

    if (ENABLE_TESTING)
        add_library(rsa_json_rpc_cut STATIC ${RSA_JSON_RPC_SRC})
        celix_deprecated_utils_headers(rsa_json_rpc_cut)
        target_include_directories(rsa_json_rpc_cut PUBLIC src)
        target_link_libraries(rsa_json_rpc_cut PUBLIC ${RSA_JSON_RPC_DEPS})
        add_subdirectory(gtest)
    endif()

endif()
