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
#include <celix_api.h>
#include <gtest/gtest.h>
#include <rsa_rpc_factory.h>

class RsaJsonRpcActivatorTestSuite : public ::testing::Test {
public:
    RsaJsonRpcActivatorTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".rsa_json_rpc_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        const char* bundleFile = RSA_JSON_RPC_BUNDLE;
        long bundleId{-1};
        bundleId = celix_bundleContext_installBundle(ctx.get(), bundleFile, true);
        EXPECT_TRUE(bundleId >= 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

static void useRsaJsonRpcServiceCallback(void *handle __attribute__((__unused__)), void *svc __attribute__((__unused__)), const celix_properties_t *props) {
    const char *rpcType = celix_properties_get(props, RSA_RPC_TYPE_KEY, NULL);
    EXPECT_STREQ("celix.remote.admin.rpc_type.json", rpcType);
}

TEST_F(RsaJsonRpcActivatorTestSuite, UseService) {
    celix_service_use_options_t opts{};
    opts.filter.serviceName = RSA_RPC_FACTORY_NAME;
    opts.callbackHandle = this;
    opts.useWithProperties = useRsaJsonRpcServiceCallback;
    opts.waitTimeoutInSeconds = 30;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);
}