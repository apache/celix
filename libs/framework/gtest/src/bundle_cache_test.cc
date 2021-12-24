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

#include "celix_framework_factory.h"
#include "celix_bundle_cache.h"
#include "celix_file_utils.h"


class CelixBundleCache : public ::testing::Test {
public:
    CelixBundleCache() {
        auto* config = celix_properties_create();
        celix_properties_set(config, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace");

        auto* fw = celix_frameworkFactory_createFramework(config);
        framework = std::shared_ptr<celix_framework_t>{fw, [](celix_framework_t* cFw) {
            celix_frameworkFactory_destroyFramework(cFw);
        }};
    }

    std::shared_ptr<celix_framework_t> framework{};
};

static void checkBundleCacheDir(const char* bundleDir, const char* providedBundlePart) {
    const char* result = strstr(bundleDir, providedBundlePart);
    EXPECT_TRUE(result != nullptr);
    EXPECT_TRUE(std::filesystem::is_directory(bundleDir));
}

TEST_F(CelixBundleCache, testExtractBundlePath) {
    auto* dummyProps = celix_properties_create();
    auto* dummyFwUUID = "dummy";

    celix_bundle_cache_t* cache;
    auto status = celix_bundleCache_create(dummyFwUUID, dummyProps, &cache);
    ASSERT_EQ(status, CELIX_SUCCESS);

    //invalid bundle url -> no extraction
    char* result = celix_bundleCache_extractBundle(cache, ".bundle2/revision.0.0", nullptr);
    EXPECT_EQ(result, nullptr);

    //invalid bundle path -> no extraction
    result = celix_bundleCache_extractBundle(cache, ".bundle3/revision.0.0", "non-existing.zip");
    EXPECT_EQ(result, nullptr);

    //invalid url prefix -> no extraction
    std::string path = std::string{"bla://"} + SIMPLE_TEST_BUNDLE1_LOCATION;
    result = celix_bundleCache_extractBundle(cache, ".bundle4/revision.0.0", path.c_str());
    EXPECT_EQ(result, nullptr);

    //invalid url prefix -> no extraction
    path = std::string{"bla://"};
    result = celix_bundleCache_extractBundle(cache, ".bundle5/revision.0.0", path.c_str());
    EXPECT_EQ(result, nullptr);

    //invalid url prefix -> no extraction
    path = std::string{"file://"};
    result = celix_bundleCache_extractBundle(cache, ".bundle6/revision.0.0", path.c_str());
    EXPECT_EQ(result, nullptr);

    //valid bundle path -> extraction
    result = celix_bundleCache_extractBundle(cache, ".bundle7/revision.0.0", SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_NE(result, nullptr);
    checkBundleCacheDir(result, ".bundle7/revision.0.0");
    celix_utils_deleteDirectory(result, NULL);
    free(result);

    //valid bundle path with file:// prefix -> extraction
    path = std::string{"file://"} + SIMPLE_TEST_BUNDLE1_LOCATION;
    result = celix_bundleCache_extractBundle(cache, ".bundle8/revision.0.0", path.c_str());
    EXPECT_NE(result, nullptr);
    checkBundleCacheDir(result, ".bundle8/revision.0.0");
    celix_utils_deleteDirectory(result, NULL);
    free(result);
}

TEST_F(CelixBundleCache, testExtractEmbeddedBundle) {
    auto *dummyProps = celix_properties_create();
    auto *dummyFwUUID = "dummy";

    celix_bundle_cache_t *cache;
    auto status = celix_bundleCache_create(dummyFwUUID, dummyProps, &cache);
    ASSERT_EQ(status, CELIX_SUCCESS);

    //invalid bundle symbol -> no extraction
    auto result = celix_bundleCache_extractBundle(cache, ".bundle10/revision.0.0", "embedded://nonexisting");
    EXPECT_EQ(result, nullptr);

    //valid bundle path -> extraction
    result = celix_bundleCache_extractBundle(cache, ".bundle11/revision.0.0", "embedded://simple_test_bundle1");
    EXPECT_NE(result, nullptr);
    checkBundleCacheDir(result, ".bundle11/revision.0.0");
    celix_utils_deleteDirectory(result, NULL);
    free(result);
}