/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#include <gtest/gtest.h>

#include "celix_bundle_context.h"
#include "celix_framework.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_module_private.h"
#include "celix_properties.h"
#include "celix_utils_ei.h"
#include "dlfcn_ei.h"

class CelixBundleContextBundlesWithErrorTestSuite : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    CelixBundleContextBundlesWithErrorTestSuite() {
        properties = celix_properties_create();
        celix_properties_setBool(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", true);
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, false);
        celix_properties_set(properties, CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE, "10");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = celix_framework_getFrameworkContext(fw);
    }

    ~CelixBundleContextBundlesWithErrorTestSuite() override {
        celix_frameworkFactory_destroyFramework(fw);
        celix_ei_expect_dlopen(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
    }

    CelixBundleContextBundlesWithErrorTestSuite(CelixBundleContextBundlesWithErrorTestSuite&&) = delete;
    CelixBundleContextBundlesWithErrorTestSuite(const CelixBundleContextBundlesWithErrorTestSuite&) = delete;
    CelixBundleContextBundlesWithErrorTestSuite& operator=(CelixBundleContextBundlesWithErrorTestSuite&&) = delete;
    CelixBundleContextBundlesWithErrorTestSuite& operator=(const CelixBundleContextBundlesWithErrorTestSuite&) = delete;
};

TEST_F(CelixBundleContextBundlesWithErrorTestSuite, activatorLoadErrorAbortBundleResolution) {
    celix_ei_expect_dlopen(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 1);
    long bndId = celix_bundleContext_installBundle(ctx, SIMPLE_CXX_BUNDLE_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not started

    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void */*handle*/, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(state, CELIX_BUNDLE_STATE_INSTALLED);
    });
    ASSERT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesWithErrorTestSuite, privateLibraryLoadErrorAbortBundleResolution) {
    celix_ei_expect_dlopen(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    long bndId = celix_bundleContext_installBundle(ctx, SIMPLE_CXX_BUNDLE_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not started

    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void */*handle*/, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(state, CELIX_BUNDLE_STATE_INSTALLED);
    });
    ASSERT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesWithErrorTestSuite, failedToGetLibraryPath) {
    celix_ei_expect_celix_utils_writeOrCreateString((void *)celix_module_loadLibraries, 2, nullptr);
    long bndId = celix_bundleContext_installBundle(ctx, SIMPLE_CXX_BUNDLE_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not started

    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void */*handle*/, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(state, CELIX_BUNDLE_STATE_INSTALLED);
    });
    ASSERT_TRUE(called);
}
