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

add_library(dm_component_ei STATIC src/celix_dm_component_ei.cc)

target_include_directories(dm_component_ei PUBLIC include)
target_link_libraries(dm_component_ei PUBLIC Celix::error_injector)
target_link_libraries(dm_component_ei PRIVATE Celix::framework)

target_link_options(dm_component_ei INTERFACE
        LINKER:--wrap,celix_dmComponent_create
        LINKER:--wrap,celix_dmServiceDependency_create
        )
add_library(Celix::dm_component_ei ALIAS dm_component_ei)
