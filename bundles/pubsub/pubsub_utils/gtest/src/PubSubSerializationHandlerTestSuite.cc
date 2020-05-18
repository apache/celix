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
#include "pubsub_serializer_handler.h"

class PubSubSerializationHandlerTestSuite : public ::testing::Test {
public:
    PubSubSerializationHandlerTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".pubsub_utils_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        msgSerSvc.handle = this;
        msgSerSvc.serialize = [](void* handle, const void*, struct iovec**, size_t*) -> celix_status_t {
            auto* suite = static_cast<PubSubSerializationHandlerTestSuite*>(handle);
            suite->serializeCallCount += 1;
            return CELIX_SUCCESS;
        };
        msgSerSvc.freeSerializedMsg = [](void* handle, struct iovec* , size_t) {
            auto* suite = static_cast<PubSubSerializationHandlerTestSuite*>(handle);
            suite->freeSerializedMsgCallCount += 1;
        };
        msgSerSvc.deserialize = [](void* handle, const struct iovec*, size_t, void**) -> celix_status_t {
            auto* suite = static_cast<PubSubSerializationHandlerTestSuite*>(handle);
            suite->deserializeCallCount += 1;
            return CELIX_SUCCESS;
        };
        msgSerSvc.freeDeserializedMsg = [](void* handle, void*) {
            auto* suite = static_cast<PubSubSerializationHandlerTestSuite*>(handle);
            suite->freeDeserializedMsgCallCount += 1;
        };
    }

    long registerSerSvc(const char* type, uint32_t msgId, const char* msgFqn, const char* msgVersion) {
        auto* p =celix_properties_create();
        celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, type);
        celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, std::to_string(msgId).c_str());
        celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, msgFqn);
        celix_properties_set(p, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, msgVersion);
        celix_service_registration_options_t opts{};
        opts.svc = static_cast<void*>(&msgSerSvc);
        opts.properties = p;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
        return celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    pubsub_message_serialization_service_t  msgSerSvc{};

    size_t serializeCallCount = 0;
    size_t freeSerializedMsgCallCount = 0;
    size_t deserializeCallCount = 0;
    size_t freeDeserializedMsgCallCount = 0;
};


