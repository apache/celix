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

#include <memory>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "pubsub_serializer_handler.h"
#include "celix/FrameworkFactory.h"
#include "msg.h"
#include "pubsub_interceptor.h"

struct TestData {
    TestData(const std::shared_ptr<celix::BundleContext>& ctx) {
        serHandler = std::shared_ptr<pubsub_serializer_handler>{pubsub_serializerHandler_create(ctx->getCBundleContext(), "json", true), [](pubsub_serializer_handler_t* h) {
            pubsub_serializerHandler_destroy(h);
        }};
    }

    std::shared_ptr<pubsub_serializer_handler_t> serHandler{};

    std::mutex mutex{}; //protects below
    int preSendCount{0};
    int postSendCount{0};
    int preReceiveCount{0};
    int postReceiveCount{0};
    std::condition_variable cond{};
};

static void serializeAndPrint(TestData* testData, uint32_t msgId, const void *msg) {
    struct iovec* vec = nullptr;
    size_t vecLen = 0;
    pubsub_serializerHandler_serialize(testData->serHandler.get(), msgId, msg, &vec, &vecLen);
    if (vecLen > 0) {
        for (size_t i = 0; i < vecLen; ++i) {
            fwrite(vec[i].iov_base, sizeof(char), vec[i].iov_len, stdout);
        }
    }
    fputc('\n', stdout);
    pubsub_serializerHandler_freeSerializedMsg(testData->serHandler.get(), msgId, vec, vecLen);
}

class PubSubInterceptorTestSuite : public ::testing::Test {
public:
    PubSubInterceptorTestSuite() {
        fw = celix::createFramework({
            {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "info"},
            {"CELIX_HTTP_ADMIN_LISTENING_PORTS", "58080"},
            {"CELIX_PUBSUB_TEST_ADD_METADATA", "true"}
        });
        ctx = fw->getFrameworkBundleContext();
        testData = std::make_shared<TestData>(ctx);

        EXPECT_GE(ctx->installBundle(PUBSUB_JSON_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_TOPMAN_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_PSA_BUNDLE_FILE), 0);
        EXPECT_GE(ctx->installBundle(PUBSUB_WIRE_BUNDLE_FILE), 0);
#ifdef HTTP_ADMIN_BUNDLE_FILE
        EXPECT_GE(ctx->installBundle(HTTP_ADMIN_BUNDLE_FILE), 0);
#endif
    }

