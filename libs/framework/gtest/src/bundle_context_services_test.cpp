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
#include <string.h>
#include <future>

#include "celix_api.h"
#include "celix_framework_factory.h"
#include "celix_service_factory.h"
#include "service_tracker_private.h"

class CelixBundleContextServicesTests : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    CelixBundleContextServicesTests() {
        properties = properties_create();
        properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
        properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);
    }

    ~CelixBundleContextServicesTests() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    CelixBundleContextServicesTests(CelixBundleContextServicesTests&&) = delete;
    CelixBundleContextServicesTests(const CelixBundleContextServicesTests&) = delete;
    CelixBundleContextServicesTests& operator=(CelixBundleContextServicesTests&&) = delete;
    CelixBundleContextServicesTests& operator=(const CelixBundleContextServicesTests&) = delete;
};

TEST_F(CelixBundleContextServicesTests, registerService) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId >= 0);
    celix_bundleContext_unregisterService(ctx, svcId);
};

TEST_F(CelixBundleContextServicesTests, incorrectUnregisterCalls) {
    celix_bundleContext_unregisterService(ctx, 1);
    celix_bundleContext_unregisterService(ctx, 2);
    celix_bundleContext_unregisterService(ctx, -1);
    celix_bundleContext_unregisterService(ctx, -2);
};

TEST_F(CelixBundleContextServicesTests, registerMultipleAndUseServices) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId1 >= 0);

    long svcId2 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId2 >= 0);

    long svcId3 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId3 >= 0);

    auto use = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        int *total =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(1);
        *total += tmp;
    };

    int total = 0;
    auto count = celix_bundleContext_useServices(ctx, "calc", &total, use);
    ASSERT_EQ(3, count);
    ASSERT_EQ(42 * 3, total);


    celix_bundleContext_unregisterService(ctx, svcId3);
    total = 0;
    count = celix_bundleContext_useServices(ctx, "calc", &total, use);
    ASSERT_EQ(2, count);
    ASSERT_EQ(42 * 2, total);

    total = 0;
    bool called = celix_bundleContext_useService(ctx, "calc", &total, use);
    ASSERT_TRUE(called);
    ASSERT_EQ(42, total);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);

    //NOTE superfluous (note prints errors)
    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
};

TEST_F(CelixBundleContextServicesTests, registerAndUseService) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId >= 0);

    int result = 0;
    bool called = celix_bundleContext_useServiceWithId(ctx, svcId, calcName, &result, [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        int *result =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(2);
        *result = tmp;
    });
    ASSERT_TRUE(called);
    ASSERT_EQ(84, result);

    result = 0;
    long nonExistingSvcId = 101;
    called = celix_bundleContext_useServiceWithId(ctx, nonExistingSvcId, calcName, &result, [](void *handle, void *svc) {
        int *result =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(2);
        *result = tmp;
    });
    ASSERT_TRUE(!called);
    ASSERT_EQ(0, result); //e.g. not called

    celix_bundleContext_unregisterService(ctx, svcId);
};

TEST_F(CelixBundleContextServicesTests, registerAndUseServiceWithTimeout) {
    const int NR_ITERATIONS = 5; //NOTE this test is sensitive for triggering race condition in the celix framework, therefore is used a few times.
    for (int i = 0; i < NR_ITERATIONS; ++i) {
        printf("Iter %i\n", i);
        struct calc {
            int (*calc)(int);
        };

        const char *calcName = "calc";
        struct calc svc;
        svc.calc = [](int n) -> int {
            return n * 42;
        };

        celix_service_use_options_t opts{};
        opts.filter.serviceName = "calc";

        bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
        EXPECT_FALSE(called); //service not avail.

        std::future<bool> result{std::async([&] {
            opts.waitTimeoutInSeconds = 2.0;
            //printf("Trying to call calc with timeout of %f\n", opts.waitTimeoutInSeconds);
            bool calledAsync = celix_bundleContext_useServiceWithOptions(ctx, &opts);
            //printf("returned from use service with timeout. calc called %s.\n", calledAsync ? "true" : "false");
            return calledAsync;
        })};
        long svcId = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
        EXPECT_TRUE(svcId >= 0);


        EXPECT_TRUE(result.get()); //should return true after waiting for the registered service.


        celix_bundleContext_unregisterService(ctx, svcId);


        //check if timeout is triggered
        std::future<bool> result2{std::async([&] {
            opts.waitTimeoutInSeconds = 0.1;
            //printf("Trying to call calc with timeout of %f\n", opts.waitTimeoutInSeconds);
            bool calledAsync = celix_bundleContext_useServiceWithOptions(ctx, &opts);
            //printf("returned from use service with timeout. calc called %s.\n", calledAsync ? "true" : "false");
            return calledAsync;
        })};
        EXPECT_FALSE(result2.get()); //note service is away, so even with a wait the service is not found.
    }
}

