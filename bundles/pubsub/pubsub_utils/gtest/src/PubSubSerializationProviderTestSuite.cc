/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include "gtest/gtest.h"

#include <memory>

#include "celix_api.h"
#include "pubsub_message_serialization_marker.h"
#include "pubsub_serialization_provider.h"

class PubSubSerializationProviderTestSuite : public ::testing::Test {
public:
    PubSubSerializationProviderTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".pubsub_serialization_provider_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        long bndId;

        bndId = celix_bundleContext_installBundle(ctx.get(), DESCRIPTOR_BUNDLE, true);
        EXPECT_TRUE(bndId >= 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};


TEST_F(PubSubSerializationProviderTestSuite, CreateDestroy) {
    //checks if the bundles are started and stopped correctly (no mem leaks).
    auto* provider = pubsub_serializationProvider_create(ctx.get(), "test", false, 0, nullptr, nullptr, nullptr, nullptr);
    auto count = celix_bundleContext_useService(ctx.get(), PUBSUB_MESSAGE_SERIALIZATION_MARKER_NAME, nullptr, nullptr);
    EXPECT_EQ(1, count);
    pubsub_serializationProvider_destroy(provider);
}

TEST_F(PubSubSerializationProviderTestSuite, FindSerializationServices) {
    auto* provider = pubsub_serializationProvider_create(ctx.get(), "test", false, 0, nullptr, nullptr, nullptr, nullptr);

    size_t nrEntries = pubsub_serializationProvider_nrOfEntries(provider);
    EXPECT_EQ(5, nrEntries);
    size_t nrOfInvalidEntries = pubsub_serializationProvider_nrOfInvalidEntries(provider);
    EXPECT_EQ(3, nrOfInvalidEntries); //note 3 invalid, because garbage.descriptor is never added (cannot extract msgFqn)

    auto* services = celix_bundleContext_findServices(ctx.get(), PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME);
    EXPECT_EQ(5, celix_arrayList_size(services)); //3 valid, 5 invalid
    celix_arrayList_destroy(services);

    pubsub_serializationProvider_destroy(provider);
}
