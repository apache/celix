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

#MyBundle example
#find_package(Celix REQUIRED) NOTE not usable inside Celix project

#With `make all`, `make celix-bundles` this bundle will be created at:
#  ${CMAKE_CURRENT_BINARY_DIR}/MyBundle.zip.
add_celix_bundle(MyBundle
    SOURCES src/MyBundleActivator.cc
)

#With `make all`, `make celix-containers` or `make MyContainer` this Celix container executable will be created at:
# ${CMAKE_BINARY_DIR}/deploy/my_container/MyContainer
add_celix_container(MyContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        MyBundle
)

#CalcProviderBundleActivator example
#find_package(Celix REQUIRED) NOTE not useable inside Celix project

add_celix_bundle(CalcProviderBundle
        SOURCES src/CalcProviderBundleActivator.cc
)
target_include_directories(CalcProviderBundle PRIVATE include)

add_celix_container(CalcProviderContainer
        BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        CalcProviderBundle
)

#CalcUserBundle example
#find_package(Celix REQUIRED) NOTE not useable inside Celix project

add_celix_bundle(CalcUserBundle
    SOURCES src/CalcUserBundleActivator.cc
)
target_include_directories(CalcUserBundle PRIVATE include)

add_celix_container(CalcUserContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        CalcProviderBundle
        CalcUserBundle
)

#CalcTrackerBundle
#find_package(Celix REQUIRED) NOTE not useable inside Celix project

add_celix_bundle(CalcTrackerBundle
    SOURCES src/CalcTrackerBundleActivator.cc
)
target_include_directories(CalcTrackerBundle PRIVATE include)

add_celix_container(CalcTrackerContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        CalcProviderBundle
        CalcTrackerBundle
)

#FilterExampleBundle
#find_package(Celix REQUIRED) NOTE not useable inside Celix project

add_celix_bundle(FilterExampleBundle
    SOURCES src/FilterExampleBundleActivator.cc
)
target_link_libraries(FilterExampleBundle PRIVATE Celix::shell_api) #adds celix/IShellCommand.h to the include path

add_celix_container(FilterExampleContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        FilterExampleBundle
)

add_celix_bundle(MyShellCommandProviderBundle
    VERSION 1.0.0
    SOURCES src/MyShellCommandProviderBundleActivator.cc
)
target_link_libraries(MyShellCommandProviderBundle PRIVATE Celix::shell_api)

add_celix_bundle(MyCShellCommandProviderBundle
    VERSION 1.0.0
    SOURCES src/MyCShellCommandProviderBundleActivator.cc
)
target_link_libraries(MyCShellCommandProviderBundle PRIVATE Celix::shell_api)

add_celix_bundle(UsingCommandServicesExampleBundle
    VERSION 1.0.0
    SOURCES
        src/UsingCommandServicesExample.cc
)
target_link_libraries(UsingCommandServicesExampleBundle PRIVATE Celix::shell_api)

add_celix_bundle(TrackingCommandServicesExampleBundle
    VERSION 1.0.0
    SOURCES
        src/TrackingCommandServicesExample.cc
)
target_link_libraries(TrackingCommandServicesExampleBundle PRIVATE Celix::shell_api)

add_celix_bundle(SimpleComponentBundle
    VERSION 1.0.0
    SOURCES src/SimpleComponentActivator.cc
)

add_celix_bundle(ComponentWithProvidedServiceBundle
    VERSION 1.0.0
    SOURCES src/ComponentWithProvidedServiceActivator.cc
)
target_link_libraries(ComponentWithProvidedServiceBundle PRIVATE Celix::shell_api)

add_celix_bundle(ComponentWithServiceDependencyBundle
    VERSION 1.0.0
    SOURCES src/ComponentWithServiceDependencyActivator.cc
)
target_link_libraries(ComponentWithServiceDependencyBundle PRIVATE Celix::shell_api)

add_celix_container(ReadmeCxxExamplesContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        MyShellCommandProviderBundle
        MyCShellCommandProviderBundle
        UsingCommandServicesExampleBundle
        TrackingCommandServicesExampleBundle
        SimpleComponentBundle
        ComponentWithProvidedServiceBundle
        ComponentWithServiceDependencyBundle
)

if (TARGET my_shell_command_provider_bundle)
    #Add C shell command provider and using LEVEL 2 so that is start before UsingCommandServicesExampleBundle
    celix_container_bundles(ReadmeCxxExamplesContainer LEVEL 2 my_shell_command_provider_bundle)
endif ()