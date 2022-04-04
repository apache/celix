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

if (BUILD_ZMQ_SECURITY)

    find_package(ZeroMQ REQUIRED)
    find_package(czmq REQUIRED)
    find_package(OpenSSL 1.1.0 REQUIRED)

    add_executable(makecert makecert.c)
    target_link_libraries(makecert czmq::czmq)
    
    add_executable(ed_file ed_file.c)
    target_link_libraries(ed_file PUBLIC ZeroMQ::ZeroMQ czmq::czmq OpenSSL::SSL)

endif()
