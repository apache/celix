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
celix_subproject(SHELL_WUI "Option to enable building the Shell Web User Interface bundles" ON DEPS SHELL HTTP_ADMIN)
if (SHELL_WUI)

    add_celix_bundle(shell_wui
        SYMBOLIC_NAME "apache_celix_shell_wui"
        VERSION "1.0.0"
        NAME "Apache Celix Shell WUI"
        FILENAME celix_shell_wui
        GROUP "Celix/Shell"
        SOURCES
            src/shell_wui_bundle_activator.c
    )

    target_link_libraries(shell_wui PRIVATE Celix::shell_api Celix::http_admin_api)
    celix_bundle_private_libs(shell_wui civetweb_shared)
    celix_bundle_add_dir(shell_wui resources)
    celix_bundle_headers(shell_wui "X-Web-Resource: /shell$<SEMICOLON>/resources")


    install_celix_bundle(shell_wui EXPORT celix)
    #Alias setup to match external usage
    add_library(Celix::shell_wui ALIAS shell_wui)

endif ()