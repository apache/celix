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

add_library(services_example_cxx_api INTERFACE)
target_include_directories(services_example_cxx_api INTERFACE api)

add_subdirectory(bar)
add_subdirectory(foo)
add_subdirectory(baz)

#TODO,fixme this is example is currently broken
message("services_example_cxx is currently broken!")
add_celix_container(services_example_cxx
    GROUP fixme
    #GROUP cxx_examples
    COPY
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        bar_cxx
        foo_cxx
        baz_cxx
    PROPERTIES
        example=value
)

