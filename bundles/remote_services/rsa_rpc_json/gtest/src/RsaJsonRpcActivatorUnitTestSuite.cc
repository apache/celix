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
#include "rsa_json_rpc_impl.h"
#include "rsa_json_rpc_constants.h"
#include "celix_bundle_activator.h"
#include "celix_properties.h"
#include "celix_types.h"
#include "celix_framework.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_bundle_ei.h"
#include "malloc_ei.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_log_helper_ei.h"
#include "celix_properties_ei.h"
#include <gtest/gtest.h>

class RsaJsonRpcActivatorUnitTestSuite : public ::testing::Test {
public:
    RsaJsonRpcActivatorUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_json_rpc_impl_cache");
        celix_properties_set(props, CELIX_FRAMEWORK_BUNDLE_VERSION, "1.0.0");
        celix_properties_set(props, RSA_JSON_RPC_LOG_CALLS_KEY, "true");
        celix_properties_set(props, "CELIX_FRAMEWORK_EXTENDER_PATH", RESOURCES_DIR);
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

    }

    ~RsaJsonRpcActivatorUnitTestSuite() override {
        celix_ei_expect_celix_bundle_getManifestValue(nullptr, 0, nullptr);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadRwlock_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celix_logHelper_create(nullptr, 0, 0);
        celix_ei_expect_celix_properties_create(nullptr, 0, 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(RsaJsonRpcActivatorUnitTestSuite, Create) {
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(CELIX_SUCCESS, status);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
    status = celix_bundleActivator_stop(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(RsaJsonRpcActivatorUnitTestSuite, FailedToCreateLogHelper) {
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_ei_expect_celix_logHelper_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(RsaJsonRpcActivatorUnitTestSuite, FailedToCreateRsaJsonRpc) {
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_ei_expect_calloc((void*)&rsaJsonRpc_create, 0, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(RsaJsonRpcActivatorUnitTestSuite, FailedToCreateRpcFactoryServiceProperties) {
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(CELIX_ENOMEM, status);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(RsaJsonRpcActivatorUnitTestSuite, FailedToRegisterRpcFactoryService) {
    celix_ei_expect_celix_bundle_getManifestValue((void*)&rsaJsonRpc_create, 1, "1.0.0");
    void *userData = nullptr;
    auto status = celix_bundleActivator_create(ctx.get(), &userData);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    status = celix_bundleActivator_start(userData, ctx.get());
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);

    status = celix_bundleActivator_destroy(userData, ctx.get());
    EXPECT_EQ(CELIX_SUCCESS, status);
}