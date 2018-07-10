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

add_library(civetweb_static STATIC
        civetweb/civetweb.c
)
target_include_directories(civetweb_static PUBLIC SYSTEM civetweb)
target_compile_options(civetweb_static PRIVATE -w -DUSE_WEBSOCKET)

add_celix_bundle(embedded_civetweb
    VERSION 1.0.0
    SOURCES src/bundle_activator.c
)
target_link_libraries(embedded_civetweb PRIVATE Celix::shell_api)
celix_bundle_private_libs(embedded_civetweb civetweb_static)
celix_bundle_add_dir(embedded_civetweb resources)

add_celix_container(civetweb_example
    BUNDLES
        Celix::shell
        Celix::shell_tui
        embedded_civetweb
)
