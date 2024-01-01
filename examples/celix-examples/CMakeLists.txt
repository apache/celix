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

if (COMMAND celix_subproject)
    celix_subproject(EXAMPLES "Option to enable building the Examples" ON)
else ()
    set(EXAMPLES true) #celix_subproject is only available in the celix project -> using examples dir in other project is also supported
    option(CELIX_CXX14 "Build C++14 libraries and bundles." ON)
    option(CELIX_CXX17 "Build C++17 libraries and bundles." ON)
endif ()
if (EXAMPLES)
    add_definitions(-DADD_CELIX_DEPRECATED_WARNING) #ensure that no deprecated api is used in the examples

    add_subdirectory(hello_world_c)
    add_subdirectory(dm_example)
    add_subdirectory(services_example_c)
    add_subdirectory(readme_c_examples)

    if (CELIX_CXX14)
        add_subdirectory(hello_world_cxx)
        add_subdirectory(dm_example_cxx)
        add_subdirectory(services_example_cxx)
        add_subdirectory(dependency_manager_example_cxx)
    endif ()
    if (CELIX_CXX14)
        add_subdirectory(readme_cxx_examples)
    endif ()

    add_subdirectory(http_example)
    add_subdirectory(track_tracker_example)
    add_subdirectory(log_service_example)
    add_subdirectory(bundle_with_private_lib)
    add_subdirectory(bundles_copy_example)

endif(EXAMPLES)
