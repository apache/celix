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
#find_package(Celix REQUIRED) NOTE not useable inside Celix project

add_celix_bundle(MyBundle
    SOURCES src/MyBundleActivator.cc
)

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