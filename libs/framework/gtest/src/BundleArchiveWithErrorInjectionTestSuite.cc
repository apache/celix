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

#include <thread>
#include <unistd.h>

#include "celix_constants.h"
#include "celix_file_utils.h"
#include "celix_framework_utils_private.h"
#include "celix_log.h"
#include "celix_properties_ei.h"
#include "celix_utils_ei.h"

#include "celix/FrameworkFactory.h"

#include "asprintf_ei.h"
#include "bundle_archive_private.h"
#include "bundle_revision_private.h"
#include "framework_private.h"
#include "malloc_ei.h"
#include "manifest.h"
#include "stat_ei.h"
#include "unistd_ei.h"

class BundleArchiveWithErrorInjectionTestSuite : public ::testing::Test {
  public:
    BundleArchiveWithErrorInjectionTestSuite() {
        fw = celix::createFramework({{"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                                     {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"},
                                     {CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, "false"}});
        ctx = fw->getFrameworkBundleContext();
    }

    ~BundleArchiveWithErrorInjectionTestSuite() override {
        teardownErrorInjectors();
        ctx.reset();
        fw.reset();
    }

    void teardownErrorInjectors() {
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_save(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_createDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_getLastModified(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_deleteDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_extractZipData(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_lstat(nullptr, 0, 0);
        celix_ei_expect_unlink(nullptr, 0, 0);
    }

    void installBundleAndExpectFailure() {
        // When I create bundle from a bundle zip by installing a bundle
        // Then the bundle install fails, and the bundle id is < 0
        long bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
        EXPECT_LT(bndId, 0);
    }

  protected:
    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

TEST_F(BundleArchiveWithErrorInjectionTestSuite, BundleArchiveCreatedFailedTest) {
    teardownErrorInjectors();
    // Given a mocked calloc which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_calloc((void*)celix_bundleArchive_create, 0, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 0, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 0, nullptr, 2);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    celix_ei_expect_asprintf((void*)celix_bundleArchive_create, 0, -1);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    celix_ei_expect_asprintf((void*)celix_bundleArchive_create, 0, -1, 2);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    celix_ei_expect_asprintf((void*)celix_bundleArchive_create, 0, -1, 3);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    // Given a mocked malloc which returns NULL from a call from manifest_create
    celix_ei_expect_malloc((void*)manifest_create, 0, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    // Given a mocked calloc which returns NULL from a call from bundleRevision_create
    celix_ei_expect_calloc((void*)celix_bundleRevision_create, 0, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    // Given a mocked celix_utils_strdup which returns NULL from a call from bundleRevision_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleRevision_create, 0, nullptr);
    installBundleAndExpectFailure();

    celix_ei_expect_celix_utils_strdup((void*)celix_bundleRevision_create, 0, nullptr, 2);
    installBundleAndExpectFailure();
}

TEST_F(BundleArchiveWithErrorInjectionTestSuite, BundleArchiveCreateCacheDirectoryFailedTest) {
    teardownErrorInjectors();
    // Given a mocked celix_utils_createDirectory which returns CELIX_FILE_IO_EXCEPTION from a (indirect) call from
    // bundleArchive_create
    celix_ei_expect_celix_utils_createDirectory((void*)celix_bundleArchive_create, 1, CELIX_FILE_IO_EXCEPTION);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    // Given a mocked celix_utils_createDirectory which returns CELIX_FILE_IO_EXCEPTION from a second (indirect) call
    //  from bundleArchive_create
    celix_ei_expect_celix_utils_createDirectory((void*)celix_bundleArchive_create, 1, CELIX_FILE_IO_EXCEPTION, 2);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    // Given a mocked celix_utils_strdup which returns NULL from a (indirect) call from bundleArchive_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 1, nullptr);
    installBundleAndExpectFailure();

    teardownErrorInjectors();
    // Given a mocked celix_utils_strdup which returns NULL from a second (indirect) call from bundleArchive_create
    celix_ei_expect_celix_utils_strdup((void*)celix_bundleArchive_create, 1, nullptr, 2);
    installBundleAndExpectFailure();
}

constexpr const char* const TEST_ARCHIVE_ROOT = ".test_archive_root";

class CelixBundleArchiveErrorInjectionTestSuite : public ::testing::Test {
  public:
    CelixBundleArchiveErrorInjectionTestSuite() {
        fw.configurationMap = celix_properties_create();
        fw.logger = celix_frameworkLogger_create(CELIX_LOG_LEVEL_TRACE);
    }
    ~CelixBundleArchiveErrorInjectionTestSuite() override {
        celix_utils_deleteDirectory(TEST_ARCHIVE_ROOT, nullptr);
        celix_frameworkLogger_destroy(fw.logger);
        celix_properties_destroy(fw.configurationMap);
    }
    void createCache(celix_bundle_cache_t** cache) {
        celix_properties_setBool(fw.configurationMap, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
        EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_create(&fw, &fw.cache));
        *cache = fw.cache;
    }
    void teardownErrorInjectors() {
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_asprintf(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_createDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_getLastModified(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_deleteDirectory(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_extractZipData(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_extractZipFile(nullptr, 0, CELIX_SUCCESS);
    }
    struct celix_framework fw {};
};

TEST_F(CelixBundleArchiveErrorInjectionTestSuite, ArchiveCreateErrorTest) {
    celix_bundle_cache_t* cache = nullptr;
    createCache(&cache);
    bundle_archive_t* archive = nullptr;

    // archive directory creation failures not covered by other tests
    celix_ei_expect_celix_utils_getLastModified((void*)celix_bundleArchive_create, 2, CELIX_FILE_IO_EXCEPTION);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_FALSE(celix_utils_directoryExists(TEST_ARCHIVE_ROOT));
    teardownErrorInjectors();

    celix_ei_expect_lstat((void*) celix_bundleArchive_create, 3, -1);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES),
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_FALSE(celix_utils_directoryExists(TEST_ARCHIVE_ROOT));
    teardownErrorInjectors();

    const char* testExtractDir = "extractBundleTestDir";
    EXPECT_EQ(CELIX_SUCCESS, celix_utils_extractZipFile(SIMPLE_TEST_BUNDLE1_LOCATION, testExtractDir, nullptr));
    EXPECT_EQ(CELIX_SUCCESS,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 2, testExtractDir, &archive));
    bundleArchive_destroy(archive);
    archive = nullptr;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    celix_utils_touch(SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_ei_expect_unlink((void*)celix_bundleArchive_create, 3, -1);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES),
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 2, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    celix_utils_deleteDirectory(testExtractDir, nullptr);
    teardownErrorInjectors();

    EXPECT_EQ(CELIX_SUCCESS,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    bundleArchive_destroy(archive);
    archive = nullptr;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    celix_utils_touch(SIMPLE_TEST_BUNDLE1_LOCATION);
    celix_ei_expect_celix_utils_deleteDirectory((void*)celix_bundleArchive_create, 3, CELIX_FILE_IO_EXCEPTION);
    EXPECT_EQ(CELIX_FILE_IO_EXCEPTION,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_FALSE(celix_utils_directoryExists(TEST_ARCHIVE_ROOT));
    teardownErrorInjectors();

    celix_ei_expect_celix_utils_writeOrCreateString((void*)celix_bundleArchive_create, 1, nullptr);
    EXPECT_EQ(CELIX_ENOMEM,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_FALSE(celix_utils_directoryExists(TEST_ARCHIVE_ROOT));
    teardownErrorInjectors();

    // revision creation failure
    celix_ei_expect_calloc((void*)celix_bundleRevision_create, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_FALSE(celix_utils_directoryExists(TEST_ARCHIVE_ROOT));
    teardownErrorInjectors();

    // bundle state persistence failure
    celix_ei_expect_celix_properties_create((void*)celix_bundleArchive_create, 1, nullptr);
    EXPECT_EQ(CELIX_ENOMEM,
              celix_bundleArchive_create(&fw, TEST_ARCHIVE_ROOT, 1, SIMPLE_TEST_BUNDLE1_LOCATION, &archive));
    EXPECT_EQ(nullptr, archive);
    EXPECT_FALSE(celix_utils_directoryExists(TEST_ARCHIVE_ROOT));
    teardownErrorInjectors();

    EXPECT_EQ(CELIX_SUCCESS, celix_bundleCache_destroy(cache));
}

TEST_F(CelixBundleArchiveErrorInjectionTestSuite, StoreBundleStatePropertiesErrorTest) {
    // Given a framework
    auto fw = celix::createFramework({
        {CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true"},
    });
    auto ctx = fw->getFrameworkBundleContext();

    // When an error is prepped for celix_properties_save
    celix_ei_expect_celix_properties_save((void*)celix_bundleCache_findBundleIdForLocation, 1, CELIX_FILE_IO_EXCEPTION);

    // When the bundle install, a bundle id is returned (async bundle install)
    long bndId = ctx->installBundle(SIMPLE_TEST_BUNDLE1_LOCATION);
    EXPECT_GE(bndId, 0);

    // Then the bundle is not successfully installed

    celix_bundleContext_useBundle(
        ctx->getCBundleContext(), bndId, nullptr, [](void* /*handle*/, const celix_bundle_t* bnd) {
            auto status = celix_bundle_getState(bnd);
            // TODO fixme, bundle is installed and active, this is not correct
            EXPECT_EQ(CELIX_BUNDLE_EVENT_INSTALLED, status);
        });
}
