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

	include_directories("${ZMQ_INCLUDE_DIR}")
	include_directories("${CZMQ_INCLUDE_DIR}")
	include_directories("${JANSSON_INCLUDE_DIR}")
	include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/dfi/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/pubsub/api/pubsub")
	include_directories("private/include")
	include_directories("public/include")

	if (BUILD_ZMQ_SECURITY)
		add_definitions(-DBUILD_WITH_ZMQ_SECURITY=1)

		find_package(OpenSSL 1.1.0 REQUIRED)
		include_directories("${OPENSSL_INCLUDE_DIR}")

		set (ZMQ_CRYPTO_C "private/src/zmq_crypto.c")
	endif()

	add_bundle(org.apache.celix.pubsub_admin.PubSubAdminZmq
	    BUNDLE_SYMBOLICNAME "apache_celix_pubsub_admin_zmq"
	    VERSION "1.0.0"
	    SOURCES
	    	private/src/psa_activator.c
	    	private/src/pubsub_admin_impl.c
	    	private/src/topic_subscription.c
	    	private/src/topic_publication.c
	    	${ZMQ_CRYPTO_C}
	    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_endpoint.c
	    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/log_helper.c
	    	${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_utils.c
    	   ${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_admin_match.c
	)

	set_target_properties(org.apache.celix.pubsub_admin.PubSubAdminZmq PROPERTIES INSTALL_RPATH "$ORIGIN")
	target_link_libraries(org.apache.celix.pubsub_admin.PubSubAdminZmq celix_framework celix_utils celix_dfi ${ZMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})
	install_bundle(org.apache.celix.pubsub_admin.PubSubAdminZmq)

endif()