TEST_F(CelixBundleContextServicesTests, registerAndUseServiceWithCorrectVersion) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    celix_service_use_options_t use_opts{};
    use_opts.filter.serviceName = "calc";
    use_opts.filter.versionRange = "[1,2)";

    celix_service_registration_options_t reg_opts{};
    reg_opts.serviceName = calcName;
    reg_opts.serviceVersion = "1.5";
    reg_opts.svc = &svc;

    bool called = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
    ASSERT_TRUE(!called); //service not avail.

    std::future<bool> result{std::async([&]{
        use_opts.waitTimeoutInSeconds = 5.0;
        printf("Trying to call calc with timeout of %f\n", use_opts.waitTimeoutInSeconds);
        bool calledAsync = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
        printf("returned from use service with timeout. calc called %s.\n", calledAsync ? "true" : "false");
        return calledAsync;
    })};
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx, &reg_opts);
    ASSERT_TRUE(svcId >= 0);

    ASSERT_TRUE(result.get()); //should return true after waiting for the registered service.

    celix_bundleContext_unregisterService(ctx, svcId);
}

TEST_F(CelixBundleContextServicesTests, registerAndUseServiceWithIncorrectVersion) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    celix_service_use_options_t use_opts{};
    use_opts.filter.serviceName = "calc";
    use_opts.filter.versionRange = "[2,3)";

    celix_service_registration_options_t reg_opts{};
    reg_opts.serviceName = calcName;
    reg_opts.serviceVersion = "1.5";
    reg_opts.svc = &svc;

    bool called = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
    ASSERT_TRUE(!called); //service not avail.

    std::future<bool> result{std::async([&]{
        use_opts.waitTimeoutInSeconds = 1.0;
        printf("Trying to call calc with timeout of %f\n", use_opts.waitTimeoutInSeconds);
        bool calledAsync = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
        printf("returned from use service with timeout. calc called %s.\n", calledAsync ? "true" : "false");
        return calledAsync;
    })};
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx, &reg_opts);
    ASSERT_TRUE(svcId >= 0);
    ASSERT_TRUE(!result.get());

    celix_bundleContext_unregisterService(ctx, svcId);
}

TEST_F(CelixBundleContextServicesTests, registerAndUseWithForcedRaceCondition) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId >= 0);

    struct sync {
        std::mutex mutex{};
        std::condition_variable sync{};
        bool inUseCall{false};
        bool readyToExitUseCall{false};
        bool unregister{false};
        int result{0};
    };
    struct sync callInfo{};

    auto use = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);

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
        ASSERT_TRUE(called);
        ASSERT_EQ(84, callInfo.result);
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


TEST_F(CelixBundleContextServicesTests, servicesTrackerTest) {
    int count = 0;
    auto add = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        int *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        int *c = static_cast<int*>(handle);
        *c -= 1;
    };

    long trackerId = celix_bundleContext_trackServices(ctx, "calc", &count, add, remove);
    ASSERT_TRUE(trackerId >= 0);
    ASSERT_EQ(0, count);

    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x100, "calc", nullptr);
    ASSERT_TRUE(svcId1 >= 0);
    ASSERT_EQ(1, count);

    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x200, "calc", nullptr);
    ASSERT_TRUE(svcId2 >= 0);
    ASSERT_EQ(2, count);

    celix_bundleContext_unregisterService(ctx, svcId1);
    ASSERT_EQ(1, count);

    celix_bundleContext_stopTracker(ctx, trackerId);
    celix_bundleContext_unregisterService(ctx, svcId2);
}

