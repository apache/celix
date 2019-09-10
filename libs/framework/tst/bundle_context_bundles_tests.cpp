/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>
#include <condition_variable>

#include <zconf.h>

#include "celix_api.h"

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>


TEST_GROUP(CelixBundleContextBundlesTests) {
    framework_t* fw = nullptr;
    bundle_context_t *ctx = nullptr;
    properties_t *properties = nullptr;

    const char * const TEST_BND1_LOC = "simple_test_bundle1.zip";
    const char * const TEST_BND2_LOC = "simple_test_bundle2.zip";
    const char * const TEST_BND3_LOC = "simple_test_bundle3.zip";
    const char * const TEST_BND4_LOC = "simple_test_bundle4.zip";
    const char * const TEST_BND5_LOC = "simple_test_bundle5.zip";

    void setup() {
        properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);
    }

    void teardown() {
        celix_frameworkFactory_destroyFramework(fw);
    }
};

TEST(CelixBundleContextBundlesTests, installBundlesTest) {
    long bndId = celix_bundleContext_installBundle(ctx, "non-existing.zip", true);
    CHECK(bndId < 0);

    bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    CHECK(bndId >= 0);

    bndId = celix_bundleContext_installBundle(ctx, TEST_BND4_LOC, true); //not loaded in subdir
    CHECK(bndId < 0);

    setenv(CELIX_BUNDLES_PATH_NAME, "subdir", true);
    bndId = celix_bundleContext_installBundle(ctx, TEST_BND4_LOC, true); //subdir now part of CELIX_BUNDLES_PATH
    CHECK(bndId >= 0);
}

TEST(CelixBundleContextBundlesTests, useBundlesTest) {
    int count = 0;

    auto use = [](void *handle, const bundle_t *bnd) {
        int *c = (int*)handle;
        *c += 1;
        long id = celix_bundle_getId(bnd);
        CHECK(id >= 0);
    };

    celix_bundleContext_useBundles(ctx, &count, use);

    count = 0;
    celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    celix_bundleContext_useBundles(ctx, &count, use);
    CHECK_EQUAL(1, count);
};

TEST(CelixBundleContextBundlesTests, useBundleTest) {
    int count = 0;

    celix_bundleContext_useBundle(ctx, 0, &count, [](void *handle, const bundle_t *bnd) {
        int *c = (int*)handle;
        *c += 1;
        long id = celix_bundle_getId(bnd);
        CHECK_EQUAL(0, id);
    });

    CHECK_EQUAL(1, count);
};

TEST(CelixBundleContextBundlesTests, StopStartTest) {
    celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);


    celix_array_list_t *ids = celix_bundleContext_listBundles(ctx);
    size_t size = arrayList_size(ids);
    CHECK_EQUAL(3, size);

    int count = 0;
    celix_bundleContext_useBundles(ctx, &count, [](void *handle, const celix_bundle_t *bnd) {
        auto *c = (int*)handle;
        CHECK_EQUAL(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
        *c += 1;
    });
    CHECK_EQUAL(3, count);


    for (size_t i = 0; i < size; ++i) {
        bool stopped = celix_bundleContext_stopBundle(ctx, celix_arrayList_getLong(ids, (int)i));
        CHECK_TRUE(stopped);
    }

    bool stopped = celix_bundleContext_stopBundle(ctx, 42 /*non existing*/);
    CHECK_FALSE(stopped);

    bool started = celix_bundleContext_startBundle(ctx, 42 /*non existing*/);
    CHECK_FALSE(started);

    for (size_t i = 0; i < size; ++i) {
        bool started = celix_bundleContext_startBundle(ctx, celix_arrayList_getLong(ids, (int)i));
        CHECK_TRUE(started);
    }

    count = 0;
    celix_bundleContext_useBundles(ctx, &count, [](void *handle, const celix_bundle_t *bnd) {
        auto *c = (int*)handle;
        CHECK_EQUAL(OSGI_FRAMEWORK_BUNDLE_ACTIVE, celix_bundle_getState(bnd));
        *c += 1;
    });
    CHECK_EQUAL(3, count);

    celix_arrayList_destroy(ids);
}

TEST(CelixBundleContextBundlesTests, trackBundlesTest) {
    struct data {
        int count{0};
        std::mutex mutex{};
        std:: condition_variable cond{};
    };
    struct data data;

    auto started = [](void *handle, const bundle_t *bnd) {
        struct data *d = static_cast<struct data*>(handle);
        CHECK(bnd != nullptr);
        d->mutex.lock();
        d->count += 1;
        d->cond.notify_all();
        d->mutex.unlock();
    };
    auto stopped = [](void *handle, const bundle_t *bnd) {
        struct data *d = static_cast<struct data*>(handle);
        CHECK(bnd != nullptr);
        d->mutex.lock();
        d->count -= 1;
        d->cond.notify_all();
        d->mutex.unlock();
    };

    long trackerId = celix_bundleContext_trackBundles(ctx, static_cast<void*>(&data), started, stopped);
    CHECK_EQUAL(0, data.count); //note default framework bundle is not tracked


    long bundleId1 = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);
    CHECK(bundleId1 >= 0);

    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 2;});

    }
    CHECK_EQUAL(1, data.count);


    long bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    CHECK(bundleId2 >= 0);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 3;});

    }
    CHECK_EQUAL(2, data.count);

    /* TODO does not work -> stopping bundle event is never forward to the bundle listener ?? very old bug?
    celix_bundleContext_uninstallBundle(ctx, bundleId2);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 2;});

    }
    CHECK_EQUAL(2, data.count);

    long bundleId3 = celix_bundleContext_installBundle(ctx, TEST_BND3_LOC, true);
    CHECK(bundleId3 >= 0);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 3;});

    }
    CHECK_EQUAL(3, data.count);

    bundleId2 = celix_bundleContext_installBundle(ctx, TEST_BND2_LOC, true);
    CHECK(bundleId3 >= 0);
    {
        std::unique_lock<std::mutex> lock{data.mutex};
        data.cond.wait_for(lock, std::chrono::milliseconds(100), [&]{return data.count == 4;});

    }
    CHECK_EQUAL(4, data.count);
     */


    celix_bundleContext_stopTracker(ctx, trackerId);
};

/* IGNORE TODO need to add locks
TEST(CelixBundleContextBundlesTests, useBundlesConcurrentTest) {

    struct data {
        std::mutex mutex{};
        std::condition_variable cond{};
        bool inUse{false};
        bool readyToExit{false};
        bool called{false};
    };
    struct data data{};

    auto use = [](void *handle, const bundle_t *bnd) {
        CHECK(bnd != nullptr);

        struct data *d = static_cast<struct data*>(handle);

        std::unique_lock<std::mutex> lock(d->mutex);
        d->inUse = true;
        d->cond.notify_all();
        d->cond.wait(lock, [d]{return d->readyToExit;});
        lock.unlock();

        auto state = celix_bundle_getState(bnd);
        CHECK_EQUAL(OSGI_FRAMEWORK_BUNDLE_ACTIVE, state);

        d->called = true;
    };

    long bndId = celix_bundleContext_installBundle(ctx, TEST_BND1_LOC, true);

    auto call = [&] {
        celix_bundleContext_useBundle(ctx, bndId, &data, use);
        CHECK(data.called);
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
};*/