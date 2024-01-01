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

add_library(string_hash_map_ei STATIC src/celix_string_hash_map_ei.cc)

target_include_directories(string_hash_map_ei PUBLIC include)
target_link_libraries(string_hash_map_ei PUBLIC Celix::error_injector Celix::utils)

target_link_options(string_hash_map_ei INTERFACE
        LINKER:--wrap,celix_stringHashMap_create
        LINKER:--wrap,celix_stringHashMap_createWithOptions
        LINKER:--wrap,celix_stringHashMap_put
        LINKER:--wrap,celix_stringHashMap_putLong
        LINKER:--wrap,celix_stringHashMap_putDouble
        LINKER:--wrap,celix_stringHashMap_putBool
        )
add_library(Celix::string_hash_map_ei ALIAS string_hash_map_ei)
