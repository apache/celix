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
#include <chrono>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <celix_log_utils.h>

#include "celix_api.h"

class CelixBundleContextBundlesTests : public ::testing::Test {
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
    const char * const TEST_BND_UNRESOLVEABLE_LOC = "" TEST_BUNDLE_UNRESOLVEABLE_LOCATION "";

    CelixBundleContextBundlesTests() {
        properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);
    }
    
    ~CelixBundleContextBundlesTests() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    CelixBundleContextBundlesTests(CelixBundleContextBundlesTests&&) = delete;
    CelixBundleContextBundlesTests(const CelixBundleContextBundlesTests&) = delete;
    CelixBundleContextBundlesTests& operator=(CelixBundleContextBundlesTests&&) = delete;
    CelixBundleContextBundlesTests& operator=(const CelixBundleContextBundlesTests&) = delete;
};


TEST_F(CelixBundleContextBundlesTests, StartStopTest) {
    //nop
}

TEST_F(CelixBundleContextBundlesTests, installBundlesTest) {
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

TEST_F(CelixBundleContextBundlesTests, TestIsSystemBundle) {
    celix_bundle_t* fwBnd = celix_framework_getFrameworkBundle(fw);
    ASSERT_TRUE(celix_bundle_isSystemBundle(fwBnd));

    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId >= 0);
    bool called = celix_bundleContext_useBundle(ctx, bndId, nullptr, [](void*, const celix_bundle_t* bnd) {
        EXPECT_FALSE(celix_bundle_isSystemBundle(bnd));
    });
    EXPECT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesTests, useBundlesTest) {
    int count = 0;

    auto use = [](void *handle, const bundle_t *bnd) {
        int *c = (int*)handle;
        *c += 1;
        long id = celix_bundle_getId(bnd);
        ASSERT_TRUE(id >= 0);
    };

    celix_bundleContext_useBundles(ctx, &count, use);

    count = 0;
    celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    celix_bundleContext_useBundles(ctx, &count, use);
    ASSERT_EQ(1, count);
};

TEST_F(CelixBundleContextBundlesTests, installAndUninstallBundlesTest) {
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

    //reinstall bundles
    long bndId4 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId5 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, false);
    long bndId6 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);

    ASSERT_TRUE(bndId4 >= 0L);
    ASSERT_FALSE(bndId1 == bndId4); //not new id
    ASSERT_TRUE(bndId5 >= 0L);
    ASSERT_FALSE(bndId2 == bndId5); //not new id
    ASSERT_TRUE(bndId6 >= 0L);
    ASSERT_FALSE(bndId5 == bndId6); //not new id
}

TEST_F(CelixBundleContextBundlesTests, startBundleWithException) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND_WITH_EXCEPTION_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not started

    bool called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void */*handle*/, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(state, OSGI_FRAMEWORK_BUNDLE_RESOLVED);
    });
    ASSERT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesTests, startUnresolveableBundle) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND_UNRESOLVEABLE_LOC, true);
    ASSERT_TRUE(bndId > 0); //bundle is installed, but not resolved

    bool called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_INSTALLED, state);
    });
    ASSERT_TRUE(called);

    celix_bundleContext_startBundle(ctx, bndId);

   called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        auto state = celix_bundle_getState(bnd);
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_INSTALLED, state);
    });
    ASSERT_TRUE(called);
}


TEST_F(CelixBundleContextBundlesTests, useBundleTest) {
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

TEST_F(CelixBundleContextBundlesTests, StopStartTest) {
    long bndId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    long bndId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    long bndId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId1));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId2));
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId3));
    ASSERT_FALSE(celix_bundleContext_isBundleInstalled(ctx, 600 /*non existing*/));



    celix_array_list_t *ids = celix_bundleContext_listBundles(ctx);
    size_t size = arrayList_size(ids);
    ASSERT_EQ(3, size);

    int count = 0;
    celix_bundleContext_useBundles(ctx, &count, [](void *handle, const celix_bundle_t *bnd) {
        auto *c = (int*)handle;
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
        *c += 1;
    });
    ASSERT_EQ(3, count);


    for (size_t i = 0; i < size; ++i) {
        bool stopped = celix_bundleContext_stopBundle(ctx, celix_arrayList_getLong(ids, (int)i));
        ASSERT_TRUE(stopped);
    }

    bool stopped = celix_bundleContext_stopBundle(ctx, 42 /*non existing*/);
    ASSERT_FALSE(stopped);

    bool started = celix_bundleContext_startBundle(ctx, 42 /*non existing*/);
    ASSERT_FALSE(started);

    for (size_t i = 0; i < size; ++i) {
        started = celix_bundleContext_startBundle(ctx, celix_arrayList_getLong(ids, (int)i));
        ASSERT_TRUE(started);
    }

    count = 0;
    celix_bundleContext_useBundles(ctx, &count, [](void *handle, const celix_bundle_t *bnd) {
        auto *c = (int*)handle;
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
        *c += 1;
    });
    ASSERT_EQ(3, count);

    celix_arrayList_destroy(ids);
}

TEST_F(CelixBundleContextBundlesTests, DoubleStopTest) {
    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId > 0);
    ASSERT_TRUE(celix_bundleContext_isBundleInstalled(ctx, bndId));

    bool called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
    });
    ASSERT_TRUE(called);

    //first
    celix_bundleContext_stopBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_RESOLVED, celix_bundle_getState(bnd));
    });
    ASSERT_TRUE(called);

    //second
    celix_bundleContext_stopBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_RESOLVED, celix_bundle_getState(bnd));
    });
    ASSERT_TRUE(called);

    //first
    celix_bundleContext_startBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
    });
    ASSERT_TRUE(called);

    //second
    celix_bundleContext_startBundle(ctx, bndId);

    called = celix_framework_useBundle(fw, false, bndId, nullptr, [](void *, const celix_bundle_t *bnd) {
        ASSERT_EQ(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
    });
    ASSERT_TRUE(called);
}

TEST_F(CelixBundleContextBundlesTests, trackBundlesTest) {
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

TEST_F(CelixBundleContextBundlesTests, useBundlesConcurrentTest) {

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

TEST_F(CelixBundleContextBundlesTests, bundleInfoTests) {
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
        celix_bundle_destroyServiceTrackerList(trackers);
        celix_bundle_destroyRegisteredServicesList(services);
    };

    bool called = celix_bundleContext_useBundle(ctx, 0, &data, updateCountFp);
    ASSERT_TRUE(called);
    ASSERT_EQ(0, data.provideCount);
    ASSERT_EQ(0, data.requestedCount);


    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "NopService", NULL);
    long trackerId = celix_bundleContext_trackServices(ctx, "AService", NULL, NULL, NULL);

    called = celix_bundleContext_useBundle(ctx, 0, &data, updateCountFp);
    ASSERT_TRUE(called);
    ASSERT_EQ(1, data.provideCount);
    ASSERT_EQ(1, data.requestedCount);

    celix_bundleContext_unregisterService(ctx, svcId);
    celix_bundleContext_stopTracker(ctx, trackerId);
}