TEST_F(CelixBundleContextServicesTests, servicesTrackerInvalidArgsTest) {
    long trackerId = celix_bundleContext_trackServices(nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_TRUE(trackerId < 0); //required ctx and service name missing
    trackerId = celix_bundleContext_trackServices(ctx, nullptr, nullptr, nullptr, nullptr);
    ASSERT_TRUE(trackerId < 0); //required service name missing
    trackerId = celix_bundleContext_trackServices(ctx, "calc", nullptr, nullptr, nullptr);
    ASSERT_TRUE(trackerId >= 0); //valid
    celix_bundleContext_stopTracker(ctx, trackerId);


    //opts
    trackerId = celix_bundleContext_trackServicesWithOptions(nullptr, nullptr);
    ASSERT_TRUE(trackerId < 0); //required ctx and opts missing
    trackerId = celix_bundleContext_trackServicesWithOptions(ctx, nullptr);
    ASSERT_TRUE(trackerId < 0); //required opts missing
    celix_service_tracking_options_t opts{};
    trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId < 0); //required opts->serviceName missing
    opts.filter.serviceName = "calc";
    trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0); //valid
    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextServicesTests, servicesTrackerTestWithAlreadyRegisteredServices) {
    int count = 0;
    auto add = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        int *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        int *c = static_cast<int*>(handle);
        *c -= 1;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x010, "calc", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x020, "calc", nullptr);



    long trackerId = celix_bundleContext_trackServices(ctx, "calc", &count, add, remove);
    ASSERT_TRUE(trackerId >= 0);
    ASSERT_EQ(2, count);

    long svcId3 = celix_bundleContext_registerService(ctx, (void*)0x100, "calc", nullptr);
    ASSERT_TRUE(svcId1 >= 0);
    ASSERT_EQ(3, count);

    long svcId4 = celix_bundleContext_registerService(ctx, (void*)0x200, "calc", nullptr);
    ASSERT_TRUE(svcId2 >= 0);
    ASSERT_EQ(4, count);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId3);
    ASSERT_EQ(2, count);

    celix_bundleContext_stopTracker(ctx, trackerId);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId4);
}

TEST_F(CelixBundleContextServicesTests, servicesTrackerTestWithProperties) {
    int count = 0;
    auto add = [](void *handle, void *svc, const properties_t *props) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_STRCASEEQ("C", celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, nullptr));
        int *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc, const properties_t *props) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_STRCASEEQ("C", celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, nullptr));
        int *c = static_cast<int*>(handle);
        *c -= 1;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x100, "calc", nullptr);

    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "calc";
    opts.callbackHandle = &count;
    opts.addWithProperties = add;
    opts.removeWithProperties = remove;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0);
    ASSERT_EQ(1, count);

    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x200, "calc", nullptr);
    ASSERT_TRUE(svcId1 >= 0);
    ASSERT_EQ(2, count);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    ASSERT_EQ(0, count);

    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextServicesTests, servicesTrackerTestWithOwner) {
    int count = 0;
    auto add = [](void *handle, void *svc, const properties_t *props, const bundle_t *svcOwner) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_STRCASEEQ("C", celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, nullptr));
        ASSERT_TRUE(celix_bundle_getId(svcOwner) >= 0);
        int *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc, const properties_t *props, const bundle_t *svcOwner) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_STRCASEEQ("C", celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_LANGUAGE, nullptr));
        ASSERT_TRUE(celix_bundle_getId(svcOwner) >= 0);
        int *c = static_cast<int*>(handle);
        *c -= 1;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x100, "calc", nullptr);

    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "calc";
    opts.callbackHandle = &count;
    opts.addWithOwner = add;
    opts.removeWithOwner = remove;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0);
    ASSERT_EQ(1, count);

    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x200, "calc", nullptr);
    ASSERT_TRUE(svcId1 >= 0);
    ASSERT_EQ(2, count);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    ASSERT_EQ(0, count);

    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextServicesTests, serviceTrackerWithRaceConditionTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    struct calc svc;
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    struct data {
        std::mutex mutex{};
        std::condition_variable sync{};
        long svcId {-1};
        bool inAddCall{false};
        bool inRemoveCall{false};
        bool readyToExit{false};
        int result{0};
    };
    struct data data{};

    auto add = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);

        struct data *d = static_cast<struct data*>(handle);

        std::unique_lock<std::mutex> lock(d->mutex);
        d->inAddCall = true;
        d->sync.notify_all();
        d->sync.wait(lock, [d]{return d->readyToExit;});
        lock.unlock();

        struct calc *calc = static_cast<struct calc *>(svc);
        int tmp = calc->calc(2);

        lock.lock();
        d->result = tmp;
        lock.unlock();
    };

    auto remove = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);

        struct data *d = static_cast<struct data*>(handle);

        std::unique_lock<std::mutex> lock(d->mutex);
        std::cout << "In Remove call. waiting for ready to exit" << std::endl;
        d->inRemoveCall = true;
        d->sync.notify_all();
        d->sync.wait(lock, [d]{return d->readyToExit;});
        lock.unlock();
    };

    long trackerId = celix_bundleContext_trackServices(ctx, calcName, &data, add, remove);

    std::thread registerThread{[&]{
        long id = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
        std::cout << "registered service with id " << id << std::endl;
        std::lock_guard<std::mutex> lock{data.mutex};
        data.svcId = id;
        data.sync.notify_all();
    }};

    std::thread unregisterThread{[&]{
        long id = -1;
        std::unique_lock<std::mutex> lock(data.mutex);
        data.sync.wait(lock, [&]{return data.svcId >= 0;});
        id = data.svcId;
        lock.unlock();
        std::cout << "trying to unregister ... with id " << id << std::endl;
        celix_bundleContext_unregisterService(ctx, id);
        std::cout << "done unregistering" << std::endl;
    }};


    std::cout << "waiting till inAddCall" << std::endl;
    std::unique_lock<std::mutex> lock{data.mutex};
    data.sync.wait(lock, [&]{return data.inAddCall;});
    data.readyToExit = true;
    data.sync.notify_all();
    lock.unlock();

    //let unregister sink in.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    lock.lock();
    std::cout << "triggering ready to exit" << std::endl;
    data.readyToExit = true;
    data.sync.notify_all();
    lock.unlock();

    registerThread.join();
    unregisterThread.join();
    std::cout << "threads joined" << std::endl;

    ASSERT_EQ(84, data.result);
    ASSERT_TRUE(data.inAddCall);
    ASSERT_TRUE(data.inRemoveCall);

    celix_bundleContext_stopTracker(ctx, trackerId);
};

