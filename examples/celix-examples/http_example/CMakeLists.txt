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
if (TARGET Celix::http_admin AND TARGET Celix::shell AND TARGET Celix::shell_wui)


    add_celix_bundle(http_example
        SYMBOLIC_NAME "apache_celix_http_example"
        VERSION "1.0.0"
        NAME "Apache Celix HTTP Example"
        SOURCES
            src/http_example_bundle_activator.c
    )

    target_link_libraries(http_example PRIVATE Celix::http_admin_api)
    celix_bundle_private_libs(http_example Celix::civetweb_shared)
    celix_bundle_add_dir(http_example resources)
    celix_bundle_headers(http_example "X-Web-Resource: /hello$<SEMICOLON>/resources")

    add_celix_container(http_example_cnt
            GROUP c_examples
            BUNDLES
                Celix::http_admin
                Celix::shell
                Celix::shell_wui #web ui (webpage to access Celix shell using static html + websocket)
                Celix::shell_tui #text ui (uses console at access Celix shell)
            #INSTALL_BUNDLES #Enabled if you want to manually start the http_example bundle using the shell: `start 5`
                http_example
    )

endif ()
