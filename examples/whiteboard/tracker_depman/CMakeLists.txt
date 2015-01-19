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

SET(BUNDLE_SYMBOLICNAME "apache_celix_examples_tracker_dm")
SET(BUNDLE_VERSION "0.0.1")

bundle(tracker_depman SOURCES
    private/src/dependency_activator
    private/src/tracker
    
    private/include/tracker.h
)
include_directories("private/include")
include_directories("${PROJECT_SOURCE_DIR}/dependency_manager_2/public/include")
#include_directories("${PROJECT_SOURCE_DIR}/dependency_manager_2/private/include")
include_directories("../publisherService/public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")

# Use some magic to include all symbols of the static library
IF(APPLE)
target_link_libraries(tracker_depman celix_framework -Wl,-all_load dependency_manager2)
else()
target_link_libraries(tracker_depman -Wl,--whole-archive dependency_manager2 -Wl,--no-whole-archive celix_framework)
ENDIF()
