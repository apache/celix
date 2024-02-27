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

#include <atomic>
#include <celix_log_utils.h>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include "bundle_context_private.h"
#include "celix_api.h"
#include "celix_bundle.h"
#include "celix_bundle_context.h"
#include "celix_stdlib_cleanup.h"
#include "celix_file_utils.h"

class CelixBundleContextBundlesTestSuite : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    const char * const TEST_BND1_LOC = "" SIMPLE_TEST_BUNDLE1_LOCATION "";
    const char * const TEST_BND2_LOC = "" SIMPLE_TEST_BUNDLE2_LOCATION "";
    const char * const TEST_BND3_LOC = "" SIMPLE_TEST_BUNDLE3_LOCATION "";
    const char * const TEST_BND4_LOC = "" SIMPLE_TEST_BUNDLE4_LOCATION "";
    const char * const TEST_BND5_LOC = "" SIMPLE_TEST_BUNDLE5_LOCATION "";
    const char * const TEST_BND_WITH_EXCEPTION_LOC = "" TEST_BUNDLE_WITH_EXCEPTION_LOCATION "";
    const char * const TEST_BND_UNRESOLVABLE_LOC = "" TEST_BUNDLE_UNRESOLVABLE_LOCATION "";

    CelixBundleContextBundlesTestSuite() {
        properties = celix_properties_create();
        celix_properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        celix_properties_set(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");
        celix_properties_set(properties, CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED, "false");


        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);
    }
    
    ~CelixBundleContextBundlesTestSuite() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    CelixBundleContextBundlesTestSuite(CelixBundleContextBundlesTestSuite&&) = delete;
    CelixBundleContextBundlesTestSuite(const CelixBundleContextBundlesTestSuite&) = delete;
    CelixBundleContextBundlesTestSuite& operator=(CelixBundleContextBundlesTestSuite&&) = delete;
    CelixBundleContextBundlesTestSuite& operator=(const CelixBundleContextBundlesTestSuite&) = delete;
};

TEST_F(CelixBundleContextBundlesTestSuite, StartStopTest) {
    //nop
}

TEST_F(CelixBundleContextBundlesTestSuite, InstallABundleTest) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId >= 0);
}

//TEST_F(CelixBundleContextBundlesTestSuite, InstallBundleWithBadExport) {
//    long bndId = celix_bundleContext_installBundle(ctx, BUNDLE_WITH_BAD_EXPORT_LOCATION, true);
//    ASSERT_TRUE(bndId >= 0);
//}

TEST_F(CelixBundleContextBundlesTestSuite, InstallBundlesTest) {
    long bndId = celix_bundleContext_installBundle(ctx, "non-existing.zip", true);
    ASSERT_TRUE(bndId < 0);

    bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId >= 0);

    bndId = celix_bundleContext_installBundle(ctx, TEST_BND4_LOC, true); //not loaded in subdir
    ASSERT_TRUE(bndId < 0);

    setenv(CELIX_BUNDLES_PATH_NAME, "subdir", true);
    bndId = celix_bundleContext_installBundle(ctx, TEST_BND4_LOC, true); //subdir now part of CELIX_BUNDLES_PATH
    unsetenv("subdir");
    ASSERT_TRUE(bndId >= 0);
}

TEST_F(CelixBundleContextBundlesTestSuite, TestIsSystemBundle) {
    celix_bundle_t* fwBnd = celix_framework_getFrameworkBundle(fw);
    ASSERT_TRUE(celix_bundle_isSystemBundle(fwBnd));

    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId >= 0);
    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void*, const celix_bundle_t* bnd) {
        EXPECT_FALSE(celix_bundle_isSystemBundle(bnd));
    });
    EXPECT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesTestSuite, UseBundlesTest) {
    int callbackCount = 0;

    auto use = [](void *handle, const bundle_t *bnd) {
        int *c = (int*)handle;
        *c += 1;
        long id = celix_bundle_getId(bnd);
        ASSERT_TRUE(id >= 0);

        const auto* v = celix_bundle_getVersion(bnd);
        ASSERT_TRUE(v != nullptr);
        ASSERT_EQ(celix_version_getMajor(v), 1);
    };

    celix_bundleContext_useBundles(ctx, &callbackCount, use);

    callbackCount = 0;
    celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    size_t useCount = celix_bundleContext_useBundles(ctx, &callbackCount, use);
    EXPECT_EQ(1, callbackCount);
    EXPECT_EQ(1, useCount);

    callbackCount = 0;
    celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false); //note installed, but not started
    useCount = celix_bundleContext_useBundles(ctx, &callbackCount, use);
    EXPECT_EQ(2, callbackCount);
    EXPECT_EQ(2, useCount);
};

