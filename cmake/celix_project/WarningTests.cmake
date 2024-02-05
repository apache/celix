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

#[[
Misc usage of Apache Celix CMake functions to check if they work as expected.
Including some constructions that will generate warnings.
]]

if (ENABLE_CMAKE_WARNING_TESTS AND TARGET Celix::shell AND TARGET Celix::shell_tui)
    add_celix_container(example_with_duplicate_bundles_1
            INSTALL_BUNDLES Celix::shell
            BUNDLES Celix::shell_tui #add bundles with run level 3
    )

    #Adding a bundle twice on the same run level is fine and the last entry should be ignored
    celix_container_bundles(example_with_duplicate_bundles_1 INSTALL Celix::shell)
    celix_container_bundles(example_with_duplicate_bundles_1 LEVEL 3 Celix::shell_tui)

    #Starting a bundle that is already installed is fine and should not result in a warning
    celix_container_bundles(example_with_duplicate_bundles_1 LEVEL 3 Celix::shell)

    #Adding a bundle twice on different run levels should result in an warning
    celix_container_bundles(example_with_duplicate_bundles_1 LEVEL 4 Celix::shell_tui)
    celix_container_bundles(example_with_duplicate_bundles_1 LEVEL 4 Celix::shell)
endif ()
