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

add_celix_bundle(hello_world_cxx
    VERSION 1.0.0
    SOURCES src/BundleActivator.cc
)

set_target_properties(hello_world_cxx PROPERTIES LINK_FLAGS -Wl,--no-as-needed)
target_link_libraries(hello_world_cxx PRIVATE -ldl)

add_celix_container(hello_world_cxx_container
    BUNDLES
        Celix::shell
        Celix::shell_tui
        hello_world_cxx
        Jansson
)