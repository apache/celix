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

add_library(dfi_ei STATIC src/dfi_ei.cc)

target_include_directories(dfi_ei PUBLIC include)
target_link_libraries(dfi_ei PUBLIC Celix::error_injector Celix::dfi)
# It plays nicely with address sanitizer this way.
target_link_options(dfi_ei INTERFACE
        LINKER:--wrap,dynFunction_createClosure
        LINKER:--wrap,jsonRpc_prepareInvokeRequest
        LINKER:--wrap,jsonRpc_call
)
add_library(Celix::dfi_ei ALIAS dfi_ei)
