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

set(RSA_COMMON_SRC
        src/endpoint_description.c
        src/export_registration_impl.c
        src/import_registration_impl.c
        src/remote_interceptors_handler.c
        )

add_library(rsa_common STATIC ${RSA_COMMON_SRC})
set_target_properties(rsa_common PROPERTIES OUTPUT_NAME "celix_rsa_common")
target_include_directories(rsa_common PRIVATE src)
target_link_libraries(rsa_common PUBLIC Celix::framework Celix::c_rsa_spi Celix::log_helper)
celix_deprecated_utils_headers(rsa_common)
celix_deprecated_framework_headers(rsa_common)
celix_target_hide_symbols(rsa_common)

install(TARGETS rsa_common EXPORT celix COMPONENT rsa DESTINATION ${CMAKE_INSTALL_LIBDIR})

#Setup target aliases to match external usage
add_library(Celix::rsa_common ALIAS rsa_common)

if (ENABLE_TESTING)
    add_library(rsa_common_cut STATIC ${RSA_COMMON_SRC})
    target_include_directories(rsa_common_cut PUBLIC src)
    target_link_libraries(rsa_common_cut PUBLIC Celix::framework Celix::c_rsa_spi Celix::log_helper)
    celix_deprecated_utils_headers(rsa_common_cut)
    celix_deprecated_framework_headers(rsa_common_cut)
    add_subdirectory(gtest)
endif()