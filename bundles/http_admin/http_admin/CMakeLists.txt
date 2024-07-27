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

add_celix_bundle(http_admin
    SOURCES
        src/http_admin.c
        src/websocket_admin.c
        src/activator.c
        src/service_tree.c
    VERSION 1.0.0
    SYMBOLIC_NAME "apache_celix_http_admin"
    GROUP "Celix/HTTP_admin"
    NAME "Apache Celix HTTP Admin"
    FILENAME celix_http_admin
)
target_include_directories(http_admin PRIVATE src)

target_link_libraries(http_admin PUBLIC Celix::http_admin_api)

install_celix_bundle(http_admin EXPORT celix COMPONENT http_admin)
#Setup target aliases to match external usage
add_library(Celix::http_admin ALIAS http_admin)
