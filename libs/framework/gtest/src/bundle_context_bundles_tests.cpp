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

#include <zconf.h>

#include "celix_api.h"

class CelixBundleContextBundlesTests : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    const char * const TEST_BND1_LOC = "simple_test_bundle1.zip";
    const char * const TEST_BND2_LOC = "simple_test_bundle2.zip";
    const char * const TEST_BND3_LOC = "simple_test_bundle3.zip";
    const char * const TEST_BND4_LOC = "simple_test_bundle4.zip";
    const char * const TEST_BND5_LOC = "simple_test_bundle5.zip";
    const char * const TEST_BND_WITH_EXCEPTION_LOC = "bundle_with_exception.zip";
    const char * const TEST_BND_UNRESOLVEABLE_LOC = "unresolveable_bundle.zip";

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



TEST_F(CelixBundleContextBundlesTests, installBundlesTest) {
    long bndId = celix_bundleContext_installBundle(ctx, "non-existing.zip", true);
    ASSERT_TRUE(bndId < 0);

    bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bndId >= 0);

    bndId = celix_bundleContext_installBundle(ctx, TEST_BND4_LOC, true); //not loaded in subdir
    ASSERT_TRUE(bndId < 0);

    setenv(CELIX_BUNDLES_PATH_NAME, "subdir", true);
    bndId = celix_bundleContext_installBundle(ctx, TEST_BND4_LOC, true); //subdir now part of CELIX_BUNDLES_PATH
    ASSERT_TRUE(bndId >= 0);
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
        int count{0};
        std::mutex mutex{};
        std:: condition_variable cond{};
    };
    struct data data;

    auto started = [](void *handle, const bundle_t *bnd) {
        struct data *d = static_cast<struct data*>(handle);
        ASSERT_TRUE(bnd != nullptr);
        d->mutex.lock();
        d->count += 1;
        d->cond.notify_all();
        d->mutex.unlock();
    };
    auto stopped = [](void *handle, const bundle_t *bnd) {
        struct data *d = static_cast<struct data*>(handle);
        ASSERT_TRUE(bnd != nullptr);
        d->mutex.lock();
        d->count -= 1;
        d->cond.notify_all();
        d->mutex.unlock();
    };

    long trackerId = celix_bundleContext_trackBundles(ctx, static_cast<void*>(&data), started, stopped);
    ASSERT_EQ(0, data.count); //note default framework bundle is not tracked


    long bundleId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    ASSERT_TRUE(bundleId1 >= 0);

    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 2;});

    }
    ASSERT_EQ(1, data.count);


    long bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    ASSERT_TRUE(bundleId2 >= 0);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 3;});

    }
    ASSERT_EQ(2, data.count);

    celix_bundleContext_uninstallBundle(ctx, bundleId2);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 2;});

    }
    ASSERT_EQ(2, data.count);

    long bundleId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);
    ASSERT_TRUE(bundleId3 >= 0);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 3;});

    }
    ASSERT_EQ(3, data.count);

    bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    ASSERT_TRUE(bundleId3 >= 0);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 4;});

    }
    ASSERT_EQ(4, data.count);


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