TEST_F(CelixBundleContextBundlesTestSuite, InstallAndUninstallBundlesTest) {
    //install bundles
    long bndId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false);
    long bndId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);

    ASSERT_TRUE(bndId1 >= 0L);
    ASSERT_TRUE(bndId2 >= 0L);
    ASSERT_TRUE(bndId3 >= 0L);

    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId2));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId3));

    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId1));
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId2)); //not auto started
    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId3));

    char *bndRoot1 = nullptr;
    ASSERT_TRUE(celix_bundleContext_useBundle(ctx, bndId1, &bndRoot1, [](void* handle, const celix_bundle_t* bnd) {
        char **root = static_cast<char **>(handle);
        *root = celix_bundle_getEntry(bnd, "/");
    }));
    ASSERT_TRUE(bndRoot1 != nullptr);
    char* bndRoot2 = nullptr;
    ASSERT_TRUE(celix_bundleContext_useBundle(ctx, bndId2, &bndRoot2, [](void* handle, const celix_bundle_t* bnd) {
        char **root = static_cast<char **>(handle);
        *root = celix_bundle_getEntry(bnd, "/");
    }));
    ASSERT_TRUE(bndRoot2 != nullptr);
    char* bndRoot3 = nullptr;
    ASSERT_TRUE(celix_bundleContext_useBundle(ctx, bndId3, &bndRoot3, [](void* handle, const celix_bundle_t* bnd) {
        char **root = static_cast<char **>(handle);
        *root = celix_bundle_getEntry(bnd, "/");
    }));
    ASSERT_TRUE(bndRoot3 != nullptr);

    ASSERT_TRUE(celix_utils_directoryExists(bndRoot1));
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot2));
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot3));

    //uninstall bundles
    ASSERT_TRUE(celix_bundleContext_uninstallBundle(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_uninstallBundle(ctx, bndId2));
    ASSERT_TRUE(celix_bundleContext_uninstallBundle(ctx, bndId3));

    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, bndId2));
    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, bndId3));

    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId1)); //not uninstall -> not active
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId2));
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId3));

    ASSERT_FALSE(celix_utils_directoryExists(bndRoot1));
    ASSERT_FALSE(celix_utils_directoryExists(bndRoot2));
    ASSERT_FALSE(celix_utils_directoryExists(bndRoot3));

    free(bndRoot1);
    free(bndRoot2);
    free(bndRoot3);

    //reinstall bundles
    long bndId4 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId5 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false);
    long bndId6 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);

    ASSERT_TRUE(bndId4 >= 0L);
    ASSERT_FALSE(bndId1 == bndId4);
    ASSERT_TRUE(bndId5 >= 0L);
    ASSERT_FALSE(bndId2 == bndId5);
    ASSERT_TRUE(bndId6 >= 0L);
    ASSERT_FALSE(bndId3 == bndId6);
}

