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

add_executable(test_cxx_remote_services_integration
    src/RemoteServicesIntegrationTestSuite.cc
)
target_link_libraries(test_cxx_remote_services_integration PRIVATE Celix::framework Celix::Promises Celix::PushStreams Celix::shell_api GTest::gtest GTest::gtest_main)
target_include_directories(test_cxx_remote_services_integration PRIVATE ../include) #Add ICalculator

add_celix_bundle_dependencies(test_cxx_remote_services_integration
    Celix::RsaConfiguredDiscovery
    Celix::RemoteServiceAdmin
    TestExportImportRemoteServiceFactory
    CalculatorProvider
    CalculatorConsumer
)


celix_get_bundle_file(Celix::RsaConfiguredDiscovery RS_DISCOVERY_BUNDLE_LOC)
target_compile_definitions(test_cxx_remote_services_integration PRIVATE RS_DISCOVERY_BUNDLE_LOC="${RS_DISCOVERY_BUNDLE_LOC}")

celix_get_bundle_file(Celix::RemoteServiceAdmin RS_RSA_BUNDLE_LOC)
target_compile_definitions(test_cxx_remote_services_integration PRIVATE RS_RSA_BUNDLE_LOC="${RS_RSA_BUNDLE_LOC}")

celix_get_bundle_file(TestExportImportRemoteServiceFactory RS_FACTORY_BUNDLE_LOC)
target_compile_definitions(test_cxx_remote_services_integration PRIVATE RS_FACTORY_BUNDLE_LOC="${RS_FACTORY_BUNDLE_LOC}")

celix_get_bundle_file(CalculatorProvider RS_PROVIDER_BUNDLE_LOC)
target_compile_definitions(test_cxx_remote_services_integration PRIVATE RS_PROVIDER_BUNDLE_LOC="${RS_PROVIDER_BUNDLE_LOC}")

celix_get_bundle_file(CalculatorConsumer RS_CONSUMER_BUNDLE_LOC)
target_compile_definitions(test_cxx_remote_services_integration PRIVATE RS_CONSUMER_BUNDLE_LOC="${RS_CONSUMER_BUNDLE_LOC}")



add_test(NAME test_cxx_remote_services_integration COMMAND test_cxx_remote_services_integration)
setup_target_for_coverage(test_cxx_remote_services_integration SCAN_DIR ../..)