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
#include "rsa_json_rpc_constants.h"
#include "rsa_request_sender_tracker.h"
#include "rsa_request_sender_service.h"
#include "rsa_request_handler_service.h"
#include "endpoint_description.h"
#include "celix_properties.h"
#include "celix_types.h"
#include "celix_framework.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include <gtest/gtest.h>

class RsaRequestSenderTrackerUnitTestSuite : public ::testing::Test {
public:
    RsaRequestSenderTrackerUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_json_rpc_impl_cache");
        celix_properties_set(props, RSA_JSON_RPC_LOG_CALLS_KEY, "true");
        celix_properties_set(props, "CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR);
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"RsaJsonRpc");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};

        static rsa_request_sender_service_t reqSenderSvc{};
        reqSenderSvc.handle = nullptr;
        reqSenderSvc.sendRequest = [](void *handle, const endpoint_description_t *endpointDesc, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) -> celix_status_t {
            (void)handle;//unused
            (void)endpointDesc;//unused
            (void)metadata;//unused
            (void)request;//unused
            response->iov_base = strdup("{}");
            response->iov_len = 2;
            return CELIX_SUCCESS;
        };
        celix_service_registration_options_t opts{};
        opts.serviceName = CELIX_RSA_REQUEST_SENDER_SERVICE_NAME;
        opts.serviceVersion = CELIX_RSA_REQUEST_SENDER_SERVICE_VERSION;
        opts.svc = &reqSenderSvc;
        reqSenderSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx.get(), &opts);
        EXPECT_NE(-1, reqSenderSvcId);
    }

    ~RsaRequestSenderTrackerUnitTestSuite() override {
        celix_bundleContext_unregisterServiceAsync(ctx.get(), reqSenderSvcId, nullptr, nullptr);
        celix_bundleContext_waitForEvents(ctx.get());

        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    long reqSenderSvcId{};
};

TEST_F(RsaRequestSenderTrackerUnitTestSuite, CreateRsaRequestSenderTracker) {
    rsa_request_sender_tracker_t *tracker = nullptr;
    auto status = rsaRequestSenderTracker_create(ctx.get(), logHelper.get(),&tracker);
    EXPECT_EQ(CELIX_SUCCESS, status);
    rsaRequestSenderTracker_destroy(tracker);
}

TEST_F(RsaRequestSenderTrackerUnitTestSuite, CreateRsaRequestSenderTrackerWithInvalidParams) {
    rsa_request_sender_tracker_t *tracker = nullptr;
    auto status = rsaRequestSenderTracker_create(nullptr, logHelper.get(),&tracker);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaRequestSenderTracker_create(ctx.get(), nullptr,&tracker);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaRequestSenderTracker_create(ctx.get(), logHelper.get(),nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaRequestSenderTrackerUnitTestSuite, FailedToCreateRsaRequestSenderTrackerLock) {
    rsa_request_sender_tracker_t *tracker = nullptr;
    celix_ei_expect_celixThreadRwlock_create((void*)&rsaRequestSenderTracker_create, 0, CELIX_ENOMEM);
    auto status = rsaRequestSenderTracker_create(ctx.get(), logHelper.get(), &tracker);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaRequestSenderTrackerUnitTestSuite, FailedToTrackRsaRequestSenderService) {
    rsa_request_sender_tracker_t *tracker = nullptr;
    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&rsaRequestSenderTracker_create, 0, -1);
    auto status = rsaRequestSenderTracker_create(ctx.get(), logHelper.get(), &tracker);
    EXPECT_EQ(CELIX_SERVICE_EXCEPTION, status);
}

TEST_F(RsaRequestSenderTrackerUnitTestSuite, UseService) {
    rsa_request_sender_tracker_t *tracker = nullptr;
    auto status = rsaRequestSenderTracker_create(ctx.get(), logHelper.get(),&tracker);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(ctx.get());

    status = rsaRequestSenderTracker_useService(tracker, reqSenderSvcId, nullptr, [](void *handle, rsa_request_sender_service_t *svc) -> celix_status_t {
        (void)handle;//unused
        (void)svc;//unused
        return CELIX_SUCCESS;
        });
    EXPECT_EQ(CELIX_SUCCESS, status);

    rsaRequestSenderTracker_destroy(tracker);
}

TEST_F(RsaRequestSenderTrackerUnitTestSuite, UseNoExistService) {
    rsa_request_sender_tracker_t *tracker = nullptr;
    auto status = rsaRequestSenderTracker_create(ctx.get(), logHelper.get(),&tracker);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(ctx.get());

    status = rsaRequestSenderTracker_useService(tracker, 100/*set a no exist service*/, nullptr, [](void *handle, rsa_request_sender_service_t *svc) -> celix_status_t {
        (void)handle;//unused
        (void)svc;//unused
        return CELIX_SUCCESS;
    });
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    rsaRequestSenderTracker_destroy(tracker);
}


