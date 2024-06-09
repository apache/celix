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
#include "endpoint_listener.h"
#include "celix_framework_factory.h"
#include "celix_bundle_context.h"
#include "celix_constants.h"
#include <gtest/gtest.h>

class DiscoveryZeroconfIntegrationTestSuite : public ::testing::Test {
public:
    DiscoveryZeroconfIntegrationTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".dzc_integration_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        const char* bundleFile = DISCOVERY_ZEROCONF_BUNDLE;
        long bundleId;
        bundleId = celix_bundleContext_installBundle(ctx.get(), bundleFile, true);
        EXPECT_TRUE(bundleId >= 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(DiscoveryZeroconfIntegrationTestSuite, FindEndpointListener) {
    auto found = celix_bundleContext_findService(ctx.get(), CELIX_RSA_ENDPOINT_LISTENER_SERVICE_NAME);
    EXPECT_TRUE(found);
}
