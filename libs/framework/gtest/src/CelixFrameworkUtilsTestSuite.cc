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

#include <filesystem>

#include "celix/FrameworkFactory.h"
#include "celix/FrameworkUtils.h"

#include "celix_framework_utils_private.h"
#include "celix_file_utils.h"


class CelixFrameworkUtilsTestSuite : public ::testing::Test {
public:
    CelixFrameworkUtilsTestSuite() {
        framework = celix::createFramework({{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"}});
    }

    std::shared_ptr<celix::Framework> framework{};
};

static void checkBundleCacheDir(const char* extractDir) {
    EXPECT_TRUE(extractDir != nullptr);
    if (extractDir) {
        EXPECT_TRUE(std::filesystem::is_directory(extractDir));
    }
}

TEST_F(CelixFrameworkUtilsTestSuite, testExtractBundlePath) {
    const char* testExtractDir = "extractBundleTestDir";
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    //invalid bundle url -> no extraction
    auto status = celix_framework_utils_extractBundle(framework->getCFramework(), nullptr, testExtractDir);
    EXPECT_NE(status, CELIX_SUCCESS);

    //invalid bundle path -> no extraction
    status = celix_framework_utils_extractBundle(nullptr, "non-existing.zip", testExtractDir); //note nullptr framwork is allowed, fallback to global logger.
    EXPECT_NE(status, CELIX_SUCCESS);

    //invalid url prefix -> no extraction
    std::string path = std::string{"bla://"} + SIMPLE_TEST_BUNDLE1_LOCATION;
    status = celix_framework_utils_extractBundle(framework->getCFramework(), path.c_str(), testExtractDir);
    EXPECT_NE(status, CELIX_SUCCESS);

    //invalid url prefix -> no extraction
    path = std::string{"bla://"};
    status = celix_framework_utils_extractBundle(framework->getCFramework(), path.c_str(), testExtractDir);
    EXPECT_NE(status, CELIX_SUCCESS);

    //invalid url prefix -> no extraction
    path = std::string{"file://"};
    status = celix_framework_utils_extractBundle(framework->getCFramework(), path.c_str(), testExtractDir);
    EXPECT_NE(status, CELIX_SUCCESS);

    //valid bundle path -> extraction
    status = celix_framework_utils_extractBundle(framework->getCFramework(), SIMPLE_TEST_BUNDLE1_LOCATION, testExtractDir);
    EXPECT_EQ(status, CELIX_SUCCESS);
    checkBundleCacheDir(testExtractDir);
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    //valid bundle path with file:// prefix -> extraction
    path = std::string{"file://"} + SIMPLE_TEST_BUNDLE1_LOCATION;
    status = celix_framework_utils_extractBundle(framework->getCFramework(), path.c_str(), testExtractDir);
    EXPECT_EQ(status, CELIX_SUCCESS);
    checkBundleCacheDir(testExtractDir);
    celix_utils_deleteDirectory(testExtractDir, nullptr);
}

TEST_F(CelixFrameworkUtilsTestSuite, testExtractEmbeddedBundle) {
    const char* testExtractDir = "extractEmbeddedBundleTestDir";
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    //invalid bundle symbol -> no extraction
    auto status = celix_framework_utils_extractBundle(framework->getCFramework(), "embedded://nonexisting", testExtractDir);
    EXPECT_NE(status, CELIX_SUCCESS);

    //valid bundle path -> extraction
    status = celix_framework_utils_extractBundle(framework->getCFramework(), "embedded://simple_test_bundle1", testExtractDir);
    EXPECT_EQ(status, CELIX_SUCCESS);
    checkBundleCacheDir(testExtractDir);
    celix_utils_deleteDirectory(testExtractDir, nullptr);
}

TEST_F(CelixFrameworkUtilsTestSuite, testListEmbeddedBundles) {
    auto list = celix::listEmbeddedBundles();
    EXPECT_EQ(2, list.size());
    //TODO check content
}

TEST_F(CelixFrameworkUtilsTestSuite, installEmbeddedBundles) {
    //TODO check installEmbeddedBundles
}