    std::shared_ptr<celix::ServiceRegistration> createInterceptor(bool cancelSend, bool cancelReceive) {
        auto interceptor = std::make_shared<pubsub_interceptor>(pubsub_interceptor{});
        interceptor->handle = (void*)testData.get();
        interceptor->preSend  = [](void* handle, const pubsub_interceptor_properties_t *, const char *, const uint32_t,
                                   const void *, celix_properties_t* metadata) {
            auto* td = (TestData*)handle;
            celix_properties_set(metadata, "test", "preSend");

            std::lock_guard<std::mutex> lck{td->mutex};
            td->preSendCount += 1;
            td->cond.notify_all();
            return true;
        };
        if (cancelSend) {
            interceptor->preSend = [](void* handle, const pubsub_interceptor_properties_t *, const char *, const uint32_t,
                                      const void *, celix_properties_t*) {
                auto* td = (TestData*)handle;
                std::lock_guard<std::mutex> lck{td->mutex};
                td->preSendCount += 1;
                td->cond.notify_all();
                return false;
            };
        }
        interceptor->postSend = [](void *handle, const pubsub_interceptor_properties_t* intProps, const char *msgType, uint32_t msgId, const void *rawMsg,
                                   celix_properties_t* metadata) {
            auto* td = (TestData*)handle;
            serializeAndPrint(td, msgId, rawMsg);
            EXPECT_STREQ(msgType, "msg");
            const auto *msg = static_cast<const msg_t*>(rawMsg);
            EXPECT_GE(msg->seqNr, 0);
            EXPECT_STREQ(celix_properties_get(metadata, "test", nullptr), "preSend");
            const char *key;
            CELIX_PROPERTIES_FOR_EACH(metadata, key) {
                printf("got property %s=%s\n", key, celix_properties_get(metadata, key, nullptr));
            }
            fprintf(stdout, "Got message in postSend interceptor %s/%s for type %s and ser %s with seq nr %i\n", intProps->scope, intProps->topic, intProps->psaType, intProps->serializationType, msg->seqNr);

            std::lock_guard<std::mutex> lck{td->mutex};
            td->postSendCount += 1;
            td->cond.notify_all();
        };
        interceptor->preReceive = [](void* handle, const pubsub_interceptor_properties_t *, const char *, uint32_t,
                                     const void *, celix_properties_t* metadata) {
            auto* td = (TestData*)handle;
            celix_properties_set(metadata, "test", "preReceive");

            std::lock_guard<std::mutex> lck{td->mutex};
            td->preReceiveCount += 1;
            td->cond.notify_all();
            return true;
        };
        if (cancelReceive) {
            interceptor->preReceive = [](void* handle, const pubsub_interceptor_properties_t *, const char *, uint32_t,
                                         const void *, celix_properties_t*) {
                auto* td = (TestData*)handle;
                std::lock_guard<std::mutex> lck{td->mutex};
                td->preReceiveCount += 1;
                td->cond.notify_all();
                return false;
            };
        }
        interceptor->postReceive = [](void *handle, const pubsub_interceptor_properties_t* intProps, const char *msgType, uint32_t msgId, const void *rawMsg,
                                      celix_properties_t* metadata) {
            auto* td = (TestData*)handle;
            serializeAndPrint(td, msgId, rawMsg);
            EXPECT_STREQ(msgType, "msg");
            const auto *msg = static_cast<const msg_t*>(rawMsg);
            EXPECT_GE(msg->seqNr, 0);
            EXPECT_STREQ(celix_properties_get(metadata, "test", nullptr), "preReceive");
            fprintf(stdout, "Got message in postReceive interceptor %s/%s for type %s and ser %s with seq nr %i\n", intProps->scope, intProps->topic, intProps->psaType, intProps-> serializationType, msg->seqNr);

            std::lock_guard<std::mutex> lck{td->mutex};
            td->postReceiveCount += 1;
            td->cond.notify_all();
        };
        //note registering identical services to validate multiple interceptors
        return ctx->registerService<pubsub_interceptor>(std::move(interceptor), PUBSUB_INTERCEPTOR_SERVICE_NAME)
                .setUnregisterAsync(false) //note to ensure test data is still valid when service is registered
                .build();
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
    std::shared_ptr<TestData> testData{};
};

TEST_F(PubSubInterceptorTestSuite, InterceptorWithSinglePublishersAndMultipleReceivers) {
    //Given a publisher (PUBSUB_PUBLISHER_BUNDLE_FILE) and 2 receivers (PUBSUB_SUBSCRIBER_BUNDLE_FILE)
    //And several registered interceptors
    //Then the interceptor receives a correct msg type.

    auto reg1 = createInterceptor(false, false);
    auto reg2 = createInterceptor(false, false);
    auto reg3 = createInterceptor(false, false);
    ctx->waitForEvents();

    EXPECT_GE(ctx->installBundle(PUBSUB_PUBLISHER_BUNDLE_FILE), 0);
    EXPECT_GE(ctx->installBundle(PUBSUB_SUBSCRIBER_BUNDLE_FILE), 0);

    std::unique_lock<std::mutex> lck{testData->mutex};
    auto isTestDone = testData->cond.wait_for(lck, std::chrono::seconds{5}, [this]{
        return  testData->preSendCount > 10 &&
                testData->postSendCount > 10 &&
                testData->preReceiveCount > 10 &&
                testData->postReceiveCount > 10;
    });

    EXPECT_TRUE(isTestDone);
}

TEST_F(PubSubInterceptorTestSuite, InterceptorWithPreSendCancelWillPreventSends) {
    //Given a publisher (PUBSUB_PUBLISHER_BUNDLE_FILE) and 2 receivers (PUBSUB_SUBSCRIBER_BUNDLE_FILE)
    //And a interceptor which cancel a send
    //Then only the preSend count will be increased, but the rest of the count will be 0

    auto reg1 = createInterceptor(true, false);
    ctx->waitForEvents();

    EXPECT_GE(ctx->installBundle(PUBSUB_PUBLISHER_BUNDLE_FILE), 0);
    EXPECT_GE(ctx->installBundle(PUBSUB_SUBSCRIBER_BUNDLE_FILE), 0);

    std::unique_lock<std::mutex> lck{testData->mutex};
    auto isTestDone = testData->cond.wait_for(lck, std::chrono::seconds{5}, [this]{
        return  testData->preSendCount > 10 ;
    });

    EXPECT_EQ(testData->postSendCount, 0);
    EXPECT_EQ(testData->preReceiveCount, 0);
    EXPECT_EQ(testData->postReceiveCount, 0);

    EXPECT_TRUE(isTestDone);
}

TEST_F(PubSubInterceptorTestSuite, InterceptorWithPreRedeiveCancelWillPreventPostReceive) {
    //Given a publisher (PUBSUB_PUBLISHER_BUNDLE_FILE) and 2 receivers (PUBSUB_SUBSCRIBER_BUNDLE_FILE)
    //And a interceptor which cancel a receive
    //Then the preSend, postSend and preReceive count will be increased, but the postReceive count will be 0

    auto reg1 = createInterceptor(false, true);
    ctx->waitForEvents();

    EXPECT_GE(ctx->installBundle(PUBSUB_PUBLISHER_BUNDLE_FILE), 0);
    EXPECT_GE(ctx->installBundle(PUBSUB_SUBSCRIBER_BUNDLE_FILE), 0);

    std::unique_lock<std::mutex> lck{testData->mutex};
    auto isTestDone = testData->cond.wait_for(lck, std::chrono::seconds{5}, [this]{
        return  testData->preSendCount > 10 &&
                testData->postSendCount > 10 &&
                testData->preReceiveCount > 10;
    });

    EXPECT_EQ(testData->postReceiveCount, 0);

    EXPECT_TRUE(isTestDone);
}