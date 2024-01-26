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

find_package(jansson REQUIRED)

add_library(jansson_ei STATIC src/jansson_ei.cc)

target_include_directories(jansson_ei PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(jansson_ei
        PUBLIC Celix::error_injector jansson::jansson
        PRIVATE Celix::utils #for celix_cleanup.h
)

target_link_options(jansson_ei INTERFACE
        LINKER:--wrap,json_array_size
        LINKER:--wrap,json_dumps
        LINKER:--wrap,json_object
        LINKER:--wrap,json_object_set_new
        LINKER:--wrap,json_array
        LINKER:--wrap,json_array_append_new
        LINKER:--wrap,json_integer
        LINKER:--wrap,json_string
        )
add_library(Celix::jansson_ei ALIAS jansson_ei)
