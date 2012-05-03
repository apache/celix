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
INCLUDE(FindPkgConfig)
pkg_search_module (GLIB  REQUIRED glib-2.0)
pkg_search_module (GTHR  REQUIRED gthread-2.0)
pkg_search_module (GTK   REQUIRED gtk+-2.0)
include_directories(
	private/include
	public/include
	../simple/public/include
)
include_directories(${GTK_INCLUDE_DIRS})
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${GTHR_INCLUDE_DIRS})

link_directories(${GTK_LIBRARY_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
link_directories(${GTHR_LIBRARY_DIRS})

bundle(square SOURCES 
 	private/src/activator
 	private/src/square_shape
 FILES 
 	private/src/square.png
)
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
target_link_libraries(square framework ${GLIB_LIBRARIES} ${GTK_LIBRARIES} ${GTHR_LIBRARIES})
