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

add_subdirectory(api)

add_subdirectory(phase1)
add_subdirectory(phase2)
add_subdirectory(phase3)
add_subdirectory(phase3_locking)

add_celix_container(dm_example_cxx
    FAT
    GROUP cxx_examples
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui

        dm_example_cxx_phase1
        dm_example_cxx_phase2a
        dm_example_cxx_phase2b
        dm_example_cxx_phase3
        dm_example_cxx_phase3_locking
)


