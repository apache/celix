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

#include <celix_api.h>
#include "pubsub_message_serialization_service.h"

class PubSubAvrobinSerializationProviderTestSuite : public ::testing::Test {
public:
    PubSubAvrobinSerializationProviderTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".pubsub_avrobin_serializer_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        const char* descBundleFile = DESCRIPTOR_BUNDLE;
        const char* serBundleFile = SERIALIZATION_BUNDLE;
        long bndId;

        bndId = celix_bundleContext_installBundle(ctx.get(), descBundleFile, true);
        EXPECT_TRUE(bndId >= 0);

        bndId = celix_bundleContext_installBundle(ctx.get(), serBundleFile, true);
        EXPECT_TRUE(bndId >= 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};


TEST_F(PubSubAvrobinSerializationProviderTestSuite, CreateDestroy) {
    //checks if the bundles are started and stopped correctly (no mem leaks).
}

TEST_F(PubSubAvrobinSerializationProviderTestSuite, FindSerializationServices) {
    auto* services = celix_bundleContext_findServices(ctx.get(), PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME);
    EXPECT_EQ(1, celix_arrayList_size(services)); //3 valid, 5 invalid
    celix_arrayList_destroy(services);
}

TEST_F(PubSubAvrobinSerializationProviderTestSuite, SerializeAndDeserializeTest) {
    struct poi1 {
        struct {
            double lat;
            double lon;
        } location;
        const char *name;
    };

    struct data {
        poi1* input;
        poi1* output;
    };

    poi1 input;
    input.location.lat = 42;
    input.location.lon = 43;
    input.name = "test";

    poi1 output;
    memset(&output, 0, sizeof(output));

    data dataHandle;
    dataHandle.input = &input;
    dataHandle.output = &output;

    celix_service_use_options_t opts{};
    opts.filter.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
    opts.filter.filter = "(msg.fqn=poi1)";
    opts.callbackHandle = static_cast<void*>(&dataHandle);
    opts.use = [](void *handle, void *svc) {
        auto *dh = static_cast<data*>(handle);
        auto* ser = static_cast<pubsub_message_serialization_service_t*>(svc);
        struct iovec* serVec;
        size_t serSize;
        ser->serialize(ser->handle, dh->input, &serVec, &serSize);
        ser->deserialize(ser->handle, serVec, serSize, (void**)(&dh->output));

        EXPECT_EQ(42, dh->output->location.lat);

        ser->freeSerializedMsg(ser->handle, serVec, serSize);
        ser->freeDeserializedMsg(ser->handle, dh->output);
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);
}
