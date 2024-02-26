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

celix_subproject(RSA_REMOTE_SERVICE_ADMIN_DFI "Option to enable building the Remote Service Admin Service DFI" ON)
if (RSA_REMOTE_SERVICE_ADMIN_DFI)
    find_package(CURL REQUIRED)
    find_package(jansson REQUIRED)
    find_package(libuuid REQUIRED)
    find_package(civetweb REQUIRED)

    add_celix_bundle(rsa_dfi
            VERSION 0.9.0
            SYMBOLIC_NAME "apache_celix_remote_service_admin_dfi"
            NAME "Apache Celix Remote Service Admin Dynamic Function Interface (DFI)"
            GROUP "Celix/RSA"
            FILENAME celix_rsa_dfi
            SOURCES
            src/remote_service_admin_dfi.c
            src/remote_service_admin_activator.c
            src/export_registration_dfi.c
            src/import_registration_dfi.c
            )
    target_link_libraries(rsa_dfi PRIVATE
            Celix::rsa_dfi_utils
            Celix::dfi
            Celix::log_helper
            Celix::rsa_common
            CURL::libcurl
            civetweb::civetweb
            jansson::jansson
            libuuid::libuuid
            )
    celix_deprecated_utils_headers(rsa_dfi)
    celix_deprecated_framework_headers(rsa_dfi)

    install_celix_bundle(rsa_dfi EXPORT celix COMPONENT rsa)
    #Setup target aliases to match external usage
    add_library(Celix::rsa_dfi ALIAS rsa_dfi)

    if (ENABLE_TESTING)
        add_subdirectory(gtest)
    endif()
endif()
