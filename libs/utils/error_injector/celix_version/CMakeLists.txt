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

add_library(version_ei STATIC src/celix_version_ei.cc)

target_include_directories(version_ei PUBLIC include)
target_link_libraries(version_ei PUBLIC Celix::error_injector Celix::utils)
# It plays nicely with address sanitizer this way.
target_link_options(version_ei INTERFACE
        LINKER:--wrap,celix_version_createVersionFromString
        LINKER:--wrap,celix_version_parse
        LINKER:--wrap,celix_version_copy
        LINKER:--wrap,celix_version_toString
)
add_library(Celix::version_ei ALIAS version_ei)
