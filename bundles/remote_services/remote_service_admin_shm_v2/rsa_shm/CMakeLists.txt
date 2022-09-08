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

find_package(libuuid REQUIRED)

add_celix_bundle(rsa_shm
    VERSION 1.0.0
    SYMBOLIC_NAME "apache_celix_remote_service_admin_shm_v2"
    NAME "Apache Celix Remote Service Admin SHM V2"
    GROUP "Celix/RSA"
    SOURCES
    src/rsa_shm_impl.c
    src/rsa_shm_activator.c
    src/rsa_shm_server.c
    src/rsa_shm_client.c
    src/rsa_shm_export_registration.c
    src/rsa_shm_import_registration.c
)

target_include_directories(rsa_shm PRIVATE src)


target_link_libraries(rsa_shm PRIVATE
    Celix::c_rsa_spi
    Celix::rsa_common
    Celix::log_helper
    Celix::framework
    Celix::thpool
    Celix::shm_pool
    libuuid::libuuid
)

install_celix_bundle(rsa_shm EXPORT celix COMPONENT rsa)
add_library(Celix::rsa_shm ALIAS rsa_shm)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif()

