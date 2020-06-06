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

set(SOURCES
        src/main.cc
        src/Utils_tests.cc
        src/Filter_tests.cc
        src/Registry_tests.cc
        src/ServiceTracking_tests.cc
        src/RegistryConcurrency_tests.cc
        src/ServiceRegistationBuilder_tests.cc
        src/ComponentManager_tests.cc
        src/FilterBuilder_tests.cc
        src/ServiceTrackerBuilder_tests.cc
        src/UseServiceBuilder_tests.cc

)
add_executable(celix_cxx_registry_tests ${SOURCES})
target_link_libraries(celix_cxx_registry_tests PRIVATE gtest celix::cxx::Registry spdlog::spdlog)

#NOTE last .. argument for the setup coverage is the scan dir (i.e. not gtest dir, but a dir higher)
add_test(NAME celix_cxx_registry_tests COMMAND celix_cxx_registry_tests)
SETUP_TARGET_FOR_COVERAGE(celix_cxx_registry_tests_cov celix_cxx_registry_tests ${CMAKE_BINARY_DIR}/coverage/celix/cxx/Registry ..)