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

add_celix_bundle(calculator_shell
    SOURCES
        src/add_command
        src/sub_command
        src/sqrt_command
        src/calculator_shell_activator
    VERSION 0.0.1
    SYMBOLIC_NAME "apache_celix_remoting_calculator_shell"
)
target_include_directories(calculator_shell PRIVATE src)
target_link_libraries(calculator_shell PRIVATE Celix::shell_api calculator_api)

celix_bundle_files(calculator_shell
    ../calculator_api/org.apache.celix.calc.api.Calculator.avpr
    #src/org.apache.celix.calc.api.Calculator2.descriptor ##Use this descriptor in case you want to try out versioning!
    DESTINATION .
)
