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


include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/framework/public/include")
include_directories("${PROJECT_SOURCE_DIR}/config_admin/service/public/include")
include_directories("private/include")

SET(BUNDLE_SYMBOLICNAME "bundle_managed_service")
SET(BUNDLE_VERSION "0.1.0")

bundle(bundle_managed_service SOURCES 
	private/src/activator
	private/src/example_managed_service_impl
)

target_link_libraries(bundle_managed_service celix_framework celix_utils config_admin ${APR_LIBRARY})