TEST_F(CelixBundleContextBundlesTestSuite, InstallAndUnloadBundlesTest) {
    //install bundles
    long bndId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false);
    long bndId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);

    ASSERT_TRUE(bndId1 >= 0L);
    ASSERT_TRUE(bndId2 >= 0L);
    ASSERT_TRUE(bndId3 >= 0L);

    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId2));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId3));

    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId1));
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId2)); //not auto started
    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId3));

    char *bndRoot1 = nullptr;
    ASSERT_TRUE(celix_bundleContext_useBundle(ctx, bndId1, &bndRoot1, [](void* handle, const celix_bundle_t* bnd) {
      char **root = static_cast<char **>(handle);
      *root = celix_bundle_getEntry(bnd, "/");
    }));
    ASSERT_TRUE(bndRoot1 != nullptr);
    char* bndRoot2 = nullptr;
    ASSERT_TRUE(celix_bundleContext_useBundle(ctx, bndId2, &bndRoot2, [](void* handle, const celix_bundle_t* bnd) {
      char **root = static_cast<char **>(handle);
      *root = celix_bundle_getEntry(bnd, "/");
    }));
    ASSERT_TRUE(bndRoot2 != nullptr);
    char* bndRoot3 = nullptr;
    ASSERT_TRUE(celix_bundleContext_useBundle(ctx, bndId3, &bndRoot3, [](void* handle, const celix_bundle_t* bnd) {
      char **root = static_cast<char **>(handle);
      *root = celix_bundle_getEntry(bnd, "/");
    }));
    ASSERT_TRUE(bndRoot3 != nullptr);

    ASSERT_TRUE(celix_utils_directoryExists(bndRoot1));
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot2));
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot3));

    //unload bundles
    ASSERT_TRUE(celix_bundleContext_unloadBundle(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_unloadBundle(ctx, bndId2));
    ASSERT_TRUE(celix_bundleContext_unloadBundle(ctx, bndId3));

    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, bndId2));
    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, bndId3));

    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId1)); //not uninstall -> not active
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId2));
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId3));

    // bundle cache is NOT cleaned up
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot1));
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot2));
    ASSERT_TRUE(celix_utils_directoryExists(bndRoot3));

    free(bndRoot1);
    free(bndRoot2);
    free(bndRoot3);

    //reinstall bundles
    long bndId4 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId5 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false);
    long bndId6 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);

    ASSERT_TRUE(bndId4 >= 0L);
    ASSERT_TRUE(bndId1 == bndId4); //bundle cache -> reuse of bundle id.
    ASSERT_TRUE(bndId5 >= 0L);
    ASSERT_TRUE(bndId2 == bndId5); //bundle cache -> reuse of bundle id.
    ASSERT_TRUE(bndId6 >= 0L);
    ASSERT_TRUE(bndId3 == bndId6); //bundle cache -> reuse of bundle id.
}

TEST_F(CelixBundleContextBundlesTestSuite, UpdateBundlesTest) {
    long bndId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId1 >= 0L);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId1));

    ASSERT_TRUE(celix_bundleContext_updateBundle(ctx, bndId1, NULL));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId1));

    ASSERT_TRUE(celix_bundleContext_stopBundle(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_updateBundle(ctx, bndId1, NULL));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_FALSE(celix_bundleContext_isBundleActive(ctx, bndId1));

    long bndId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false);
    ASSERT_TRUE(bndId2 >= 0L);
    ASSERT_FALSE(celix_bundleContext_updateBundle(ctx, bndId1, TEST_BND2_LOC));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));

    ASSERT_TRUE(celix_bundleContext_unloadBundle(ctx, bndId1));
    ASSERT_FALSE(celix_bundleContext_updateBundle(ctx, bndId1, NULL));
}

TEST_F(CelixBundleContextBundlesTestSuite, ForceUpdateUsingBundleFromDifferentLocation) {
    long bndId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId1 >= 0L);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId1));
    celix_autofree char* name1 = celix_bundleContext_getBundleSymbolicName(ctx, bndId1);
    EXPECT_TRUE(celix_bundleContext_useBundle(ctx, bndId1, nullptr, [] (void *, const bundle_t *bundle) {
        // make bundle cache root newer than the bundle at location TEST_BND2_LOC
        celix_autofree char* root = celix_bundle_getEntry(bundle, "/");
        celix_utils_touch(root);
    }));
    ASSERT_TRUE(celix_bundleContext_updateBundle(ctx, bndId1, TEST_BND2_LOC));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    celix_autofree char* name2 = celix_bundleContext_getBundleSymbolicName(ctx, bndId1);
    // bundle cache contains the bundle at location TEST_BND2_LOC
    ASSERT_STRNE(name1, name2);
}

