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
#include "remote_service_admin.h"
#include "celix_api.h"
#include <gtest/gtest.h>

class RsaShmActivatorTestSuite : public ::testing::Test {
public:
    RsaShmActivatorTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_setBool(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        const char* bundleFile = RSA_SHM_BUNDLE;
        long bundleId{-1};
        bundleId = celix_bundleContext_installBundle(ctx.get(), bundleFile, true);
        EXPECT_TRUE(bundleId >= 0);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(RsaShmActivatorTestSuite, FindRsaShmService) {
    celix_bundleContext_waitForEvents(ctx.get());
    long found = celix_bundleContext_findService(ctx.get(), OSGI_RSA_REMOTE_SERVICE_ADMIN);
    EXPECT_GE(found, 0);
}
