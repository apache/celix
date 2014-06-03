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

include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/event_admin/event_admin/public/include")
include_directories(public/include)

add_library(event_admin_utils STATIC private/src/event_impl.c)
target_link_libraries(event_admin_utils ${APRUTIL_LIBRARY})
#install(TARGETS event_admin_utils DESTINATION lib COMPONENT framework)
#FILE(GLOB files "public/include/*.h")
#INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
