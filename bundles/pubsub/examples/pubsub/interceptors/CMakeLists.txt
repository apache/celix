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

add_celix_bundle(celix_pubsub_interceptors_example
    SYMBOLIC_NAME "celix_pubsub_interceptors_example"
    VERSION "1.0.0"
    SOURCES 
        src/ps_interceptor_activator.c
        src/first_interceptor.c
        src/second_interceptor.c
)

target_link_libraries(celix_pubsub_interceptors_example PRIVATE Celix::framework Celix::pubsub_spi)
target_include_directories(celix_pubsub_interceptors_example PRIVATE include)