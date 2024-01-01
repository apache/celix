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

#include <dirent.h>
#include <time.h>
#include <unistd.h>

#include "celix/FrameworkFactory.h"
#include "celix/FrameworkUtils.h"

#include "celix_framework_utils_private.h"
#include "celix_file_utils.h"
#include "asprintf_ei.h"
#include "celix_utils_ei.h"
#include "dlfcn_ei.h"
#include "stdlib_ei.h"
#include "unistd_ei.h"

class CelixFrameworkUtilsErrorInjectionTestSuite : public ::testing::Test {
public:
    CelixFrameworkUtilsErrorInjectionTestSuite () {
        framework = celix::createFramework({
            {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
            {CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, "false"}
        });
    }

    ~CelixFrameworkUtilsErrorInjectionTestSuite () override {
        celix_ei_expect_celix_utils_extractZipData(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_extractZipFile(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_dlopen(nullptr, 0, nullptr);
        celix_ei_expect_dlerror(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_realpath(nullptr, 0, nullptr);
        celix_ei_expect_symlink(nullptr, 0, 0);
    }
    std::shared_ptr<celix::Framework> framework{};
};

TEST_F(CelixFrameworkUtilsErrorInjectionTestSuite, testExtractFileBundle) {
    const char* testExtractDir = "extractFileBundleTestDir";
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    celix_ei_expect_celix_utils_extractZipFile(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
    auto status = celix_framework_utils_extractBundle(framework->getCFramework(), SIMPLE_TEST_BUNDLE1_LOCATION, testExtractDir);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    // not enough memory for bundle url validation
    celix_ei_expect_celix_utils_writeOrCreateString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 1);
    status = celix_framework_utils_extractBundle(framework->getCFramework(), SIMPLE_TEST_BUNDLE1_LOCATION, testExtractDir);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    // not enough memory to resolve URL before unzipping
    celix_ei_expect_celix_utils_writeOrCreateString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    status = celix_framework_utils_extractBundle(framework->getCFramework(), SIMPLE_TEST_BUNDLE1_LOCATION, testExtractDir);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_utils_deleteDirectory(testExtractDir, nullptr);
}

TEST_F(CelixFrameworkUtilsErrorInjectionTestSuite, ExtractUncompressedBundleTest) {
    const char* testExtractDir = "extractBundleTestDir";
    const char* testLinkDir = "linkBundleTestDir";
    celix_utils_deleteDirectory(testExtractDir, nullptr);
    unlink(testLinkDir);
    EXPECT_EQ(CELIX_SUCCESS, celix_utils_extractZipFile(SIMPLE_TEST_BUNDLE1_LOCATION, testExtractDir, nullptr));

    // failed to get realpath of bundle
    celix_ei_expect_realpath((void*)celix_framework_utils_extractBundle, 1, nullptr);
    auto status = celix_framework_utils_extractBundle(framework->getCFramework(), testExtractDir, testLinkDir);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES));

    // failed to install symbolic link
    celix_ei_expect_symlink((void*) celix_framework_utils_extractBundle, 1, -1);
    status = celix_framework_utils_extractBundle(framework->getCFramework(), testExtractDir, testLinkDir);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EIO));

    celix_utils_deleteDirectory(testExtractDir, nullptr);
    unlink(testLinkDir);
}

TEST_F(CelixFrameworkUtilsErrorInjectionTestSuite, CheckBundleAge) {
    struct timespec now = {0, 0};
    celix_ei_expect_celix_utils_writeOrCreateString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 1);
    EXPECT_FALSE(celix_framework_utils_isBundleUrlNewerThan(framework->getCFramework(), SIMPLE_TEST_BUNDLE1_LOCATION, &now));
}

TEST_F(CelixFrameworkUtilsErrorInjectionTestSuite, testIsBundleUrlValid) {
    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), "non-existing.zip", false);
    EXPECT_FALSE(valid);
}
