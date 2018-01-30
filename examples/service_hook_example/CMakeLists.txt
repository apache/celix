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

if(NOT APPLE)
#Importing and exporting libraries not (yet) work under OSX.

include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("public/include")

add_celix_bundle(hook_example
           BUNDLE_SYMBOLICNAME "Hook_Example"
           VERSION "1.0.0"
           SOURCES
                private/src/activator
)

add_celix_container("hook_service_example"
            BUNDLES
                shell
                shell_tui
                hook_example
)
endif()
