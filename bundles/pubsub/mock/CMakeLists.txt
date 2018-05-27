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

#only install if CppuTest is available
find_package(CppUTest QUIET)
if (CPPUTEST_FOUND)

    add_library(pubsub_mock STATIC
            src/publisher_mock.cc
    )
    target_include_directories(pubsub_mock PUBLIC api)
    target_link_libraries(pubsub_mock PRIVATE Celix::pubsub_spi ${CPPUTEST_LIBRARY})
    target_include_directories(pubsub_mock SYSTEM PRIVATE 
        ${CPPUTEST_INCLUDE_DIR}
    )

    if (ENABLE_TESTING)
        add_executable(pubsubmock_test
            tst/pubsubmock_test.cc
            tst/run_tests.cc
        )
        target_include_directories(pubsubmock_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/api)
	target_link_libraries(pubsubmock_test PRIVATE Celix::pubsub_api pubsub_mock ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} Celix::framework)
        add_test(NAME pubsubmock_test COMMAND pubsubmock_test)
    endif()
endif()

