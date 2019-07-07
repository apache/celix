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

#if (BUILD_PUBSUB_PSA_TCP)
	find_package(Jansson REQUIRED)
	find_package(UUID REQUIRED)
		
	add_celix_bundle(celix_pubsub_admin_tcp
		BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_tcp"
		VERSION "1.0.0"
		GROUP "Celix/PubSub"
		SOURCES
			src/psa_activator.c
			src/pubsub_tcp_admin.c
			src/pubsub_tcp_topic_sender.c
			src/pubsub_tcp_topic_receiver.c
                        src/pubsub_tcp_handler.c
			src/pubsub_tcp_common.c
	)

	set_target_properties(celix_pubsub_admin_tcp PROPERTIES INSTALL_RPATH "$ORIGIN")
	target_link_libraries(celix_pubsub_admin_tcp PRIVATE
			Celix::pubsub_spi
			Celix::framework Celix::dfi Celix::log_helper
	)
	target_include_directories(celix_pubsub_admin_tcp PRIVATE
		${JANSSON_INCLUDE_DIR}
		src
	)
	# cmake find package UUID set the wrong include dir for OSX
	if (NOT APPLE)
		target_include_directories(celix_pubsub_admin_tcp SYSTEM PRIVATE ${UUID_INCLUDE_DIRS})
	endif()

	install_celix_bundle(celix_pubsub_admin_tcp EXPORT celix COMPONENT pubsub)
	target_link_libraries(celix_pubsub_admin_tcp PRIVATE Celix::shell_api)
	add_library(Celix::pubsub_admin_tcp ALIAS celix_pubsub_admin_tcp)
#endif()