TEST_F(CelixBundleContextBundlesTestSuite, UpdateCorruptUncompressedBundlesTest) {
    const char* testExtractDir1 = "extractBundleTestDir1";
    const char* testExtractDir2 = "extractBundleTestDir2";
    celix_utils_deleteDirectory(testExtractDir1, nullptr);
    EXPECT_EQ(CELIX_SUCCESS, celix_utils_extractZipFile(TEST_BND1_LOC, testExtractDir1, nullptr));
    long bndId1 = celix_bundleContext_installBundle(ctx, testExtractDir1, true);
    ASSERT_TRUE(bndId1 >= 0L);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleActive(ctx, bndId1));

    // make the symbolic link in cache dangling
    celix_utils_deleteDirectory(testExtractDir1, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    EXPECT_EQ(CELIX_SUCCESS, celix_utils_extractZipFile(TEST_BND2_LOC, testExtractDir2, nullptr));
    ASSERT_TRUE(celix_bundleContext_updateBundle(ctx, bndId1, testExtractDir2));

    celix_utils_deleteDirectory(testExtractDir1, nullptr);
    celix_utils_deleteDirectory(testExtractDir2, nullptr);
}

TEST_F(CelixBundleContextBundlesTestSuite, StartBundleWithException) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND_WITH_EXCEPTION_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not started

    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void */*handle*/, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(state, CELIX_BUNDLE_STATE_RESOLVED);
        ASSERT_EQ(state, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
    });
    ASSERT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesTestSuite, StartUnresolveableBundle) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND_UNRESOLVABLE_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not resolved

    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(CELIX_BUNDLE_STATE_INSTALLED, state);
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_INSTALLED, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);

    celix_bundleContext_startBundle(ctx, bndId);

    called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(CELIX_BUNDLE_STATE_INSTALLED, state);
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_INSTALLED, state); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);
}


TEST_F(CelixBundleContextBundlesTestSuite, UseBundleTest) {
    int count = 0;

    bool called = celix_bundleContext_useBundle(ctx, 0, &count, [](void *handle, const bundle_t *bnd) {
        int *c = (int*)handle;
        *c += 1;
        long id = celix_bundle_getId(bnd);
        ASSERT_EQ(0, id);
    });

    ASSERT_TRUE(called);
    ASSERT_EQ(1, count);
};

TEST_F(CelixBundleContextBundlesTestSuite, StopStartTest) {
    long bndId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    long bndId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId2));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId3));
    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, 600 /*non existing*/));


    celix_array_list_t *ids = celix_bundleContext_listInstalledBundles(ctx);
    size_t size = celix_arrayList_size(ids);
    EXPECT_EQ(3, size);
    celix_arrayList_destroy(ids);

    ids = celix_bundleContext_listBundles(ctx);
    size = celix_arrayList_size(ids);
    EXPECT_EQ(3, size);

    int count = 0;
    celix_bundleContext_useBundles(ctx, &count, [](void *handle, const celix_bundle_t *bnd) {
        auto *c = (int*)handle;
        ASSERT_EQ(CELIX_BUNDLE_STATE_ACTIVE, celix_bundle_getState(bnd));
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is the same.
        *c += 1;
    });
    EXPECT_EQ(3, count);


    for (size_t i = 0; i < size; ++i) {
        bool stopped = celix_bundleContext_stopBundle(ctx, celix_arrayList_getLong(ids, (int)i));
        EXPECT_TRUE(stopped);
    }

    bool stopped = celix_bundleContext_stopBundle(ctx, 42 /*non existing*/);
    EXPECT_FALSE(stopped);

    bool started = celix_bundleContext_startBundle(ctx, 42 /*non existing*/);
    EXPECT_FALSE(started);

    for (size_t i = 0; i < size; ++i) {
        started = celix_bundleContext_startBundle(ctx, celix_arrayList_getLong(ids, (int)i));
        EXPECT_TRUE(started);
    }

    count = 0;
    celix_bundleContext_useBundles(ctx, &count, [](void *handle, const celix_bundle_t *bnd) {
        auto *c = (int*)handle;
        EXPECT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
        *c += 1;
    });
    EXPECT_EQ(3, count);

    celix_arrayList_destroy(ids);
}

