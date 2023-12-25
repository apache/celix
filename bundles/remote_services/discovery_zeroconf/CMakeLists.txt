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

celix_subproject(RSA_DISCOVERY_ZEROCONF "Option to enable building the zeroconf service discovery bundles" OFF)
if (RSA_DISCOVERY_ZEROCONF)

    find_package(DNSSD REQUIRED)

    set(RSA_DISCOVERY_ZEROCONF_SRC
            src/discovery_zeroconf_activator.c
            src/discovery_zeroconf_announcer.c
            src/discovery_zeroconf_watcher.c
            )

    set(RSA_DISCOVERY_ZEROCONF_DEPS
            DNSSD::DNSSD
            Celix::c_rsa_spi
            Celix::rsa_common
            Celix::log_helper
            Celix::framework
            Celix::utils
            )

    add_celix_bundle(rsa_discovery_zeroconf
        SYMBOLIC_NAME "apache_celix_rsa_discovery_zeroconf"
        VERSION "2.0.0"
        NAME "Apache Celix Multicast zeroconf service discovery"
        GROUP "Celix/RSA"
        FILENAME celix_rsa_discovery_zeroconf
        SOURCES
        ${RSA_DISCOVERY_ZEROCONF_SRC}
    )

    celix_deprecated_utils_headers(rsa_discovery_zeroconf)
    celix_deprecated_framework_headers(rsa_discovery_zeroconf)

    target_link_libraries(rsa_discovery_zeroconf PRIVATE ${RSA_DISCOVERY_ZEROCONF_DEPS})

    target_include_directories(rsa_discovery_zeroconf PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

    install_celix_bundle(rsa_discovery_zeroconf EXPORT celix COMPONENT rsa_discovery_zeroconf)

    #Setup target aliases to match external usage
    add_library(Celix::rsa_discovery_zeroconf ALIAS rsa_discovery_zeroconf)

    if (ENABLE_TESTING)
        add_library(rsa_discovery_zeroconf_cut STATIC ${RSA_DISCOVERY_ZEROCONF_SRC})
        celix_deprecated_utils_headers(rsa_discovery_zeroconf_cut)
        celix_deprecated_framework_headers(rsa_discovery_zeroconf_cut)
        target_include_directories(rsa_discovery_zeroconf_cut PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
        target_link_libraries(rsa_discovery_zeroconf_cut PUBLIC ${RSA_DISCOVERY_ZEROCONF_DEPS})
        add_subdirectory(gtest)
    endif(ENABLE_TESTING)

endif (RSA_DISCOVERY_ZEROCONF)
