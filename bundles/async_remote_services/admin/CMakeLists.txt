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

add_celix_bundle(async_rsa
        VERSION 0.9.0
        SYMBOLIC_NAME "apache_celix_async_remote_service_admin"
        NAME "Apache Celix Async Remote Service Admin"
        GROUP "Celix/AsyncRSA"
        SOURCES
        src/admin.cc
        )
target_include_directories(async_rsa PRIVATE include)
target_link_libraries(async_rsa PRIVATE
        Celix::async_rsa_common
        Celix::async_rsa_discovery_configured
        Celix::pubsub_api
        Celix::pubsub_spi
        Celix::framework
        )

#if (ENABLE_TESTING)
#    add_subdirectory(gtest)
#endif()


install_celix_bundle(async_rsa EXPORT celix COMPONENT async_rsa)
#Setup target aliases to match external usage
add_library(Celix::async_rsa ALIAS async_rsa)