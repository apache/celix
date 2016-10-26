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


celix_subproject(RSA_REMOTE_SERVICE_ADMIN_DFI "Option to enable building the Remote Service Admin Service DFI" ON DEPS TOPOLOGY_MANAGER)

if (RSA_REMOTE_SERVICE_ADMIN_DFI)

    find_package(CURL REQUIRED)
    find_package(Jansson REQUIRED)

    include_directories(
        ${CURL_INCLUDE_DIRS}
        ${JANSSON_INCLUDE_DIRS}
    )

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        include_directories(dynamic_function_interface/memstream)
    endif()

    add_subdirectory(rsa)

    if (ENABLE_TESTING)
        find_package(CppUTest REQUIRED)
        include_directories(${CPPUTEST_INCLUDE_DIR})
        add_subdirectory(rsa_tst)
    endif()

endif()
