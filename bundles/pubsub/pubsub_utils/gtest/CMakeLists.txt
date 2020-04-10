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

add_executable(test_pubsub_utils
		src/PubSubMsgSerializationTestSuite.cc
)
target_link_libraries(test_pubsub_utils PRIVATE Celix::framework Celix::pubsub_utils GTest::GTest GTest::Main)
target_compile_options(test_pubsub_utils PRIVATE -std=c++14) #Note test code is allowed to be C++14
add_test(NAME test_pubsub_utils COMMAND test_pubsub_utils)
SETUP_TARGET_FOR_COVERAGE(test_pubsub_utils_cov test_pubsub_utils ${CMAKE_BINARY_DIR}/coverage/pubsub/pubsub_utils ..)

