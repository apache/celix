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

#api target
add_library(pubsub_api INTERFACE)

target_include_directories(pubsub_api INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

#install api
install(TARGETS pubsub_api EXPORT celix DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT pubsub
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/pubsub)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/pubsub COMPONENT pubsub)

#Setup target aliases to match external usage
add_library(Celix::pubsub_api ALIAS pubsub_api)

