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

#Importing and exporting libraries not (yet) work under OSX.

add_celix_bundle(log_service_example
    VERSION "1.0"
    SOURCES
        src/activator.c
)
target_include_directories(log_service_example PRIVATE src)
target_link_libraries(log_service_example PRIVATE Celix::log_helper)

add_celix_container(log_example
    GROUP c_examples
    BUNDLES log_service_example Celix::log_admin Celix::shell Celix::shell_tui
)
