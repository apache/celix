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

add_library(ServiceExamplesApi INTERFACE)
target_include_directories(ServiceExamplesApi INTERFACE api)

add_celix_bundle(SimpleProvider SOURCES src/SimpleProviderBundleActivator.cc)
target_link_libraries(SimpleProvider PRIVATE ServiceExamplesApi)

add_celix_bundle(SimpleConsumer SOURCES src/SimpleConsumerBundleActivator.cc)
target_link_libraries(SimpleConsumer PRIVATE ServiceExamplesApi)

add_celix_container(SimpleServicesExample CXX
        GROUP cxx_examples
        BUNDLES
            Celix::ShellCxx
            Celix::shell_tui
            SimpleProvider
            SimpleConsumer
)


add_celix_bundle(DynamicConsumer SOURCES src/DynamicConsumerBundleActivator.cc)
target_link_libraries(DynamicConsumer PRIVATE Celix::shell_api ServiceExamplesApi)

add_celix_bundle(DynamicProvider SOURCES src/DynamicProviderBundleActivator.cc)
target_link_libraries(DynamicProvider PRIVATE Celix::shell_api ServiceExamplesApi)

add_celix_container(DynamicServicesExample CXX
        GROUP cxx_examples
        BUNDLES
            Celix::ShellCxx
            Celix::shell_tui
            DynamicProvider
            DynamicConsumer
)