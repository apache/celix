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

add_celix_bundle(pubsub_avrobin_serialization_descriptor NO_ACTIVATOR VERSION 1.0.0)
celix_bundle_files(pubsub_avrobin_serialization_descriptor
		${CMAKE_CURRENT_SOURCE_DIR}/msg_descriptors/msg_poi1.descriptor
		DESTINATION "META-INF/descriptors"
)

add_executable(test_pubsub_serializer_avrobin
        src/PubSubAvrobinSerializationProviderTestSuite.cc
)
target_link_libraries(test_pubsub_serializer_avrobin PRIVATE Celix::framework Celix::dfi Celix::pubsub_utils GTest::gtest GTest::gtest_main Celix::pubsub_spi)

add_celix_bundle_dependencies(test_pubsub_serializer_avrobin celix_pubsub_serializer_avrobin pubsub_avrobin_serialization_descriptor)
target_compile_definitions(test_pubsub_serializer_avrobin PRIVATE -DSERIALIZATION_BUNDLE=\"$<TARGET_PROPERTY:celix_pubsub_serializer_avrobin,BUNDLE_FILE>\")
target_compile_definitions(test_pubsub_serializer_avrobin PRIVATE -DDESCRIPTOR_BUNDLE=\"$<TARGET_PROPERTY:pubsub_avrobin_serialization_descriptor,BUNDLE_FILE>\")

add_test(NAME test_pubsub_serializer_avrobin COMMAND test_pubsub_serializer_avrobin)
setup_target_for_coverage(test_pubsub_serializer_avrobin SCAN_DIR ..)

