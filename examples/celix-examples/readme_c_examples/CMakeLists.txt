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

#my_bundle example
#find_package(Celix REQUIRED) NOTE not usable inside Celix project

#With `make all`, `make celix-bundles` this bundle will be created at:
#  ${CMAKE_CURRENT_BINARY_DIR}/my_bundle.zip.
add_celix_bundle(my_bundle
    VERSION 1.0.0
    SOURCES src/my_bundle_activator.c
)

#With `make all`, `make celix-containers` or `make my_container` this Celix container executable will be created at:
# ${CMAKE_BINARY_DIR}/deploy/my_container/my_container
add_celix_container(my_container
    C
    BUNDLES
        Celix::shell
        Celix::shell_tui
        my_bundle
)

add_celix_bundle(my_shell_command_provider_bundle
        VERSION 1.0.0
        SOURCES src/my_shell_command_provider_bundle_activator.c
)
target_link_libraries(my_shell_command_provider_bundle PRIVATE Celix::shell_api)

add_celix_bundle(using_command_service_bundle
        VERSION 1.0.0
        SOURCES src/using_command_service_example.c
)
target_link_libraries(using_command_service_bundle PRIVATE Celix::shell_api)

add_celix_bundle(tracker_command_services_bundle
        VERSION 1.0.0
        SOURCES src/track_command_services_example.c
)
target_link_libraries(tracker_command_services_bundle PRIVATE Celix::shell_api)

add_celix_bundle(simple_component_bundle
        VERSION 1.0.0
        SOURCES src/simple_component_activator.c
)

add_celix_bundle(component_with_provided_service_bundle
        VERSION 1.0.0
        SOURCES src/component_with_provided_service_activator.c
)
target_link_libraries(component_with_provided_service_bundle PRIVATE Celix::shell_api)

add_celix_bundle(component_with_service_dependency_bundle
    VERSION 1.0.0
    SOURCES src/component_with_service_dependency_activator.c
)
target_link_libraries(component_with_service_dependency_bundle PRIVATE Celix::shell_api)

add_celix_container(readme_c_examples_container
    C
    BUNDLES
        Celix::shell
        Celix::shell_tui
        my_shell_command_provider_bundle
        using_command_service_bundle
        tracker_command_services_bundle
        simple_component_bundle
        component_with_provided_service_bundle
        component_with_service_dependency_bundle
)


add_celix_bundle(http_admin_extendee_bundle # <----------------------------------------------------------------------<1>
    VERSION 1.0.0
    NO_ACTIVATOR # <-------------------------------------------------------------------------------------------------<2>
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/index.html" "<html><body>Hello World</body></html>") # <---------------------<3>
celix_bundle_files(http_admin_extendee_bundle "${CMAKE_CURRENT_BINARY_DIR}/index.html" DESTINATION resources) # <----<4>
celix_bundle_headers(http_admin_extendee_bundle "X-Web-Resource: /hello$<SEMICOLON>/resources") # <------------------<5>

add_celix_container(extender_pattern_example_container # <-----------------------------------------------------------<6>
    BUNDLES
        Celix::http_admin
        http_admin_extendee_bundle
)