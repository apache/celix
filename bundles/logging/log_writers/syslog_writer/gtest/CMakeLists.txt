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


add_executable(test_syslog_writer
        src/SyslogWriterTestSuite.cc
)
target_link_libraries(test_syslog_writer PRIVATE Celix::log_helper GTest::gtest GTest::gtest_main)
add_celix_bundle_dependencies(test_syslog_writer Celix::log_admin Celix::syslog_writer)
target_compile_definitions(test_syslog_writer PRIVATE -DLOG_ADMIN_BUNDLE=\"$<TARGET_PROPERTY:log_admin,BUNDLE_FILE>\")
target_compile_definitions(test_syslog_writer PRIVATE -DSYSLOG_WRITER_BUNDLE=\"$<TARGET_PROPERTY:syslog_writer,BUNDLE_FILE>\")


add_test(NAME test_syslog_writer COMMAND test_syslog_writer)
setup_target_for_coverage(test_syslog_writer SCAN_DIR ..)