TEST_F(CelixBundleContextBundlesTestSuite, DoubleStopTest) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId > 0);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId));

    bool called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(CELIX_BUNDLE_STATE_ACTIVE, celix_bundle_getState(bnd));
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);

    //first
    celix_bundleContext_stopBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(CELIX_BUNDLE_STATE_RESOLVED, celix_bundle_getState(bnd));
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_RESOLVED, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);

    //second
    celix_bundleContext_stopBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(CELIX_BUNDLE_STATE_RESOLVED, celix_bundle_getState(bnd));
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_RESOLVED, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);

    //first
    celix_bundleContext_startBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(CELIX_BUNDLE_STATE_ACTIVE, celix_bundle_getState(bnd));
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);

    //second
    celix_bundleContext_startBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(CELIX_BUNDLE_STATE_ACTIVE, celix_bundle_getState(bnd));\
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd)); //NOTE the OSGI_ variant is equivalent
    });
    ASSERT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesTestSuite, TrackBundlesTest) {
    struct data {
        std::atomic<int> installedCount{0};
        std::atomic<int> startedCount{0};
        std::atomic<int> stoppedCount{0};
    };
    struct data data;

    auto installed = [](void *handle, const bundle_t *bnd) {
        auto *d = static_cast<struct data*>(handle);
        EXPECT_TRUE(bnd != nullptr);
        d->installedCount.fetch_add(1);
    };

    auto started = [](void *handle, const bundle_t *bnd) {
        auto *d = static_cast<struct data*>(handle);
        EXPECT_TRUE(bnd != nullptr);
        d->startedCount.fetch_add(1);
    };

    auto stopped = [](void *handle, const bundle_t *bnd) {
        auto *d = static_cast<struct data*>(handle);
        if (bnd == nullptr) {
            celix_logUtils_logToStdout("test", CELIX_LOG_LEVEL_ERROR, "bnd should not be null");
        }
        EXPECT_TRUE(bnd != nullptr);
        d->stoppedCount.fetch_add(1);
    };

    celix_bundle_tracking_options_t opts{};
    opts.callbackHandle = static_cast<void*>(&data);
    opts.onInstalled = installed;
    opts.onStarted = started;
    opts.onStopped = stopped;

    long bundleId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId1 >= 0);

    /*
     * NOTE for bundles already installed (TEST_BND1) the callbacks are called on the
     * thread of celix_bundleContext_trackBundlesWithOptions.
     * For Bundles installed after the celix_bundleContext_trackBundlesWithOptions function
     * the called are called on the Celix framework event queue thread.
     */
    long trackerId = celix_bundleContext_trackBundlesWithOptions(ctx, &opts);
    EXPECT_EQ(1, data.installedCount.load());
    EXPECT_EQ(1, data.startedCount.load());
    EXPECT_EQ(0, data.stoppedCount.load());


    long bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId2 >= 0);
    EXPECT_EQ(2, data.installedCount.load());
    EXPECT_EQ(2, data.startedCount.load());
    EXPECT_EQ(0, data.stoppedCount.load());

    celix_bundleContext_uninstallBundle(ctx, bundleId2);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_EQ(2, data.installedCount.load());
    EXPECT_EQ(2, data.startedCount.load());
    EXPECT_EQ(1, data.stoppedCount.load());

    long bundleId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId3 >= 0);
    EXPECT_EQ(3, data.installedCount.load());
    EXPECT_EQ(3, data.startedCount.load());
    EXPECT_EQ(1, data.stoppedCount.load());

    bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId2 >= 0);
    EXPECT_EQ(4, data.installedCount.load());
    EXPECT_EQ(4, data.startedCount.load());
    EXPECT_EQ(1, data.stoppedCount.load());

    celix_bundleContext_stopTracker(ctx, trackerId);
};


