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

celix_subproject(PUBSUB "Option to build the pubsub bundles" OFF DEPS UTILS)
if (PUBSUB)

    option(BUILD_PUBSUB_PSA_ZMQ "Build ZeroMQ PubSub Admin (LGPL License)" OFF)
    if (BUILD_PUBSUB_PSA_ZMQ)
		message(WARNING "Celix will now contain a dependency with a LGPL License (ZeroMQ). For more information about this, consult the pubsub/README.md file.")
		option(BUILD_ZMQ_SECURITY "Build with security for ZeroMQ." OFF)
    endif (BUILD_PUBSUB_PSA_ZMQ)

	add_subdirectory(pubsub_api)
	add_subdirectory(pubsub_spi)
	add_subdirectory(pubsub_topology_manager)
	add_subdirectory(pubsub_discovery)
	add_subdirectory(pubsub_serializer_json)
	add_subdirectory(pubsub_admin_zmq)
	add_subdirectory(pubsub_admin_udp_mc)
	add_subdirectory(keygen)
	add_subdirectory(mock)

	add_subdirectory(examples)

	if (ENABLE_TESTING)
		option(BUILD_PUBSUB_TESTS "Enable Tests for PUBSUB" OFF)
	endif()
	if (ENABLE_TESTING AND BUILD_PUBSUB_TESTS AND BUILD_PUBSUB_PSA_ZMQ)
		add_subdirectory(test)
	endif()

endif(PUBSUB)
