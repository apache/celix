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

find_package(Jansson REQUIRED)

include_directories("private/include")
include_directories("public/include")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
include_directories("${PROJECT_SOURCE_DIR}/dfi/public/include")
include_directories("${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/include")
include_directories("${PROJECT_SOURCE_DIR}/pubsub/api/pubsub")
include_directories("${JANSSON_INCLUDE_DIR}")

add_bundle(org.apache.celix.pubsub_serializer.PubSubSerializerJson
    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_serializer_json"
    VERSION "1.0.0"
    SOURCES
    	private/src/ps_activator.c
    	private/src/pubsub_serializer_impl.c
	   ${PROJECT_SOURCE_DIR}/log_service/public/src/log_helper.c
    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_utils.c
)

set_target_properties(org.apache.celix.pubsub_serializer.PubSubSerializerJson PROPERTIES INSTALL_RPATH "$ORIGIN")
target_link_libraries(org.apache.celix.pubsub_serializer.PubSubSerializerJson celix_framework celix_utils celix_dfi ${JANSSON_LIBRARIES})

install_bundle(org.apache.celix.pubsub_serializer.PubSubSerializerJson)

