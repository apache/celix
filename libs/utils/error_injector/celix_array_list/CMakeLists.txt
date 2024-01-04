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

add_library(array_list_ei STATIC src/celix_array_list_ei.cc)

target_include_directories(array_list_ei PUBLIC include)
target_link_libraries(array_list_ei PUBLIC Celix::error_injector Celix::utils)
target_link_options(array_list_ei INTERFACE
        LINKER:--wrap,celix_arrayList_create
        LINKER:--wrap,celix_arrayList_createWithOptions
        LINKER:--wrap,celix_arrayList_add
        LINKER:--wrap,celix_arrayList_addInt
        LINKER:--wrap,celix_arrayList_addLong
        LINKER:--wrap,celix_arrayList_addUInt
        LINKER:--wrap,celix_arrayList_addULong
        LINKER:--wrap,celix_arrayList_addFloat
        LINKER:--wrap,celix_arrayList_addDouble
        LINKER:--wrap,celix_arrayList_addBool
        LINKER:--wrap,celix_arrayList_addSize
        LINKER:--wrap,celix_arrayList_copy
)
add_library(Celix::array_list_ei ALIAS array_list_ei)
