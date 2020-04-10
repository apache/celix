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

add_celix_bundle(pubsub_seriailizer_json_descriptors NO_ACTIVATOR VERSION 1.0.0)
celix_bundle_files(pubsub_seriailizer_json_descriptors
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/garbage.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1_invalid1.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1_invalid2.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1_invalid3.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1_invalid4.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi2.descriptor
	${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poiCmd.descriptor
	DESTINATION "META-INF/descriptors"
)

add_executable(test_pubsub_serializer_json
		src/PubSubJsonSerializationProviderTestSuite.cc
)
target_link_libraries(test_pubsub_serializer_json PRIVATE Celix::framework Celix::pubsub_serializer_json Celix::pubsub_utils GTest::GTest GTest::Main)
target_compile_options(test_pubsub_serializer_json PRIVATE -std=c++14) #Note test code is allowed to be C++14

add_dependencies(test_pubsub_serializer_json celix_pubsub_serializer_json_bundle pubsub_seriailizer_json_descriptors_bundle)
target_compile_definitions(test_pubsub_serializer_json PRIVATE -DSER_BUNDLE=\"$<TARGET_PROPERTY:celix_pubsub_serializer_json,BUNDLE_FILE>\")
target_compile_definitions(test_pubsub_serializer_json PRIVATE -DDESCRIPTOR_BUNDLE=\"$<TARGET_PROPERTY:pubsub_seriailizer_json_descriptors,BUNDLE_FILE>\")

add_test(NAME test_pubsub_serializer_json COMMAND test_pubsub_serializer_json)
SETUP_TARGET_FOR_COVERAGE(test_pubsub_serializer_json_cov test_pubsub_serializer_json ${CMAKE_BINARY_DIR}/coverage/pubsub/pubsub_serializer_json ..)

