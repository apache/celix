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

bundle(client SOURCES private/src/client_activator private/src/client_impl)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("private/include")
include_directories("public/include")
include_directories("../server/public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
target_link_libraries(client framework)

include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories(${PROJECT_SOURCE_DIR}/cmake/cpputest/include)
link_directories(${PROJECT_SOURCE_DIR}/cmake/cpputest/lib)
add_executable(client_test private/test/client_test private/src/client_impl)
target_link_libraries(client_test CppUTest CppUTestExt framework)
run_cppu_test(client_test)