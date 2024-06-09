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
#include "celix_event_admin_service.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include <gtest/gtest.h>

class CelixEventAdminBundleTestSuite : public ::testing::Test {
public:
    CelixEventAdminBundleTestSuite() {
        auto props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".event_admin_integration_test_cache");
        auto fwPtr = celix_frameworkFactory_createFramework(props);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](celix_framework_t* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{celix_framework_getFrameworkContext(fw.get()), [](celix_bundle_context_t*){/*nop*/}};

        bundleId = celix_bundleContext_installBundle(ctx.get(), EVENT_ADMIN_BUNDLE, true);
        EXPECT_TRUE(bundleId >= 0);
    }

    ~CelixEventAdminBundleTestSuite() override {
        celix_bundleContext_uninstallBundle(ctx.get(), bundleId);
    }

    long bundleId{};
    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
};

TEST_F(CelixEventAdminBundleTestSuite, InstallBundleTest) {
    celix_bundleContext_waitForEvents(ctx.get());
    long svcId = celix_bundleContext_findService(ctx.get(), CELIX_EVENT_ADMIN_SERVICE_NAME);
    EXPECT_TRUE(svcId >= 0);
}