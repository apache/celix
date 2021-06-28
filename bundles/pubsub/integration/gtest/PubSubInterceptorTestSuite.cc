/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>

#include "pubsub_serializer_handler.h"
#include "celix/FrameworkFactory.h"
#include "msg.h"
#include "pubsub_interceptor.h"

class PubSubInterceptorTestSuite : public ::testing::Test {
public:
    PubSubInterceptorTestSuite() {
        fw = celix::createFramework({
            {"CELIX_PUBSUB_TEST_ADD_METADATA", "true"} /*TODO memleak in pubsub zmq v2 when metadata is empty*/
        });
        ctx = fw->getFrameworkBundleContext();

        EXPECT_GE(ctx->installBundle(PUBSUB_JSON_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_TOPMAN_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_ZMQ_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_WIRE_BUNDLE_FILE), 0);
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

static void serializeAndPrint(pubsub_serializer_handler_t* ser, uint32_t msgId, const void *msg) {
    struct iovec* vec = nullptr;
    size_t vecLen = 0;
    pubsub_serializerHandler_serialize(ser, msgId, msg, &vec, &vecLen);
    if (vecLen > 0) {
        for (size_t i = 0; i < vecLen; ++i) {
            fwrite(vec[i].iov_base, sizeof(char), vec[i].iov_len, stdout);
        }
    }
    fputc('\n', stdout);
    pubsub_serializerHandler_freeSerializedMsg(ser, msgId, vec, vecLen);
}

std::shared_ptr<celix::ServiceRegistration> createInterceptor(std::shared_ptr<celix::BundleContext>& ctx) {
    auto interceptor = std::shared_ptr<pubsub_interceptor>{new pubsub_interceptor{}, [](pubsub_interceptor* inter) {
        auto* handler = (pubsub_serializer_handler_t*)inter->handle;
        pubsub_serializerHandler_destroy(handler);
        delete inter;
    }};
    interceptor->handle = pubsub_serializerHandler_create(ctx->getCBundleContext(), "json", true);
    interceptor->preSend  = [](void *, const pubsub_interceptor_properties_t *, const char *, const uint32_t,
                               const void *, celix_properties_t* metadata) {
        celix_properties_set(metadata, "test", "preSend");
        return true;
    };
    interceptor->postSend = [](void *handle, const pubsub_interceptor_properties_t* intProps, const char *msgType, uint32_t msgId, const void *rawMsg,
                               const celix_properties_t* metadata) {
        auto* ser = (pubsub_serializer_handler_t*)handle;
        serializeAndPrint(ser, msgId, rawMsg);
        EXPECT_STREQ(msgType, "msg");
        const auto *msg = static_cast<const msg_t*>(rawMsg);
        EXPECT_GE(msg->seqNr, 0);
        EXPECT_STREQ(celix_properties_get(metadata, "test", nullptr), "preSend");
        fprintf(stdout, "Got message in postSend interceptor %s/%s for type %s and ser %s with seq nr %i\n", intProps->scope, intProps->topic, intProps->psaType, intProps->serializationType, msg->seqNr);
    };
    interceptor->preReceive = [](void *, const pubsub_interceptor_properties_t *, const char *, const uint32_t,
                                 const void *, celix_properties_t* metadata) {
        celix_properties_set(metadata, "test", "preReceive");
        return true;
    };
    interceptor->postReceive = [](void *handle, const pubsub_interceptor_properties_t* intProps, const char *msgType, uint32_t msgId, const void *rawMsg,
                                  const celix_properties_t* metadata) {
        auto* ser = (pubsub_serializer_handler_t*)handle;
        serializeAndPrint(ser, msgId, rawMsg);
        EXPECT_STREQ(msgType, "msg");
        const auto *msg = static_cast<const msg_t*>(rawMsg);
        EXPECT_GE(msg->seqNr, 0);
        EXPECT_STREQ(celix_properties_get(metadata, "test", nullptr), "preReceive");
        fprintf(stdout, "Got message in postReceive interceptor %s/%s for type %s and ser %s with seq nr %i\n", intProps->scope, intProps->topic, intProps->psaType, intProps-> serializationType, msg->seqNr);
    };
    //note registering identical services to validate multiple interceptors
    return ctx->registerService<pubsub_interceptor>(interceptor, PUBSUB_INTERCEPTOR_SERVICE_NAME).build();
}

TEST_F(PubSubInterceptorTestSuite, InterceptorWithSinglePublishersAndMultipleReceivers) {
    //Given a publisher (PUBSUB_PUBLISHER_BUNDLE_FILE) and 2 receivers (PUBSUB_SUBSCRIBER_BUNDLE_FILE)
    //And a registered interceptor
    //Then the interceptor receives a correct msg type.

    EXPECT_GE(ctx->installBundle(PUBSUB_PUBLISHER_BUNDLE_FILE), 0);
    EXPECT_GE(ctx->installBundle(PUBSUB_SUBSCRIBER_BUNDLE_FILE), 0);

    auto reg1 = createInterceptor(ctx);
    auto reg2 = createInterceptor(ctx);
    auto reg3 = createInterceptor(ctx);

    //TODO stop after a certain amount of messages send
    //TODO also test with tcp v2.
    sleep(5);
}
