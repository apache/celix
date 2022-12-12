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

#include "celix/FrameworkFactory.h"
#include "celix/FrameworkUtils.h"

#include "celix_framework_utils_private.h"
#include "celix_file_utils.h"

/**
 * Tests for the C and C++ framework utils functions which can be found in
 * - celix_framework_utils.h
 * - celix_framework_utils_private.h
 * - celix/FrameworkUtils.h (C++)
 */
class CelixFrameworkUtilsTestSuite : public ::testing::Test {
public:
    CelixFrameworkUtilsTestSuite() {
        framework = celix::createFramework({{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"}});
    }

    std::shared_ptr<celix::Framework> framework{};
};

static void checkBundleCacheDir(const char* extractDir) {
    ASSERT_TRUE(extractDir != nullptr);
    DIR* dir = opendir(extractDir);
    EXPECT_TRUE(dir != nullptr);
    if (dir != nullptr) {
        closedir(dir);
    }
}

TEST_F(CelixFrameworkUtilsTestSuite, testIsBundleUrlValid) {
    auto valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), "non-existing.zip", false);
    EXPECT_FALSE(valid);

    valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), "file://non-existing.zip", false);
    EXPECT_FALSE(valid);

    valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), "embedded://non_existing", false);
    EXPECT_FALSE(valid);

    valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), SIMPLE_TEST_BUNDLE1_LOCATION, false);
    EXPECT_TRUE(valid);

    auto url = std::string{"file://"} + SIMPLE_TEST_BUNDLE1_LOCATION;
    valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), url.c_str(), false);
    EXPECT_TRUE(valid);

    valid = celix_framework_utils_isBundleUrlValid(framework->getCFramework(), "embedded://simple_test_bundle1", false);
    EXPECT_TRUE(valid);
}

TEST_F(CelixFrameworkUtilsTestSuite, testExtractBundlePath) {
    const char* testExtractDir = "extractBundleTestDir";
    celix_utils_deleteDirectory(testExtractDir, nullptr);

    //invalid bundle url -> no extraction
    auto status = celix_framework_utils_extractBundle(framework->getCFramework(), nullptr, testExtractDir);
    EXPECT_NE(status, CELIX_SUCCESS);

    //invalid bundle path -> no extraction
    status = celix_framework_utils_extractBundle(framework->getCFramework(), "non-existing.zip", testExtractDir); //note nullptr framwork is allowed, fallback to global logger.
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
    ASSERT_EQ(2, list.size());
    EXPECT_EQ("embedded://simple_test_bundle1", list[0]);
    EXPECT_EQ("embedded://simple_test_bundle2", list[1]);
}

TEST_F(CelixFrameworkUtilsTestSuite, installEmbeddedBundles) {
    auto ids = framework->getFrameworkBundleContext()->listBundleIds();
    EXPECT_EQ(0, ids.size());

    celix::installEmbeddedBundles(*framework, true);
    ids = framework->getFrameworkBundleContext()->listBundleIds();
    EXPECT_EQ(2, ids.size());
}

TEST_F(CelixFrameworkUtilsTestSuite, installBundleSet) {
    auto ids = framework->getFrameworkBundleContext()->listBundleIds();
    EXPECT_EQ(0, ids.size());

    EXPECT_EQ(std::string{}, std::string{BUNDLE_EMPTY_TEST_SET});
    celix::installBundlesSet(*framework, BUNDLE_EMPTY_TEST_SET);
    ids = framework->getFrameworkBundleContext()->listBundleIds();
    EXPECT_EQ(0, ids.size());

    celix::installBundlesSet(*framework, BUNDLE_TEST_SET);
    ids = framework->getFrameworkBundleContext()->listBundleIds();
    EXPECT_EQ(3, ids.size());
}