TEST_F(CelixBundleContextServicesTests, servicesTrackerSetTest) {
    int count = 0;

    void *svc1 = (void*)0x100; //no ranking
    void *svc2 = (void*)0x200; //no ranking
    void *svc3 = (void*)0x300; //10 ranking
    void *svc4 = (void*)0x400; //5 ranking

    auto set = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        static int callCount = 0;
        callCount += 1;
        if (callCount == 1) {
            //first time svc1 should be set (oldest service with equal ranking
            ASSERT_EQ(0x100, (long)svc);
        } else if (callCount == 2) {
            ASSERT_EQ(0x300, (long)svc);
            //second time svc3 should be set (highest ranking)
        } else if (callCount == 3) {
            //third time svc4 should be set (highest ranking
            ASSERT_EQ(0x400, (long)svc);
        }

        int *c = static_cast<int*>(handle);
        *c = callCount;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, svc1, "NA", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, svc2, "NA", nullptr);

    //starting tracker should lead to first set call
    celix_service_tracking_options_t opts{};
    opts.callbackHandle = (void*)&count;
    opts.filter.serviceName = "NA";
    opts.set = set;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0);

    //register svc3 should lead to second set call
    properties_t *props3 = celix_properties_create();
    celix_properties_set(props3, OSGI_FRAMEWORK_SERVICE_RANKING, "10");
    long svcId3 = celix_bundleContext_registerService(ctx, svc3, "NA", props3);

    //register svc4 should lead to no set (lower ranking)
    properties_t *props4 = celix_properties_create();
    celix_properties_set(props4, OSGI_FRAMEWORK_SERVICE_RANKING, "10");
    long svcId4 = celix_bundleContext_registerService(ctx, svc4, "NA", props4);

    //unregister svc3 should lead to set (new highest ranking)
    celix_bundleContext_unregisterService(ctx, svcId3);

    celix_bundleContext_stopTracker(ctx, trackerId);
    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId4);

    ASSERT_EQ(3, count); //check if the set is called the expected times
}

//TODO test tracker with options for properties & service owners


