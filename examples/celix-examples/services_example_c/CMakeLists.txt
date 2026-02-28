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

add_library(services_example_c_api INTERFACE)
target_include_directories(services_example_c_api INTERFACE api)


add_celix_bundle(provider_example
        VERSION 1.0.0
        SOURCES src/simple_provider_example.c
)
target_link_libraries(provider_example PRIVATE services_example_c_api)

add_celix_bundle(consumer_example
        VERSION 1.0.0
        SOURCES src/simple_consumer_example.c
        )
target_link_libraries(consumer_example PRIVATE services_example_c_api)



add_celix_bundle(dynamic_provider_example
        VERSION 1.0.0
        SOURCES src/dynamic_provider_example.c
)
target_link_libraries(dynamic_provider_example PRIVATE services_example_c_api)

add_celix_bundle(dynamic_consumer_example
        VERSION 1.0.0
        SOURCES src/dynamic_consumer_example.c
)
target_link_libraries(dynamic_consumer_example PRIVATE services_example_c_api)

add_celix_container(services_example_c
    GROUP c_examples
    BUNDLES
        Celix::shell
        Celix::shell_tui
        provider_example
        consumer_example
)

add_celix_container(dynamic_services_example_c
    GROUP c_examples
    BUNDLES
      Celix::shell
      Celix::shell_tui
      dynamic_provider_example
      dynamic_consumer_example
)