TEST_F(PubSubSerializationHandlerTestSuite, CreateDestroy) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);
    ASSERT_TRUE(handler != nullptr);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, SerializationServiceFound) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);
    long svcId = registerSerSvc("json", 42, "example::Msg", "1.0.0");

    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    EXPECT_EQ(42, pubsub_serializerHandler_getMsgId(handler, "example::Msg"));
    auto *fqn = pubsub_serializerHandler_getMsgFqn(handler, 42);
    EXPECT_STREQ("example::Msg",  fqn);
    free(fqn);
    EXPECT_TRUE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 0));
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 2, 0));

    celix_bundleContext_unregisterService(ctx.get(), svcId);
    EXPECT_EQ(0, pubsub_serializerHandler_messageSerializationServiceCount(handler));

    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, DifferentTypeOfSerializationService) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);
    long svcId = registerSerSvc("avrobin", 42, "example::Msg", "1.0.0");
    EXPECT_EQ(0, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    celix_bundleContext_unregisterService(ctx.get(), svcId);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, MutipleSerializationServices) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);
    long svcId1 = registerSerSvc("json", 43, "example::Msg1", "1.0.0");
    long svcId2 = registerSerSvc("json", 44, "example::Msg2", "1.0.0");
    long svcId3 = registerSerSvc("json", 45, "example::Msg3", "1.0.0");
    long svcId4 = registerSerSvc("json", 46, "example::Msg4", "1.0.0");
    long svcId5 = registerSerSvc("json", 47, "example::Msg5", "1.0.0");
    long svcId6 = registerSerSvc("json", 48, "example::Msg6", "1.0.0");
    EXPECT_EQ(6, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    celix_bundleContext_unregisterService(ctx.get(), svcId2);
    celix_bundleContext_unregisterService(ctx.get(), svcId3);
    celix_bundleContext_unregisterService(ctx.get(), svcId4);
    celix_bundleContext_unregisterService(ctx.get(), svcId5);
    celix_bundleContext_unregisterService(ctx.get(), svcId6);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, ClashingId) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);

    testing::internal::CaptureStderr();
    long svcId1 = registerSerSvc("json", 42, "example::Msg1", "1.0.0");
    long svcId2 = registerSerSvc("json", 42, "example::Msg2", "1.0.0");
    std::string output = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(strstr(output.c_str(), "error") != nullptr);
    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    celix_bundleContext_unregisterService(ctx.get(), svcId2);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, MultipleVersions) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);

    testing::internal::CaptureStderr();
    long svcId1 = registerSerSvc("json", 42, "example::Msg1", "1.0.0"); //successful
    long svcId2 = registerSerSvc("json", 42, "example::Msg1", "1.1.0"); //ERROR
    long svcId3 = registerSerSvc("json", 42, "example::Msg1", "2.1.0"); //ERROR
    std::string output = testing::internal::GetCapturedStderr();

    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    EXPECT_TRUE(strstr(output.c_str(), "error") != nullptr);
    EXPECT_TRUE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 0));
    EXPECT_TRUE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 1));
    EXPECT_TRUE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 14));
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 2, 1));
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 2, 0));

    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    celix_bundleContext_unregisterService(ctx.get(), svcId2);
    celix_bundleContext_unregisterService(ctx.get(), svcId3);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, NoBackwardsCompatbile) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", false);

    long svcId1 = registerSerSvc("json", 42, "example::Msg1", "1.0.0");

    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    EXPECT_TRUE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 0)); //NOTE only exact is supported
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 1));
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 1, 14));
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 2, 1));
    EXPECT_FALSE(pubsub_serializerHandler_isMessageSupported(handler, 42, 2, 0));

    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, CallServiceMethods) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", false);

    long svcId1 = registerSerSvc("json", 42, "example::Msg1", "1.0.0");

    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    EXPECT_EQ(0, serializeCallCount);
    EXPECT_EQ(0, freeSerializedMsgCallCount);
    EXPECT_EQ(0, deserializeCallCount);
    EXPECT_EQ(0, freeDeserializedMsgCallCount);
    pubsub_serializerHandler_serialize(handler, 42, nullptr, nullptr, nullptr);
    pubsub_serializerHandler_freeSerializedMsg(handler, 42, nullptr, 0);
    pubsub_serializerHandler_deserialize(handler, 42, 1, 0, nullptr, 0, nullptr);
    pubsub_serializerHandler_freeDeserializedMsg(handler, 42, nullptr);
    EXPECT_EQ(1, serializeCallCount);
    EXPECT_EQ(1, freeSerializedMsgCallCount);
    EXPECT_EQ(1, deserializeCallCount);
    EXPECT_EQ(1, freeDeserializedMsgCallCount);

    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, MismatchedCallServiceMethods) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", false);

    long svcId1 = registerSerSvc("json", 42, "example::Msg1", "1.0.0");

    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    EXPECT_EQ(0, serializeCallCount);
    EXPECT_EQ(0, freeSerializedMsgCallCount);
    EXPECT_EQ(0, deserializeCallCount);
    EXPECT_EQ(0, freeDeserializedMsgCallCount);
    pubsub_serializerHandler_serialize(handler, 43, nullptr, nullptr, nullptr);
    pubsub_serializerHandler_freeSerializedMsg(handler, 43, nullptr, 0);
    pubsub_serializerHandler_deserialize(handler, 43, 1, 0, nullptr, 0, nullptr);
    pubsub_serializerHandler_deserialize(handler, 42, 1, 1, nullptr, 0, nullptr); //note wrong version
    pubsub_serializerHandler_freeDeserializedMsg(handler, 43, nullptr);
    EXPECT_EQ(0, serializeCallCount);
    EXPECT_EQ(0, freeSerializedMsgCallCount);
    EXPECT_EQ(0, deserializeCallCount);
    EXPECT_EQ(0, freeDeserializedMsgCallCount);

    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    pubsub_serializerHandler_destroy(handler);
}

TEST_F(PubSubSerializationHandlerTestSuite, BackwardsCompatibleCall) {
    auto *handler = pubsub_serializerHandler_create(ctx.get(), "json", true);

    long svcId1 = registerSerSvc("json", 42, "example::Msg1", "1.0.0");

    EXPECT_EQ(1, pubsub_serializerHandler_messageSerializationServiceCount(handler));
    EXPECT_EQ(0, deserializeCallCount);
    pubsub_serializerHandler_deserialize(handler, 42, 1, 0, nullptr, 0, nullptr);
    pubsub_serializerHandler_deserialize(handler, 42, 1, 1, nullptr, 0, nullptr); //note compatible
    pubsub_serializerHandler_deserialize(handler, 42, 1, 15, nullptr, 0, nullptr); //note compatible
    pubsub_serializerHandler_deserialize(handler, 42, 2, 9, nullptr, 0, nullptr); //note not compatible
    EXPECT_EQ(3, deserializeCallCount);

    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    pubsub_serializerHandler_destroy(handler);
}