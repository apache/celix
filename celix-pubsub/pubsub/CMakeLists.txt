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

	include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
	include_directories("${PROJECT_SOURCE_DIR}/framework/public/include")

	option(BUILD_ZMQ_SECURITY "Build with security for ZeroMQ." OFF)
	
	set (PUBSUB_SERIALIZER_SRC "${PROJECT_SOURCE_DIR}/pubsub/pubsub_common/public/src/pubsub_serializer.c")
	set (SERIALIZER_PATH "" CACHE FILEPATH "Path to the directory which will contain the serializer (include / src).")
	set (SERIALIZER_LIB_INCLUDE_DIR "" CACHE FILEPATH "Path to the include dir of the addiotional libs.")
	set (SERIALIZER_LIB_PATH "" CACHE FILEPATH "Path to the additional library.")
	if (EXISTS ${SERIALIZER_PATH})
		file (GLOB PUBSUB_SERIALIZER_SRC ${SERIALIZER_PATH}/src/*.c)
		
		if (SERIALIZER_LIB_PATH)
			get_filename_component(SERIALIZER_LIB_DIR ${SERIALIZER_LIB_PATH} DIRECTORY)
			get_filename_component(SERIALIZER_LIB_FULLNAME ${SERIALIZER_LIB_PATH} NAME_WE)
			string (REPLACE "lib" "" SERIALIZER_LIBRARY ${SERIALIZER_LIB_FULLNAME})
		endif()
	endif()
	
	add_subdirectory(pubsub_topology_manager)
	add_subdirectory(pubsub_discovery)
	add_subdirectory(pubsub_admin_zmq)
	add_subdirectory(pubsub_admin_udp_mc)
	add_subdirectory(examples)
	add_subdirectory(deploy)
	add_subdirectory(keygen)
	
endif(PUBSUB)
