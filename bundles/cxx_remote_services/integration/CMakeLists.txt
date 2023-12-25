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

celix_subproject(CXX_RSA_INTEGRATION "Option to build the C++17 Remote Service Admin Service integration" ON)
if (CXX_RSA_INTEGRATION)
    add_celix_bundle(TestExportImportRemoteServiceFactory
            SOURCES src/TestExportImportRemoteServiceFactory.cc
            )
    target_link_libraries(TestExportImportRemoteServiceFactory PRIVATE Celix::rsa_spi Celix::Promises Celix::PushStreams Celix::log_helper)
    target_include_directories(TestExportImportRemoteServiceFactory PRIVATE include)

    add_celix_bundle(CalculatorProvider
            SOURCES src/CalculatorProvider.cc
            )
    target_link_libraries(CalculatorProvider PRIVATE Celix::Promises Celix::PushStreams)
    target_include_directories(CalculatorProvider PRIVATE include)

    add_celix_bundle(CalculatorConsumer
            SOURCES src/CalculatorConsumer.cc
            )
    celix_bundle_files(CalculatorConsumer ${CMAKE_CURRENT_SOURCE_DIR}/resources/endpoint_discovery.json DESTINATION "META-INF/discovery") # add configured discovery as resource in the bundle.
    target_link_libraries(CalculatorConsumer PRIVATE Celix::Promises Celix::PushStreams Celix::shell_api Celix::RsaConfiguredDiscovery_api)
    target_include_directories(CalculatorConsumer PRIVATE include)

    if (ENABLE_TESTING)
        add_subdirectory(gtest)
    endif()

    ################# Integration examples ##################################
    add_celix_container(RemoteCalculatorProvider
        CXX
        GROUP rsa
        PROPERTIES
            CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=debug
        BUNDLES
            Celix::ShellCxx
            Celix::shell_tui

            #Remote Services
            Celix::RemoteServiceAdmin
            TestExportImportRemoteServiceFactory #needed to be able to create a ExportedService for ICalculator

            CalculatorProvider
    )

    add_celix_container(RemoteCalculatorConsumer
        CXX
        GROUP rsa
        PROPERTIES
            CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=debug
        BUNDLES
            Celix::ShellCxx
            Celix::shell_tui

            #Remote Services
            Celix::RsaConfiguredDiscovery
            Celix::RemoteServiceAdmin
            TestExportImportRemoteServiceFactory #needed to be able to create a ExportedService for ICalculator

            CalculatorConsumer
    )
endif()
