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

find_package(APR REQUIRED)
find_package(CELIX REQUIRED)

include_directories(private/include)
include_directories(${PROJECT_SOURCE_DIR}/event_admin/event_admin/public/include)
include_directories("${CELIX_INCLUDE_DIRS}")

SET_HEADER(BUNDLE_SYMBOLICNAME "event_publisher")
SET_HEADERS("Bundle-Name: Event Publisher")


bundle(event_publisher SOURCES 
	private/src/event_publisher_activator.c
	private/src/event_publisher_impl.c
)

install_bundle(event_publisher)

target_link_libraries(event_publisher celix_framework celix_utils ${APR_LIBRARY} ${APRUTIL_LIBRARY})
