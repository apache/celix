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

#include "celix/FrameworkFactory.h"
#include "celix_constants.h"
#include "celix_file_utils.h"
#include "celix_framework_utils.h"
#include "framework.h"
#include "bundle_archive.h"

//including private headers, which should only be used for testing
#include "bundle_archive_private.h"
#include "celix_bundle_private.h"

class CxxBundleArchiveTestSuite : public ::testing::Test {
public:
};

/**
 * Test == operator to compare timespec values
 */
bool operator==(const timespec& lhs, const timespec& rhs) {
    return lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec == rhs.tv_nsec;
}

/**
 * Test != operator to compare timespec values
 */
bool operator!=(const timespec& lhs, const timespec& rhs) {
    return !(lhs == rhs);
}

TEST_F(CxxBundleArchiveTestSuite, BundleArchiveReusedTest) {
    auto fw = celix::createFramework({
         {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
         {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
    });
    auto ctx = fw->getFrameworkBundleContext();

    std::mutex m; //protects installTime
    timespec installTime{};

    auto tracker = ctx->trackBundles()
        .addOnInstallCallback([&](const celix::Bundle& b) {
            std::lock_guard<std::mutex> lock{m};
            auto *archive = celix_bundle_getArchive(b.getCBundle());
            EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &installTime));
        }).build();

    long bndId1 = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId1, -1);

    std::unique_lock<std::mutex> lock{m};
    EXPECT_GT(installTime.tv_sec, 0);
    auto firstBundleRevisionTime = installTime;
    lock.unlock();

    tracker.reset();
    fw = celix::createFramework({
                                        {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                                        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "false"}
                                });
    ctx = fw->getFrameworkBundleContext();
    tracker = ctx->trackBundles()
            .addOnInstallCallback([&](const celix::Bundle& b) {
                std::lock_guard<std::mutex> lock{m};
                auto *archive = celix_bundle_getArchive(b.getCBundle());
                EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &installTime));
            }).build();
    std::this_thread::sleep_for(std::chrono::milliseconds{100}); //wait so that the zip <-> archive dir modification time is different
    long bndId2 = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId2, -1);
    EXPECT_EQ(bndId1, bndId2); //bundle id should be reused.

    lock.lock();
    EXPECT_GT(installTime.tv_sec, 0);
    //bundle archive should not be updated
    EXPECT_EQ(installTime, firstBundleRevisionTime);
    lock.unlock();


    auto secondBundleRevisionTime = installTime;
    tracker.reset();
    fw = celix::createFramework({
                                        {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                                        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "false"}
                                });
    ctx = fw->getFrameworkBundleContext();
    tracker = ctx->trackBundles()
            .addOnInstallCallback([&](const celix::Bundle& b) {
                std::lock_guard<std::mutex> lock{m};
                auto *archive = celix_bundle_getArchive(b.getCBundle());
                EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &installTime));
            }).build();
    std::this_thread::sleep_for(std::chrono::milliseconds{100}); //wait so that the zip <-> archive dir modification time is different
    celix_utils_touch(SIMPLE_TEST_BUNDLE1_LOCATION); //touch the bundle zip file to force an update
    long bndId3 = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId3, -1);
    EXPECT_EQ(bndId1, bndId3); //bundle id should be reused.

    lock.lock();
    EXPECT_GT(installTime.tv_sec, 0);
    //bundle archive should be updated, because the zip file is touched
    EXPECT_NE(installTime, secondBundleRevisionTime);
    lock.unlock();
}

TEST_F(CxxBundleArchiveTestSuite, BundleArchiveUpdatedAfterCleanOnCreateTest) {
    auto fw = celix::createFramework({
        {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
    });
    auto ctx = fw->getFrameworkBundleContext();

    std::mutex m; //protects installTimes
    timespec firstInstallTime{};

    auto tracker = ctx->trackBundles()
            .addOnInstallCallback([&](const celix::Bundle& b) {
                std::lock_guard<std::mutex> lock{m};
                auto *archive = celix_bundle_getArchive(b.getCBundle());
                EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &firstInstallTime));
            }).build();

    long bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId, -1);

    std::unique_lock<std::mutex> lock{m};
    EXPECT_GT(firstInstallTime.tv_sec, 0);
    lock.unlock();

    //remove framework and create again with true on clean cache dir on create
    tracker.reset();
    ctx.reset();
    fw.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds{100}); //wait so that the zip <-> archive dir modification time is different

    timespec secondInstallTime{};


    fw = celix::createFramework({
        {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
    });
    ctx = fw->getFrameworkBundleContext();
    tracker = ctx->trackBundles()
            .addOnInstallCallback([&](const celix::Bundle& b) {
                std::lock_guard<std::mutex> lock{m};
                auto *archive = celix_bundle_getArchive(b.getCBundle());
                EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &secondInstallTime));
            }).build();

    bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId, -1);

    lock.lock();
    EXPECT_GT(secondInstallTime.tv_sec, 0);
    //bundle archive should be updated
    EXPECT_NE(secondInstallTime, firstInstallTime);
    lock.unlock();
}

