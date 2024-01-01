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

#include <dfi_utils.h>
#include <celix_log_helper.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "celix_constants.h"
#include "celix_framework_factory.h"

class DfiUtilsTestSuite : public ::testing::Test {
public:
    DfiUtilsTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_dfi_utils_cache");
        celix_properties_set(props, "CELIX_FRAMEWORK_EXTENDER_PATH", DESCRIPTOR_FILE_PATH);
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        logHelper = std::shared_ptr<celix_log_helper_t>{celix_logHelper_create(ctx.get(),
                "test_rsa_dfi_utils"), [](auto *logger){celix_logHelper_destroy(logger);}};

        const char* descBundleFile = DESCRIPTOR_BUNDLE;
        descBundleId = celix_bundleContext_installBundle(ctx.get(), descBundleFile, true);
        EXPECT_TRUE(descBundleId >= 0);
    }

    long descBundleId{-1};
    std::string curTestDescFile{};
    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
};

static void useBundleCallback(void *handle, const celix_bundle_t *bundle) {
    DfiUtilsTestSuite *testSuite = static_cast<DfiUtilsTestSuite *>(handle);
    dyn_interface_type *intfOut{nullptr};
    auto status = dfi_findAndParseInterfaceDescriptor(testSuite->logHelper.get(), testSuite->ctx.get(), bundle, testSuite->curTestDescFile.c_str(), &intfOut);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_TRUE(intfOut != nullptr);
    dynInterface_destroy(intfOut);
}

TEST_F(DfiUtilsTestSuite, ParseDescriptor) {
    curTestDescFile = "rsa_dfi_utils_test";
    bool found = celix_bundleContext_useBundle(ctx.get(), descBundleId, this, useBundleCallback);
    EXPECT_TRUE(found);
}

TEST_F(DfiUtilsTestSuite, ParseDescriptorForFw) {
    curTestDescFile = "rsa_dfi_utils_test";
    bool found = celix_bundleContext_useBundle(ctx.get(), 0, this, useBundleCallback);
    EXPECT_TRUE(found);
}

static void useBundleCallbackForPasreNonexistentFile(void *handle, const celix_bundle_t *bundle) {
    DfiUtilsTestSuite *testSuite = static_cast<DfiUtilsTestSuite *>(handle);
    dyn_interface_type *intfOut{nullptr};
    auto status = dfi_findAndParseInterfaceDescriptor(testSuite->logHelper.get(), testSuite->ctx.get(), bundle, testSuite->curTestDescFile.c_str(), &intfOut);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
    EXPECT_TRUE(intfOut == nullptr);
}

TEST_F(DfiUtilsTestSuite, DescriptorFileNoExist) {
    curTestDescFile = "nonexistent-file";
    bool found = celix_bundleContext_useBundle(ctx.get(), descBundleId, this, useBundleCallbackForPasreNonexistentFile);
    EXPECT_TRUE(found);
}