TEST_F(CelixBundleContextBundlesTestSuite, TrackBundlesTestAsync) {
    struct data {
        std::atomic<int> installedCount{0};
        std::atomic<int> startedCount{0};
        std::atomic<int> stoppedCount{0};
    };
    struct data data;

    auto installed = [](void *handle, const bundle_t *bnd) {
        auto *d = static_cast<struct data*>(handle);
        EXPECT_TRUE(bnd != nullptr);
        d->installedCount.fetch_add(1);
    };

    auto started = [](void *handle, const bundle_t *bnd) {
        auto *d = static_cast<struct data*>(handle);
        EXPECT_TRUE(bnd != nullptr);
        d->startedCount.fetch_add(1);
    };

    auto stopped = [](void *handle, const bundle_t *bnd) {
        auto *d = static_cast<struct data*>(handle);
        if (bnd == nullptr) {
            celix_logUtils_logToStdout("test", CELIX_LOG_LEVEL_ERROR, "bnd should not be null");
        }
        EXPECT_TRUE(bnd != nullptr);
        d->stoppedCount.fetch_add(1);
    };

    celix_bundle_tracking_options_t opts{};
    opts.callbackHandle = static_cast<void*>(&data);
    opts.onInstalled = installed;
    opts.onStarted = started;
    opts.onStopped = stopped;

    long bundleId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId1 >= 0);

    /*
     * NOTE for bundles already installed (TEST_BND1) the callbacks are called on the
     * thread of celix_bundleContext_trackBundlesWithOptions.
     * For Bundles installed after the celix_bundleContext_trackBundlesWithOptions function
     * the called are called on the Celix framework event queue thread.
     */
    long trackerId = celix_bundleContext_trackBundlesWithOptionsAsync(ctx, &opts);
    celix_bundleContext_waitForAsyncTracker(ctx, trackerId);
    EXPECT_EQ(1, data.installedCount.load());
    EXPECT_EQ(1, data.startedCount.load());
    EXPECT_EQ(0, data.stoppedCount.load());


    long bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId2 >= 0);
    EXPECT_EQ(2, data.installedCount.load());
    EXPECT_EQ(2, data.startedCount.load());
    EXPECT_EQ(0, data.stoppedCount.load());

    celix_bundleContext_uninstallBundle(ctx, bundleId2);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_EQ(2, data.installedCount.load());
    EXPECT_EQ(2, data.startedCount.load());
    EXPECT_EQ(1, data.stoppedCount.load());

    long bundleId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId3 >= 0);
    EXPECT_EQ(3, data.installedCount.load());
    EXPECT_EQ(3, data.startedCount.load());
    EXPECT_EQ(1, data.stoppedCount.load());

    bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    celix_framework_waitForEmptyEventQueue(fw);
    EXPECT_TRUE(bundleId2 >= 0);
    EXPECT_EQ(4, data.installedCount.load());
    EXPECT_EQ(4, data.startedCount.load());
    EXPECT_EQ(1, data.stoppedCount.load());

    celix_bundleContext_stopTrackerAsync(ctx, trackerId, nullptr, nullptr);
    celix_bundleContext_waitForAsyncStopTracker(ctx, trackerId);
};