TEST_F(CxxBundleArchiveTestSuite, BundleArchiveReusedIfNoCleanOnCreateTest) {
    auto fw = celix::createFramework({
        {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"}
    });
    auto ctx = fw->getFrameworkBundleContext();

    std::mutex m; //protects installTimes
    timespec firstInstallTime{};

    auto tracker = ctx->trackBundles()
            .addOnInstallCallback([&](const celix::Bundle& b) {
                std::lock_guard<std::mutex> lock{m};
                auto *archive = celix_bundle_getArchive(b.getCBundle());
                EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &firstInstallTime));
            }).build();

    long bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId, -1);

    std::unique_lock<std::mutex> lock{m};
    EXPECT_GT(firstInstallTime.tv_sec, 0);
    lock.unlock();

    //remove framework and create again with false on clean cache dir on create
    tracker.reset();
    ctx.reset();
    fw.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds{100}); //wait so that the zip <-> archive dir modification time is different

    timespec secondInstallTime{};

    fw = celix::createFramework({
        {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "false"}
    });
    ctx = fw->getFrameworkBundleContext();
    tracker = ctx->trackBundles()
            .addOnInstallCallback([&](const celix::Bundle& b) {
                std::lock_guard<std::mutex> lock{m};
                auto *archive = celix_bundle_getArchive(b.getCBundle());
                EXPECT_EQ(CELIX_SUCCESS, celix_bundleArchive_getLastModified(archive, &secondInstallTime));
            }).build();

    bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GT(bndId, -1);

    lock.lock();
    EXPECT_GT(secondInstallTime.tv_sec, 0);
    //bundle archive should be reused
    EXPECT_EQ(secondInstallTime, firstInstallTime);
    lock.unlock();
}

TEST_F(CxxBundleArchiveTestSuite, BundleArchivesCreatedBeforeStarting) {
    //Given a properties set with 1 bundle configured for start and 1 bundle configured for install
    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_AUTO_START_0, SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_properties_set(props, CELIX_AUTO_INSTALL, SIMPLE_TEST_BUNDLE2_LOCATION);

    //When the framework is created
    celix_framework_t* cFw = nullptr;
    auto status = framework_create(&cFw, props);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Then the celix_framework_utils_createBundleArchivesCache should extract the bundle archives for the
    //configured bundles
    status = celix_framework_utils_createBundleArchivesCache(cFw);
    EXPECT_EQ(status, CELIX_SUCCESS);
    framework_destroy(cFw);

    //When I create a new framework, but with no properties
    auto fw = celix::createFramework({{CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "false"}});
    //Then there a no bundles installed (because there is no start, install config
    EXPECT_EQ(fw->getFrameworkBundleContext()->listBundleIds().size(), 0);

    //But when I manually install a new bundle
    auto bndId = fw->getFrameworkBundleContext()->installBundle(SIMPLE_TEST_BUNDLE3_LOCATION);
    //Then  the bundle id will be 3, because there are already 2 bundles
    //in the bundle archive which uses the bundle ids 1 and 2.
    EXPECT_EQ(bndId, 3); // <-- note whitebox knowledge of the bundle id

    //When I install SIMPLE_TEST_BUNDLE2_LOCATION again
    bndId = fw->getFrameworkBundleContext()->installBundle(SIMPLE_TEST_BUNDLE2_LOCATION);
    //Then the bundle id will be 2, because the bundle archive is already created
    EXPECT_EQ(bndId, 2); // <-- note whitebox knowledge of the bundle id

    //When I install SIMPLE_TEST_BUNDLE1_LOCATION again
    bndId = fw->getFrameworkBundleContext()->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    //Then the bundle id will be 1, because the bundle archive is already created
    EXPECT_EQ(bndId, 1); // <-- note whitebox knowledge of the bundle id
}
