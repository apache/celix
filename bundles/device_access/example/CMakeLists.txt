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

celix_subproject(DEVICE_ACCESS_EXAMPLE "Option to enable building the Device Access example bundles" OFF DEPS SHELL SHELL_TUI)
if(DEVICE_ACCESS_EXAMPLE)
	add_subdirectory(base_driver)
	add_subdirectory(consuming_driver)
	add_subdirectory(refining_driver)

    add_celix_container(device_access_example
        BUNDLES Celix::device_manager Celix::driver_locator Celix::shell Celix::shell_tui base_driver
    )

    celix_container_bundles_dir(device_access_example
            DIR_NAME "drivers"
            BUNDLES word_consumingdriver char_refiningdriver
    )
endif(DEVICE_ACCESS_EXAMPLE)