TEST_F(CelixBundleContextBundlesTestSuite, useBundlesConcurrentTest) {

    struct data {
        std::mutex mutex{};
        std::condition_variable cond{};
        bool inUse{false};
        bool readyToExit{false};
    };
    struct data data{};

    auto use = [](void *handle, const bundle_t *bnd) {
        ASSERT_TRUE(bnd != nullptr);

        struct data *d = static_cast<struct data*>(handle);

        std::unique_lock<std::mutex> lock(d->mutex);
        d->inUse = true;
        d->cond.notify_all();
        d->cond.wait(lock, [d]{return d->readyToExit;});
        lock.unlock();
    };

    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);

    auto call = [&] {
        bool called = celix_bundleContext_useBundle(ctx, bndId, &data, use);
        ASSERT_TRUE(called);
    };
    std::thread useThread{call};


    std::thread uninstallThread{[&]{
        std::unique_lock<std::mutex> lock(data.mutex);
        data.cond.wait(lock, [&]{return data.inUse;});
        lock.unlock();
        std::cout << "trying to uninstall bundle ..." << std::endl;
        celix_bundleContext_uninstallBundle(ctx, bndId);
        std::cout << "done uninstalling bundle" << std::endl;
    }};


    //sleep 100 milli to give unregister a change to sink in
    std::cout << "before sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "after sleep" << std::endl;


    std::cout << "setting readyToExitUseCall and notify" << std::endl;
    std::unique_lock<std::mutex> lock(data.mutex);
    data.readyToExit = true;
    lock.unlock();
    data.cond.notify_all();

    useThread.join();
    std::cout << "use thread joined" << std::endl;
    uninstallThread.join();
    std::cout << "uninstall thread joined" << std::endl;
};

TEST_F(CelixBundleContextBundlesTestSuite, BundleInfoTests) {
    struct data {
        int provideCount{0};
        int requestedCount{0};
    };
    struct data data;

    void (*updateCountFp)(void *, const celix_bundle_t*) = [](void *handle, const celix_bundle_t *bnd) {
        auto *data = static_cast<struct data*>(handle);
        auto *trackers = celix_bundle_listServiceTrackers(bnd);
        auto *services = celix_bundle_listRegisteredServices(bnd);
        data->requestedCount = celix_arrayList_size(trackers);
        data->provideCount = celix_arrayList_size(services);
        celix_arrayList_destroy(trackers);
        celix_bundle_destroyRegisteredServicesList(services);
    };

    bool called = celix_bundleContext_useBundle(ctx, 0, &data, updateCountFp);
    EXPECT_TRUE(called);
    EXPECT_EQ(0, data.provideCount);
    EXPECT_EQ(0, data.requestedCount);


    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "NopService", NULL);
    long trackerId = celix_bundleContext_trackServices(ctx, "AService");

    called = celix_bundleContext_useBundle(ctx, 0, &data, updateCountFp);
    EXPECT_TRUE(called);
    EXPECT_EQ(1, data.provideCount);
    EXPECT_EQ(1, data.requestedCount);

    celix_bundleContext_unregisterService(ctx, svcId);
    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextBundlesTestSuite, StartStopBundleTrackerAsync) {
    std::atomic<int> count{0};

    auto cb = [](void* data) {
        auto* c = static_cast<std::atomic<int>*>(data);
        (*c)++;
    };

    celix_bundle_tracking_options_t opts{};
    opts.trackerCreatedCallbackData = &count;
    opts.trackerCreatedCallback = cb;
    long trkId = celix_bundleContext_trackBundlesWithOptionsAsync(ctx, &opts);
    EXPECT_GE(trkId, 0);
    celix_bundleContext_waitForAsyncTracker(ctx, trkId);
    EXPECT_EQ(count.load(), 1); //1x tracker started

    celix_bundleContext_stopTrackerAsync(ctx, trkId, &count, cb);
    celix_bundleContext_waitForAsyncStopTracker(ctx, trkId);
    EXPECT_EQ(2, count.load()); //1x tracker started, 1x tracker stopped
}

