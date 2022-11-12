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

add_celix_bundle(RemoteServiceAdmin
        VERSION 0.9.0
        SYMBOLIC_NAME "Apache_Celix_RemoteServiceAdmin"
        NAME "Apache Celix Remote Service Admin"
        GROUP "Celix/RSA"
        FILENAME "Celix_RemoteServiceAdmin"
        SOURCES
            src/RemoteServiceAdmin.cc
            src/RemoteServiceAdminActivator.cc
)
target_link_libraries(RemoteServiceAdmin PRIVATE
        Celix::rsa_spi
        Celix::framework
        Celix::log_helper
)

install_celix_bundle(RemoteServiceAdmin EXPORT celix COMPONENT rsa)
#Setup target aliases to match external usage
add_library(Celix::RemoteServiceAdmin ALIAS RemoteServiceAdmin)

if (ENABLE_TESTING)
    add_subdirectory(gtest)
endif()