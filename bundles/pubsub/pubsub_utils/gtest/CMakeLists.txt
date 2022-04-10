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

add_celix_bundle(pubsub_serialization_matching_descriptors NO_ACTIVATOR VERSION 1.0.0)
celix_bundle_files(pubsub_serialization_matching_descriptors
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1.descriptor
		DESTINATION "META-INF/descriptors"
		)

celix_bundle_files(pubsub_serialization_matching_descriptors
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/fiets.properties
		DESTINATION "META-INF/topics/pub"
		)

celix_bundle_files(pubsub_serialization_matching_descriptors
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/fiets.properties
		DESTINATION "META-INF/topics/sub"
		)

add_celix_bundle(pubsub_serialization_provider_descriptors NO_ACTIVATOR VERSION 1.0.0)
celix_bundle_files(pubsub_serialization_provider_descriptors
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/garbage.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poiCmd.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi2_variant1.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi2_variant2.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi3_variant1.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi3_variant2.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi4_variant1.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi4_variant2.descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/int_calc.descriptor
		DESTINATION "META-INF/descriptors"
)

add_executable(test_pubsub_utils
		src/PubSubSerializationHandlerTestSuite.cc
		src/PubSubSerializationProviderTestSuite.cc
		src/PubSubMatchingTestSuite.cpp
)
target_link_libraries(test_pubsub_utils PRIVATE Celix::framework Celix::pubsub_utils GTest::gtest GTest::gtest_main Celix::pubsub_spi)
add_test(NAME test_pubsub_utils COMMAND test_pubsub_utils)
setup_target_for_coverage(test_pubsub_utils SCAN_DIR ..)
add_celix_bundle_dependencies(test_pubsub_utils pubsub_serialization_matching_descriptors pubsub_serialization_provider_descriptors)
target_compile_definitions(test_pubsub_utils PRIVATE -DDESCRIPTOR_BUNDLE=\"$<TARGET_PROPERTY:pubsub_serialization_provider_descriptors,BUNDLE_FILE>\")
target_compile_definitions(test_pubsub_utils PRIVATE -DMATCHING_BUNDLE=\"$<TARGET_PROPERTY:pubsub_serialization_matching_descriptors,BUNDLE_FILE>\")