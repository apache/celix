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

if (BUILD_PUBSUB_PSA_ZMQ)

	find_package(ZMQ REQUIRED)
	find_package(CZMQ REQUIRED)
	find_package(Jansson REQUIRED)

	if (BUILD_ZMQ_SECURITY)
		add_definitions(-DBUILD_WITH_ZMQ_SECURITY=1)

		find_package(OpenSSL 1.1.0 REQUIRED)
		include_directories("${OPENSSL_INCLUDE_DIR}")

		set (ZMQ_CRYPTO_C "private/src/zmq_crypto.c")
	endif()

	add_celix_bundle(celix_pubsub_admin_zmq
			BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_zmq"
			VERSION "1.0.0"
			SOURCES
			src/psa_activator.c
			src/pubsub_admin_impl.c
			src/topic_subscription.c
			src/topic_publication.c
			${ZMQ_CRYPTO_C}
	)

	set_target_properties(celix_pubsub_admin_zmq PROPERTIES INSTALL_RPATH "$ORIGIN")
	target_link_libraries(celix_pubsub_admin_zmq PRIVATE
			Celix::pubsub_spi
			Celix::framework Celix::dfi Celix::log_helper
			${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY}
	)
	target_include_directories(celix_pubsub_admin_zmq PRIVATE
		${ZMQ_INCLUDE_DIR}
		${CZMQ_INCLUDE_DIR}
		${JANSSON_INCLUDE_DIR}
		src
	)

	install_celix_bundle(celix_pubsub_admin_zmq EXPORT celix COMPONENT pubsub)
	target_link_libraries(celix_pubsub_admin_zmq PRIVATE Celix::shell_api)
	add_library(Celix::pubsub_admin_zmq ALIAS celix_pubsub_admin_zmq)
endif()