TEST_F(CelixBundleContextBundlesTestSuite, TestBundleStateToString) {
    const char* result = celix_bundleState_getName(CELIX_BUNDLE_STATE_UNKNOWN);
    EXPECT_STREQ(result, "UNKNOWN");

    result = celix_bundleState_getName(CELIX_BUNDLE_STATE_UNINSTALLED);
    EXPECT_STREQ(result, "UNINSTALLED");

    result = celix_bundleState_getName(CELIX_BUNDLE_STATE_INSTALLED);
    EXPECT_STREQ(result, "INSTALLED");

    result = celix_bundleState_getName(CELIX_BUNDLE_STATE_RESOLVED);
    EXPECT_STREQ(result, "RESOLVED");

    result = celix_bundleState_getName(CELIX_BUNDLE_STATE_STARTING);
    EXPECT_STREQ(result, "STARTING");

    result = celix_bundleState_getName(CELIX_BUNDLE_STATE_STOPPING);
    EXPECT_STREQ(result, "STOPPING");

    result = celix_bundleState_getName(CELIX_BUNDLE_STATE_ACTIVE);
    EXPECT_STREQ(result, "ACTIVE"); 

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_UNKNOWN);
    EXPECT_STREQ(result, "UNKNOWN");

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_UNINSTALLED);
    EXPECT_STREQ(result, "UNINSTALLED");

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_INSTALLED);
    EXPECT_STREQ(result, "INSTALLED");

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_RESOLVED);
    EXPECT_STREQ(result, "RESOLVED");

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_STARTING);
    EXPECT_STREQ(result, "STARTING");

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_STOPPING);
    EXPECT_STREQ(result, "STOPPING");

    result = celix_bundleState_getName(OSGI_FRAMEWORK_BUNDLE_ACTIVE);
    EXPECT_STREQ(result, "ACTIVE");

    result = celix_bundleState_getName((celix_bundle_state_e)444 /*invalid*/);
    EXPECT_STREQ(result, "UNKNOWN");
}

TEST_F(CelixBundleContextBundlesTestSuite, UsingDmFunctionsWithInstalledBundlesTest) {
    //Given a clean framework and a fw dependency manager
    auto* dm = celix_bundleContext_getDependencyManager(ctx);

    //Then all components are active (no bundles installed yet)
    bool allActive = celix_dependencyManager_allComponentsActive(dm);
    EXPECT_TRUE(allActive);

    //When installing a bundle
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, false);
    EXPECT_GE(bndId, 0);

    //Then all compnents are still active (bundle not started and no components in bundle) 
    allActive = celix_dependencyManager_allComponentsActive(dm);
    EXPECT_TRUE(allActive);

    //And a dm info can be received for the fw bundle
    auto* info = celix_dependencyManager_createInfo(dm, CELIX_FRAMEWORK_BUNDLE_ID);
    EXPECT_NE(info, nullptr);
    celix_dependencyManager_destroyInfo(dm, info);

    //But a valid dm info cannot be received for the installed bundle (not started yet)
    info = celix_dependencyManager_createInfo(dm, bndId);
    EXPECT_EQ(info, nullptr);
    celix_dependencyManager_destroyInfo(dm, info); //should be safe

    //And infos can be received and only contains dm info for 1 bundle (the framework bundle)
    auto* infos = celix_dependencyManager_createInfos(dm);
    EXPECT_NE(infos, nullptr);
    EXPECT_EQ(1, celix_arrayList_size(infos));
    celix_dependencyManager_destroyInfos(dm, infos);
}


class CelixBundleContextTempBundlesTestSuite : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    const char * const TEST_BND1_LOC = "" SIMPLE_TEST_BUNDLE1_LOCATION "";

    CelixBundleContextTempBundlesTestSuite() {
        properties = celix_properties_create();
        celix_properties_setBool(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", true);
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CACHE_USE_TMP_DIR, true);
        fw = celix_frameworkFactory_createFramework(properties);
        EXPECT_NE(fw, nullptr);
        ctx = framework_getContext(fw);
        EXPECT_NE(ctx, nullptr);
    }

    ~CelixBundleContextTempBundlesTestSuite() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    CelixBundleContextTempBundlesTestSuite(CelixBundleContextTempBundlesTestSuite&&) = delete;
    CelixBundleContextTempBundlesTestSuite(const CelixBundleContextTempBundlesTestSuite&) = delete;
    CelixBundleContextTempBundlesTestSuite& operator=(CelixBundleContextTempBundlesTestSuite&&) = delete;
    CelixBundleContextTempBundlesTestSuite& operator=(const CelixBundleContextTempBundlesTestSuite&) = delete;
};

TEST_F(CelixBundleContextTempBundlesTestSuite, InstallABundleTest) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId >= 0);
}
