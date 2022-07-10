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


add_executable(test_shm_pool
        src/ShmPoolTestSuite.cc
        src/ShmCacheTestSuite.cc
)

target_link_libraries(test_shm_pool PRIVATE Celix::shm_pool GTest::gtest GTest::gtest_main)

add_test(NAME run_test_shm_pool COMMAND test_shm_pool)
setup_target_for_coverage(test_shm_pool SCAN_DIR ..)

