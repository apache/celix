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

#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <zconf.h>

#include "bundle.h"
#include "properties.h"
#include "celix_framework_factory.h"


TEST_GROUP(CelixBundleContextServicesTests) {
    framework_t* fw = NULL;
    bundle_context_t *ctx = NULL;
    properties_t *properties = NULL;

    void setup() {
        properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = frameworkFactory_newFramework(properties);
        ctx = framework_getContext(fw);
    }

    void teardown() {
        framework_stop(fw);
        framework_waitForStop(fw);
        framework_destroy(fw);
    }
};

TEST(CelixBundleContextServicesTests, registerService) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, calcName, &svc, NULL, NULL);
    CHECK(svcId >= 0);
    celix_bundleContext_unregisterService(ctx, svcId);
};

TEST(CelixBundleContextServicesTests, incorrectUnregisterCalls) {
    celix_bundleContext_unregisterService(ctx, 1);
    celix_bundleContext_unregisterService(ctx, 2);
    celix_bundleContext_unregisterService(ctx, -1);
    celix_bundleContext_unregisterService(ctx, -2);
};

TEST(CelixBundleContextServicesTests, registerMultipleAndUseServices) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, calcName, &svc, NULL, NULL);
    CHECK(svcId1 >= 0);

    long svcId2 = celix_bundleContext_registerService(ctx, calcName, &svc, NULL, NULL);
    CHECK(svcId2 >= 0);

    long svcId3 = celix_bundleContext_registerService(ctx, calcName, &svc, NULL, NULL);
    CHECK(svcId3 >= 0);

    auto use = [](void *handle, void *svc, const properties_t *props, const bundle_t *bnd) {
        CHECK(svc != NULL);
        CHECK(props != NULL);
        CHECK(bnd != NULL);
        int *total =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(1);
        *total += tmp;
    };

    int total = 0;
    celix_bundleContext_useServices(ctx, "calc", NULL, NULL, &total, use);
    CHECK_EQUAL(42 * 3, total);


    celix_bundleContext_unregisterService(ctx, svcId3);
    total = 0;
    celix_bundleContext_useServices(ctx, "calc", NULL, NULL, &total, use);
    CHECK_EQUAL(42 * 2, total);

    total = 0;
    bool called = celix_bundleContext_useService(ctx, "calc", NULL, NULL, &total, use);
    CHECK(called);
    CHECK_EQUAL(42, total);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
};

TEST(CelixBundleContextServicesTests, registerAndUseService) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, calcName, &svc, NULL, NULL);
    CHECK(svcId >= 0);

    int result = 0;
    bool called = celix_bundleContext_useServiceWithId(ctx, svcId, calcName, &result, [](void *handle, void *svc, const properties_t *props, const bundle_t *bnd) {
        CHECK(svc != NULL);
        CHECK(props != NULL);
        CHECK(bnd != NULL);
        int *result =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(2);
        *result = tmp;
    });
    CHECK(called);
    CHECK_EQUAL(84, result);

    result = 0;
    long nonExistingSvcId = 101;
    called = celix_bundleContext_useServiceWithId(ctx, nonExistingSvcId, calcName, &result, [](void *handle, void *svc, const properties_t *, const bundle_t *) {
        int *result =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(2);
        *result = tmp;
    });
    CHECK(!called);
    CHECK_EQUAL(0, result); //e.g. not called

    celix_bundleContext_unregisterService(ctx, svcId);
};

TEST(CelixBundleContextServicesTests, registerAndUseWithForcedRaceCondition) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, calcName, &svc, NULL, NULL);
    CHECK(svcId >= 0);

    struct sync {
        std::mutex mutex{};
        std::condition_variable sync{};
        bool inUseCall{false};
        bool readyToExitUseCall{false};
        bool unregister{false};
        int result{0};
    };
    struct sync callInfo{};

    auto use = [](void *handle, void *svc, const properties_t *props, const bundle_t *bnd) {
        CHECK(svc != NULL);
        CHECK(props != NULL);
        CHECK(bnd != NULL);

        struct sync *h = static_cast<struct sync*>(handle);

        std::cout << "setting isUseCall to true and syncing on readyToExitUseCall" << std::endl;
        std::unique_lock<std::mutex> lock(h->mutex);
        h->inUseCall = true;
        h->sync.notify_all();
        h->sync.wait(lock, [h]{return h->readyToExitUseCall;});
        lock.unlock();

        std::cout << "Calling calc " << std::endl;
        struct calc *calc = static_cast<struct calc *>(svc);
        int tmp = calc->calc(2);
        h->result = tmp;
    };

    auto call = [&] {
        bool called = celix_bundleContext_useServiceWithId(ctx, svcId, calcName, &callInfo, use);
        CHECK(called);
        CHECK_EQUAL(84, callInfo.result);
    };
    std::thread useThread{call};


    std::thread unregisterThread{[&]{
        std::cout << "syncing to wait if use function is called ..." << std::endl;
        std::unique_lock<std::mutex> lock(callInfo.mutex);
        callInfo.sync.wait(lock, [&]{return callInfo.inUseCall;});
        lock.unlock();
        std::cout << "trying to unregister ..." << std::endl;
        celix_bundleContext_unregisterService(ctx, svcId);
        std::cout << "done unregistering" << std::endl;
    }};


    //sleep 100 milli to give unregister a change to sink in
    std::cout << "before sleep" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "after sleep" << std::endl;


    std::cout << "setting readyToExitUseCall and notify" << std::endl;
    std::unique_lock<std::mutex> lock(callInfo.mutex);
    callInfo.readyToExitUseCall = true;
    lock.unlock();
    callInfo.sync.notify_all();

    useThread.join();
    std::cout << "use thread joined" << std::endl;
    unregisterThread.join();
    std::cout << "unregister thread joined" << std::endl;
};
