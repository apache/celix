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

find_package(libuv REQUIRED)

if (NOT TARGET libuv::uv AND TARGET uv)
    #Note: conan libuv package 1.49.2 defines uv target, but 1.51.0 defines lubuv::uv target
    add_library(libuv::uv ALIAS uv)
endif ()

if (ENABLE_TESTING)
    add_executable(libuv_smoke_test
        src/libuv_smoke_test.cc
    )
    target_link_libraries(libuv_smoke_test PRIVATE libuv::uv GTest::gtest GTest::gtest_main)
    add_test(NAME libuv_smoke_test COMMAND libuv_smoke_test)
endif ()