TEST_F(CelixBundleContextServicesTests, serviceFactoryTest) {
    struct calc {
        int (*calc)(int);
    };
    auto name = "CALC";

    int count = 0;
    celix_service_factory_t fac;
    memset(&fac, 0, sizeof(fac));
    fac.handle = (void*)&count;
    fac.getService = [](void *handle, const celix_bundle_t *, const celix_properties_t *) -> void* {
        auto *c = (int *)handle;
        *c += 1;
        static struct calc svc; //normally a service per bundle
        svc.calc = [](int arg) { return arg * 42; };
        return &svc;
    };
    fac.ungetService = [](void *handle, const celix_bundle_t *, const celix_properties_t *) {
        auto *c = (int *)handle;
        *c += 1;
    };

    long facId = celix_bundleContext_registerServiceFactory(ctx, &fac, name, nullptr);
    ASSERT_TRUE(facId >= 0);


    int result = -1;
    bool called = celix_bundleContext_useService(ctx, name, &result, [](void *handle, void* svc) {
        auto *r = (int *)(handle);
        auto *calc = (struct calc*)svc;
        *r = calc->calc(2);
    });
    ASSERT_TRUE(called);
    ASSERT_EQ(84, result);
    ASSERT_EQ(2, count); //expecting getService & unGetService to be called during the useService call.


    celix_bundleContext_unregisterService(ctx, facId);
}

TEST_F(CelixBundleContextServicesTests, findServicesTest) {
    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x100, "example", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x100, "example", nullptr);

    long foundId = celix_bundleContext_findService(ctx, "non existing service name");
    ASSERT_EQ(-1L, foundId);

    foundId = celix_bundleContext_findService(ctx, "example");
    ASSERT_EQ(foundId, svcId1); //oldest should have highest ranking

    array_list_t *list = celix_bundleContext_findServices(ctx, "non existintg service name");
    ASSERT_EQ(0, celix_arrayList_size(list));
    arrayList_destroy(list);

    list = celix_bundleContext_findServices(ctx, "example");
    ASSERT_EQ(2, celix_arrayList_size(list));
    arrayList_destroy(list);

    celix_bundleContext_unregisterService(ctx, svcId1);

    celix_service_filter_options_t opts{};
    opts.serviceName = "example";
    foundId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    ASSERT_EQ(foundId, svcId2); //only one left

    celix_bundleContext_unregisterService(ctx, svcId2);
}

TEST_F(CelixBundleContextServicesTests, trackServiceTrackerTest) {

    int count = 0;

    auto add = [](void *handle, const celix_service_tracker_info_t *info) {
        ASSERT_STRCASEEQ("example", info->serviceName);
        ASSERT_STRCASEEQ(CELIX_FRAMEWORK_SERVICE_C_LANGUAGE, info->serviceLanguage);
        auto *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, const celix_service_tracker_info_t *info) {
        ASSERT_STRCASEEQ("example", info->serviceName);
        ASSERT_STRCASEEQ(CELIX_FRAMEWORK_SERVICE_C_LANGUAGE, info->serviceLanguage);
        auto *c = static_cast<int*>(handle);
        *c -= 1;
    };

    long trackerId = celix_bundleContext_trackServiceTrackers(ctx, "example", &count, add, remove);
    ASSERT_TRUE(trackerId >= 0);
    ASSERT_EQ(0, count);

    long tracker2 = celix_bundleContext_trackService(ctx, "example", nullptr, nullptr);
    ASSERT_TRUE(tracker2 >= 0);
    ASSERT_EQ(1, count);

    long tracker3 = celix_bundleContext_trackServices(ctx, "example", nullptr, nullptr, nullptr);
    ASSERT_TRUE(tracker3 >= 0);
    ASSERT_EQ(2, count);

    long tracker4 = celix_bundleContext_trackServices(ctx, "no-match", nullptr, nullptr, nullptr);
    ASSERT_TRUE(tracker4 >= 0);
    ASSERT_EQ(2, count);

    celix_bundleContext_stopTracker(ctx, tracker2);
    celix_serviceTracker_syncForContext(ctx); //service tracker shutdown on separate track -> need sync
    ASSERT_EQ(1, count);
    celix_bundleContext_stopTracker(ctx, tracker3);
    celix_serviceTracker_syncForContext(ctx); //service tracker shutdown on separate track -> need sync
    ASSERT_EQ(0, count);

    celix_bundleContext_stopTracker(ctx, trackerId);
    celix_bundleContext_stopTracker(ctx, tracker4);
}
