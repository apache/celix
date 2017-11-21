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

add_bundle(rsa_dfi_tst_bundle
    VERSION 0.0.1
    SOURCES
        tst_activator.c
)

bundle_files(rsa_dfi_tst_bundle
    ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_api/include/org.apache.celix.calc.api.Calculator2.descriptor
    DESTINATION .
)
target_include_directories(rsa_dfi_tst_bundle PRIVATE calculator)
target_link_libraries(rsa_dfi_tst_bundle PRIVATE ${CPPUTEST_LIBRARY} calculator_api)
