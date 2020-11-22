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

class PubSubJsonSerializationProviderTestSuite : public ::testing::Test {
public:
    PubSubJsonSerializationProviderTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".pubsub_json_serializer_cache");
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


TEST_F(PubSubJsonSerializationProviderTestSuite, CreateDestroy) {
    //checks if the bundles are started and stopped correctly (no mem leaks).
}

TEST_F(PubSubJsonSerializationProviderTestSuite, FindSerializationServices) {
    auto* services = celix_bundleContext_findServices(ctx.get(), PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME);
    EXPECT_EQ(1, celix_arrayList_size(services));
    celix_arrayList_destroy(services);
}

struct poi1 {
    struct {
        double lat;
        double lon;
    } location;
    const char *name;
};

TEST_F(PubSubJsonSerializationProviderTestSuite, SerializeTest) {
    poi1 p;
    p.location.lat = 42;
    p.location.lon = 43;
    p.name = "test";

    celix_service_use_options_t opts{};
    opts.filter.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
    opts.filter.filter = "(msg.fqn=poi1)";
    opts.callbackHandle = static_cast<void*>(&p);
    opts.use = [](void *handle, void *svc) {
        //auto *poi = static_cast<poi1*>(handle);
        auto* ser = static_cast<pubsub_message_serialization_service_t*>(svc);
        struct iovec* outVec = NULL;
        size_t outSize = 0;
        int rc = ser->serialize(ser->handle, handle, &outVec, &outSize);
        EXPECT_EQ(0, rc);
        EXPECT_TRUE(strstr(static_cast<char*>(outVec->iov_base), "\"lat\":42") != NULL);
        ser->freeSerializedMsg(ser->handle, outVec, outSize);
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);
}

TEST_F(PubSubJsonSerializationProviderTestSuite, DeserializeTest) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
    opts.filter.filter = "(msg.fqn=poi1)";
    opts.callbackHandle = nullptr;
    opts.use = [](void *, void *svc) {
        auto* ser = static_cast<pubsub_message_serialization_service_t*>(svc);
        const char* data = R"({"location":{"lat":42.0,"lon":43.0},"name":"test"})";
        poi1 *p = nullptr;
        iovec inVec;
        inVec.iov_base = static_cast<void*>(const_cast<char*>(data));
        inVec.iov_len = strlen(data);
        ser->deserialize(ser->handle, &inVec, 1, (void**)(&p));
        EXPECT_EQ(42,p->location.lat);
        EXPECT_EQ(43,p->location.lon);
        EXPECT_STREQ("test", p->name);
        ser->freeDeserializedMsg(ser->handle, p);
    };
    bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);
}
