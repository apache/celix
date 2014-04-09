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

include_directories(public/include)
include_directories(private/include)
include_directories("${CELIX_INCLUDE_DIRS}")
#include_directories(../utils/public/include)

SET_HEADER(BUNDLE_SYMBOLICNAME "eventadmin")
SET_HEADERS("Bundle-Name: Event admin")


bundle(event_admin 
	SOURCES 
		private/src/event_admin_activator.c
		private/src/event_admin_impl.c
		private/src/event_impl.c
		public/include/event_admin.h
		public/include/event_handler.h
		private/include/event_admin_impl.h
)

install_bundle(event_admin
	HEADERS
	    public/include/event_admin.h
	    public/include/event_constants.h
	    public/include/event_handler.h
)

target_link_libraries(event_admin celix_framework celix_utils ${APR_LIBRARY} ${APRUTIL_LIBRARY})
