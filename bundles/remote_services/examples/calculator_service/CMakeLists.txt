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

add_celix_bundle(calculator
    SOURCES
        src/calculator_impl.c
        src/calculator_activator.c
    SYMBOLIC_NAME "apache_celix_remoting_calculator_impl"
    VERSION 0.0.1
)
target_include_directories(calculator PRIVATE src)
target_link_libraries(calculator PRIVATE Celix::c_rsa_spi calculator_api m)

get_target_property(DESCR calculator_api INTERFACE_DESCRIPTOR)
celix_bundle_files(calculator ${DESCR} DESTINATION .)
