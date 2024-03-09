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
#include <cstring>
#include <future>

#include "celix_api.h"
#include "celix_framework_factory.h"
#include "celix_service_factory.h"
#include "service_tracker_private.h"

class CelixBundleContextServicesTestSuite : public ::testing::Test {
public:
    celix_framework_t* fw = nullptr;
    celix_bundle_context_t *ctx = nullptr;
    celix_properties_t *properties = nullptr;

    const char * const CMP_TEST_BND_LOC = "" CMP_TEST_BUNDLE_LOC "";

    CelixBundleContextServicesTestSuite() {
        properties = celix_properties_create();
        celix_properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
        celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");
        celix_properties_set(properties, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace");
        celix_properties_set(properties, "CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED", "false");
        celix_properties_setLong(properties, CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE,  256); //ensure that the floodEventLoopTest overflows the static event queue size

        fw = celix_frameworkFactory_createFramework(properties);
        ctx = framework_getContext(fw);

        celix_err_resetErrors();
    }

    ~CelixBundleContextServicesTestSuite() override {
        celix_frameworkFactory_destroyFramework(fw);
    }

    CelixBundleContextServicesTestSuite(CelixBundleContextServicesTestSuite&&) = delete;
    CelixBundleContextServicesTestSuite(const CelixBundleContextServicesTestSuite&) = delete;
    CelixBundleContextServicesTestSuite& operator=(CelixBundleContextServicesTestSuite&&) = delete;
    CelixBundleContextServicesTestSuite& operator=(const CelixBundleContextServicesTestSuite&) = delete;

    void registerAndUseServiceWithCorrectVersion(bool direct) const {
        struct calc {
            int (*calc)(int);
        };

        const char *calcName = "calc";
        calc svc{};
        svc.calc = [](int n) -> int {
            return n * 42;
        };

        celix_service_use_options_t use_opts{};
        use_opts.filter.serviceName = "calc";
        use_opts.filter.versionRange = "[1,2)";
        if(direct) {
            use_opts.flags = CELIX_SERVICE_USE_DIRECT;
        }

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

    void registerAndUseServiceWithIncorrectVersion(bool direct) const {
        struct calc {
            int (*calc)(int);
        };

        const char *calcName = "calc";
        calc svc{};
        svc.calc = [](int n) -> int {
            return n * 42;
        };

        celix_service_use_options_t use_opts{};
        use_opts.filter.serviceName = "calc";
        use_opts.filter.versionRange = "[2,3)";
        if(direct) {
            use_opts.flags = CELIX_SERVICE_USE_DIRECT;
        }

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

    void registerAndUseServiceWithTimeout(bool direct) const {
        const int NR_ITERATIONS = 5; //NOTE this test is sensitive for triggering race condition in the celix framework, therefore is used a few times.
        for (int i = 0; i < NR_ITERATIONS; ++i) {
            printf("Iter %i\n", i);
            struct calc {
                int (*calc)(int);
            };

            const char *calcName = "calc";
            calc svc{};
            svc.calc = [](int n) -> int {
                return n * 42;
            };

            celix_service_use_options_t opts{};
            opts.filter.serviceName = "calc";
            if(direct) {
                opts.flags = CELIX_SERVICE_USE_DIRECT;
            }

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

    void registerAsyncAndUseServiceWithTimeout(bool direct) const {
        const int NR_ITERATIONS = 5; //NOTE this test is sensitive for triggering race condition in the celix framework, therefore is used a few times.
        for (int i = 0; i < NR_ITERATIONS; ++i) {
            printf("Iter %i\n", i);
            struct calc {
                int (*calc)(int);
            };

            const char *calcName = "calc";
            calc svc{};
            svc.calc = [](int n) -> int {
                return n * 42;
            };

            celix_service_use_options_t opts{};
            opts.filter.serviceName = "calc";
            if(direct) {
                opts.flags = CELIX_SERVICE_USE_DIRECT;
            }

            bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
            EXPECT_FALSE(called); //service not avail.

            std::future<bool> result{std::async([&] {
                opts.waitTimeoutInSeconds = 2.0;
                //printf("Trying to call calc with timeout of %f\n", opts.waitTimeoutInSeconds);
                bool calledAsync = celix_bundleContext_useServiceWithOptions(ctx, &opts);
                //printf("returned from use service with timeout. calc called %s.\n", calledAsync ? "true" : "false");
                return calledAsync;
            })};
            long svcId = celix_bundleContext_registerServiceAsync(ctx, &svc, calcName, nullptr);
            EXPECT_TRUE(svcId >= 0);


            EXPECT_TRUE(result.get()); //should return true after waiting for the registered service.


            celix_bundleContext_unregisterServiceAsync(ctx, svcId, nullptr, nullptr);
            celix_bundleContext_waitForAsyncUnregistration(ctx, svcId);


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
};

TEST_F(CelixBundleContextServicesTestSuite, RegisterServiceTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId >= 0);
    celix_bundleContext_unregisterService(ctx, svcId);
};

TEST_F(CelixBundleContextServicesTestSuite, TegisterServiceAsyncTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId = celix_bundleContext_registerServiceAsync(ctx, &svc, calcName, nullptr);
    ASSERT_TRUE(svcId >= 0);
    celix_bundleContext_waitForAsyncRegistration(ctx, svcId);
    ASSERT_GE(celix_bundleContext_findService(ctx, calcName), 0L);

    celix_bundleContext_unregisterServiceAsync(ctx, svcId, nullptr, nullptr);
    celix_bundleContext_waitForAsyncUnregistration(ctx, svcId);
    ASSERT_LT(celix_bundleContext_findService(ctx, calcName), 0L);
};

TEST_F(CelixBundleContextServicesTestSuite, IncorrectUnregisterCallsTest) {
    celix_bundleContext_unregisterService(ctx, 1);
    celix_bundleContext_unregisterService(ctx, 2);
    celix_bundleContext_unregisterService(ctx, -1);
    celix_bundleContext_unregisterService(ctx, -2);
};

TEST_F(CelixBundleContextServicesTestSuite, IncorrectAsyncUnregisterCallsTset) {
    celix_bundleContext_unregisterServiceAsync(ctx, 1, nullptr, nullptr);
    celix_bundleContext_unregisterServiceAsync(ctx, 2, nullptr, nullptr);
    celix_bundleContext_unregisterServiceAsync(ctx, -1, nullptr, nullptr);
    celix_bundleContext_unregisterServiceAsync(ctx, -2, nullptr, nullptr);
};

TEST_F(CelixBundleContextServicesTestSuite, UseServicesWithoutNameTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId1 >= 0);

    auto use = [](void *handle, void *svc) {
        EXPECT_TRUE(svc != nullptr);
        int *total =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(1);
        *total += tmp;
    };

    int total = 0;
    auto count = celix_bundleContext_useServices(ctx, nullptr, &total, use);
    EXPECT_EQ(0, count);
    count = celix_bundleContext_useServices(ctx, calcName, &total, use);
    EXPECT_EQ(1, count);
    bool called = celix_bundleContext_useService(ctx, nullptr, &total, use);
    EXPECT_FALSE(called);
    called = celix_bundleContext_useService(ctx, calcName, &total, use);
    EXPECT_TRUE(called);
    called = celix_bundleContext_useServiceWithId(ctx, svcId1, nullptr, &total, use);
    EXPECT_FALSE(called);

    celix_service_use_options_t use_opts{};
    use_opts.filter.serviceName = nullptr;
    use_opts.filter.versionRange = nullptr;
    use_opts.callbackHandle = &total;
    use_opts.use = use;
    called = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
    ASSERT_FALSE(called);
    count = celix_bundleContext_useServicesWithOptions(ctx, &use_opts);
    EXPECT_EQ(0, count);

    use_opts.filter.serviceName = calcName;
    called = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
    ASSERT_TRUE(called);
    count = celix_bundleContext_useServicesWithOptions(ctx, &use_opts);
    EXPECT_EQ(1, count);

    celix_bundleContext_unregisterService(ctx, svcId1);
}

TEST_F(CelixBundleContextServicesTestSuite, TegisterMultipleAndUseServicesTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId1 >= 0);

    long svcId2 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId2 >= 0);

    long svcId3 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId3 >= 0);

    auto use = [](void *handle, void *svc) {
        EXPECT_TRUE(svc != nullptr);
        int *total =  static_cast<int*>(handle);
        struct calc *calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(1);
        *total += tmp;
    };

    int total = 0;
    auto count = celix_bundleContext_useServices(ctx, "calc", &total, use);
    EXPECT_EQ(3, count);
    EXPECT_EQ(42 * 3, total);

    celix_service_use_options_t use_opts{};
    use_opts.filter.serviceName = calcName;
    use_opts.filter.versionRange = nullptr;
    use_opts.callbackHandle = &total;
    use_opts.use = use;
    use_opts.flags = 0;
    total = 0;
    count = celix_bundleContext_useServicesWithOptions(ctx, &use_opts);
    EXPECT_EQ(3, count);
    EXPECT_EQ(42 * 3, total);

    use_opts.flags = CELIX_SERVICE_USE_DIRECT;
    total = 0;
    count = celix_bundleContext_useServicesWithOptions(ctx, &use_opts);
    EXPECT_EQ(3, count);
    EXPECT_EQ(42 * 3, total);

    celix_bundleContext_unregisterService(ctx, svcId3);
    total = 0;
    count = celix_bundleContext_useServices(ctx, "calc", &total, use);
    EXPECT_EQ(2, count);
    EXPECT_EQ(42 * 2, total);

    use_opts.flags = 0;
    total = 0;
    count = celix_bundleContext_useServicesWithOptions(ctx, &use_opts);
    EXPECT_EQ(2, count);
    EXPECT_EQ(42 * 2, total);

    use_opts.flags = CELIX_SERVICE_USE_DIRECT;
    total = 0;
    count = celix_bundleContext_useServicesWithOptions(ctx, &use_opts);
    EXPECT_EQ(2, count);
    EXPECT_EQ(42 * 2, total);

    total = 0;
    bool called = celix_bundleContext_useService(ctx, "calc", &total, use);
    EXPECT_TRUE(called);
    EXPECT_EQ(42, total);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);

    //NOTE superfluous (note prints errors)
    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
};

TEST_F(CelixBundleContextServicesTestSuite, UseServiceInUseCallbackTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
    svc.calc = [](int n) -> int {
        return n * 42;
    };

    long svcId1 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId1 >= 0);

    long svcId2 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId2 >= 0);

    long svcId3 = celix_bundleContext_registerService(ctx, &svc, calcName, nullptr);
    EXPECT_TRUE(svcId3 >= 0);

    auto use = [](void *handle, void *svc) {
        celix_bundle_context_t *ctx = static_cast<celix_bundle_context_t *>(handle);
        celix_service_use_options_t opts{};
        opts.filter.serviceName = "calc";
        opts.callbackHandle = svc; //avoid warning
        opts.use = nullptr;
        size_t count = celix_bundleContext_useServicesWithOptions(ctx, &opts);
        EXPECT_EQ(3, count);
    };

    celix_service_use_options_t opts{};
    opts.filter.serviceName = calcName;
    opts.callbackHandle = ctx;
    opts.use = use;
    opts.flags = 0;
    bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    EXPECT_TRUE(called);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId3);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
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
        auto* calc = static_cast<struct calc*>(svc);
        int tmp = calc->calc(2);
        *result = tmp;
    });
    ASSERT_TRUE(!called);
    ASSERT_EQ(0, result); //e.g. not called

    celix_bundleContext_unregisterService(ctx, svcId);
};

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceWithTimeoutTest) {
    registerAndUseServiceWithTimeout(false);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceDirectWithTimeoutTest) {
    registerAndUseServiceWithTimeout(true);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAsyncAndUseServiceWithTimeoutTest) {
    registerAsyncAndUseServiceWithTimeout(false);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAsyncAndUseServiceDirectWithTimeoutTest) {
    registerAsyncAndUseServiceWithTimeout(true);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceWithCorrectVersionTest) {
    registerAndUseServiceWithCorrectVersion(false);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceDirectWithCorrectVersionTest) {
    registerAndUseServiceWithCorrectVersion(true);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceWithIncorrectVersionTest) {
    registerAndUseServiceWithIncorrectVersion(false);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseServiceDirectWithIncorrectVersionTest) {
    registerAndUseServiceWithIncorrectVersion(true);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterAndUseWithForcedRaceConditionTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
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

        auto* h = static_cast<struct sync*>(handle);

        std::cout << "setting isUseCall to true and syncing on readyToExitUseCall" << std::endl;
        std::unique_lock<std::mutex> lock(h->mutex);
        h->inUseCall = true;
        h->sync.notify_all();
        h->sync.wait(lock, [h]{return h->readyToExitUseCall;});
        lock.unlock();

        std::cout << "Calling calc " << std::endl;
        auto* calc = static_cast<struct calc *>(svc);
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


TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerTest) {
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

    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "calc";
    opts.callbackHandle = &count;
    opts.add = add;
    opts.remove = remove;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
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

TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerTestAsync) {
    std::atomic<int> count {0};
    auto add = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        auto *c = static_cast<std::atomic<int>*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc) {
        ASSERT_TRUE(svc != nullptr);
        auto *c = static_cast<std::atomic<int>*>(handle);
        *c -= 1;
    };

    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "calc";
    opts.callbackHandle = &count;
    opts.add = add;
    opts.remove = remove;
    long trackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0);
    ASSERT_EQ(0, count);

    long svcId1 = celix_bundleContext_registerServiceAsync(ctx, (void*)0x100, "calc", nullptr);
    celix_bundleContext_waitForAsyncRegistration(ctx, svcId1); //note also means the service tracker is done
    ASSERT_TRUE(svcId1 >= 0);
    ASSERT_EQ(1, count);

    long svcId2 = celix_bundleContext_registerServiceAsync(ctx, (void*)0x200, "calc", nullptr);
    celix_bundleContext_waitForAsyncRegistration(ctx, svcId2);
    ASSERT_TRUE(svcId2 >= 0);
    ASSERT_EQ(2, count);

    celix_bundleContext_unregisterServiceAsync(ctx, svcId1, nullptr, nullptr);
    celix_bundleContext_waitForAsyncUnregistration(ctx, svcId1);
    ASSERT_EQ(1, count);

    celix_bundleContext_stopTrackerAsync(ctx, trackerId, nullptr, nullptr);
    celix_bundleContext_unregisterServiceAsync(ctx, svcId2, nullptr, nullptr);

    celix_framework_waitForEmptyEventQueue(fw);
}

TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerInvalidArgsTest) {
    long trackerId = celix_bundleContext_trackServices(nullptr, nullptr);
    ASSERT_TRUE(trackerId < 0); //required ctx missing
    trackerId = celix_bundleContext_trackServices(ctx, "calc");
    ASSERT_TRUE(trackerId >= 0); //valid
    celix_bundleContext_stopTracker(ctx, trackerId);


    //opts
    trackerId = celix_bundleContext_trackServicesWithOptions(nullptr, nullptr);
    ASSERT_TRUE(trackerId < 0); //required ctx and opts missing
    trackerId = celix_bundleContext_trackServicesWithOptions(ctx, nullptr);
    ASSERT_TRUE(trackerId < 0); //required opts missing
    celix_service_tracking_options_t opts{};
    trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0); //valid with empty opts
    celix_bundleContext_stopTracker(ctx, trackerId);

    opts.filter.serviceName = "calc";
    trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0); //valid
    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerTestWithAlreadyRegisteredServices) {
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



    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "calc";
    opts.callbackHandle = &count;
    opts.add = add;
    opts.remove = remove;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
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

TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerTestWithProperties) {
    int count = 0;
    int count2 = 0;
    auto add = [](void *handle, void *svc, const celix_properties_t *props) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_TRUE(props != nullptr);
        int *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc, const celix_properties_t *props) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_TRUE(props != nullptr);
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

    celix_service_tracking_options_t opts2{};
    opts2.filter.serviceName = "calc";
    opts2.callbackHandle = &count2;
    opts2.addWithProperties = nullptr;
    opts2.removeWithProperties = remove;
    long trackerId2 = celix_bundleContext_trackServicesWithOptions(ctx, &opts2);
    ASSERT_TRUE(trackerId2 >= 0);
    ASSERT_EQ(0, count2);

    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x200, "calc", nullptr);
    ASSERT_TRUE(svcId1 >= 0);
    ASSERT_EQ(2, count);
    ASSERT_EQ(0, count2);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    ASSERT_EQ(0, count);
    ASSERT_EQ(-2, count2);

    celix_bundleContext_stopTracker(ctx, trackerId2);
    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerTestWithOwner) {
    int count = 0;
    auto add = [](void *handle, void *svc, const celix_properties_t *props, const bundle_t *svcOwner) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_TRUE(props != nullptr);
        ASSERT_TRUE(celix_bundle_getId(svcOwner) >= 0);
        int *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, void *svc, const celix_properties_t *props, const bundle_t *svcOwner) {
        ASSERT_TRUE(svc != nullptr);
        ASSERT_TRUE(props != nullptr);
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

TEST_F(CelixBundleContextServicesTestSuite, ServiceTrackerWithRaceConditionTest) {
    struct calc {
        int (*calc)(int);
    };

    const char *calcName = "calc";
    calc svc{};
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

        auto *d = static_cast<struct data*>(handle);

        std::unique_lock<std::mutex> lock(d->mutex);
        d->inAddCall = true;
        d->sync.notify_all();
        d->sync.wait(lock, [d]{return d->readyToExit;});
        lock.unlock();

        auto *calc = static_cast<struct calc *>(svc);
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

    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "calc";
    opts.callbackHandle = &data;
    opts.add = add;
    opts.remove = remove;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);

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

TEST_F(CelixBundleContextServicesTestSuite, UseServiceDoesNotBlockInEventLoop) {
    void *svc1 = (void*)0x100;

    auto set = [](void *handle, void */*svc*/) {
        celix_bundle_context_t *ctx = static_cast<celix_bundle_context_t *>(handle);
        celix_service_use_options_t use_opts{};
        use_opts.filter.serviceName = "NotAvailable";
        use_opts.waitTimeoutInSeconds = 3600; // unacceptable long blocking
        use_opts.callbackHandle = nullptr;
        use_opts.use =  [](void *handle, void *svc) {
            FAIL() << "We shouldn't get here: (" << handle << "," << svc << ")";
        };

        bool called = celix_bundleContext_useServiceWithOptions(ctx, &use_opts);
        ASSERT_FALSE(called);
    };

    long svcId1 = celix_bundleContext_registerService(ctx, svc1, "NA", nullptr);
    ASSERT_GE(svcId1, 0);

    celix_service_tracking_options_t opts{};
    opts.callbackHandle = (void*)ctx;
    opts.filter.serviceName = "NA";
    opts.set = set;
    long trackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    ASSERT_TRUE(trackerId >= 0);

    void *svc2 = (void*)0x200; //no ranking
    long svcId2 = celix_bundleContext_registerServiceAsync(ctx, svc2, "NotAvailable", nullptr);
    ASSERT_GE(svcId2, 0);
    celix_bundleContext_waitForAsyncTracker(ctx, trackerId);
    celix_bundleContext_waitForAsyncRegistration(ctx, svcId2);

    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_stopTracker(ctx, trackerId);
    celix_bundleContext_unregisterService(ctx, svcId1);
}

TEST_F(CelixBundleContextServicesTestSuite, ServicesTrackerSetTest) {
    int count = 0;

    void *svc1 = (void*)0x100; //no ranking
    void *svc2 = (void*)0x200; //no ranking
    void *svc3 = (void*)0x300; //10 ranking
    void *svc4 = (void*)0x400; //5 ranking

    auto set = [](void *handle, void *svc) {
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
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts); //call 1
    ASSERT_TRUE(trackerId >= 0);

    //register svc3 should lead to second set call
    celix_properties_t *props3 = celix_properties_create();
    celix_properties_set(props3, CELIX_FRAMEWORK_SERVICE_RANKING, "10");
    long svcId3 = celix_bundleContext_registerService(ctx, svc3, "NA", props3); //call 2

    //register svc4 should lead to no set (lower ranking)
    celix_properties_t *props4 = celix_properties_create();
    celix_properties_set(props4, CELIX_FRAMEWORK_SERVICE_RANKING, "10");
    long svcId4 = celix_bundleContext_registerService(ctx, svc4, "NA", props4); //no update

    //unregister svc3 should lead to set (new highest ranking)
    celix_bundleContext_unregisterService(ctx, svcId3); //call 3

    celix_bundleContext_stopTracker(ctx, trackerId); //call 4 (nullptr)
    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId4);

    ASSERT_EQ(4, count); //check if the set is called the expected times
}

TEST_F(CelixBundleContextServicesTestSuite, TrackerOfAllServicesSetTest) {
    int count = 0;

    void *svc1 = (void*)0x100; //no ranking
    void *svc2 = (void*)0x200; //no ranking
    void *svc3 = (void*)0x300; //10 ranking
    void *svc4 = (void*)0x400; //5 ranking

    auto set = [](void *handle, void *svc) {
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
    opts.filter.serviceName = nullptr;
    opts.set = set;
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts); //call 1
    ASSERT_TRUE(trackerId >= 0);

    //register svc3 should lead to second set call
    celix_properties_t *props3 = celix_properties_create();
    celix_properties_set(props3, CELIX_FRAMEWORK_SERVICE_RANKING, "10");
    long svcId3 = celix_bundleContext_registerService(ctx, svc3, "NA", props3); //call 2

    //register svc4 should lead to no set (lower ranking)
    celix_properties_t *props4 = celix_properties_create();
    celix_properties_set(props4, CELIX_FRAMEWORK_SERVICE_RANKING, "10");
    long svcId4 = celix_bundleContext_registerService(ctx, svc4, "NA", props4); //no update

    //unregister svc3 should lead to set (new highest ranking)
    celix_bundleContext_unregisterService(ctx, svcId3); //call 3

    celix_bundleContext_stopTracker(ctx, trackerId); //call 4 (nullptr)
    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId4);

    ASSERT_EQ(4, count); //check if the set is called the expected times
}

TEST_F(CelixBundleContextServicesTestSuite, TrackAllServices) {
    std::atomic<size_t> count{0};

    void *svc1 = (void *) 0x100; //no ranking
    void *svc2 = (void *) 0x200; //no ranking
    void *svc3 = (void *) 0x300; //10 ranking
    void *svc4 = (void *) 0x400; //5 ranking

    long svcId1 = celix_bundleContext_registerService(ctx, svc1, "svc_type1", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, svc2, "svc_type1", nullptr);
    long svcId3 = celix_bundleContext_registerService(ctx, svc3, "svc_type2", nullptr);
    long svcId4 = celix_bundleContext_registerService(ctx, svc4, "svc_type2", nullptr);

    celix_service_tracking_options_t opts{};
    opts.callbackHandle = (void *) &count;
    opts.filter.serviceName = nullptr;
    opts.callbackHandle = (void *) &count;
    opts.add = [](void *handle, void *) {
        auto c = (std::atomic<size_t> *) handle;
        c->fetch_add(1);
    };
    long trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    EXPECT_GE(trackerId, 0);
    EXPECT_EQ(4, count.load());

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId3);
    celix_bundleContext_unregisterService(ctx, svcId4);
    celix_bundleContext_stopTracker(ctx, trackerId);
}

TEST_F(CelixBundleContextServicesTestSuite, MetaTrackAllServiceTrackers) {
    std::atomic<size_t> count{0};
    auto add = [](void *handle, const celix_service_tracker_info_t*) {
        auto *c = (std::atomic<size_t>*)handle;
        c->fetch_add(1);
    };
    long trkId1 = celix_bundleContext_trackServiceTrackers(ctx, nullptr, (void*)&count, add, nullptr);
    EXPECT_TRUE(trkId1 >= 0);

    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "service1";
    long trkId2 = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    EXPECT_TRUE(trkId2 >= 0);

    opts.filter.serviceName = "service2";
    long trkId3 = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    EXPECT_TRUE(trkId3 >= 0);

    EXPECT_EQ(2, count.load());

    celix_bundleContext_stopTracker(ctx, trkId1);
    celix_bundleContext_stopTracker(ctx, trkId2);
    celix_bundleContext_stopTracker(ctx, trkId3);
}

TEST_F(CelixBundleContextServicesTestSuite, MetaTrackServiceTrackersFromOtherBundle) {
    long bndId = celix_bundleContext_installBundle(ctx, CMP_TEST_BND_LOC, true);
    ASSERT_TRUE(bndId >= 0);
    long bndIdInAdd = -1;
    long bndIdInRemove = -1;
    auto add = [](void *handle, const celix_service_tracker_info_t* info) {
        long * bid= static_cast<long *>(handle);
        *bid = info->bundleId;
    };
    auto remove = [](void *handle, const celix_service_tracker_info_t *info) {
        long * bid= static_cast<long *>(handle);
        *bid = info->bundleId;
    };
    long trkId1 = celix_bundleContext_trackServiceTrackers(ctx, nullptr, &bndIdInAdd, add, nullptr);
    EXPECT_TRUE(trkId1 >= 0);
    long trkId2 = celix_bundleContext_trackServiceTrackers(ctx, nullptr, &bndIdInRemove, nullptr, remove);
    EXPECT_TRUE(trkId2 >= 0);
    EXPECT_EQ(bndIdInAdd, bndId);
    EXPECT_EQ(bndIdInRemove, -1);
    EXPECT_TRUE(celix_bundleContext_stopBundle(ctx, bndId));
    EXPECT_EQ(bndIdInRemove, bndId);
    bndIdInAdd = -1;
    EXPECT_TRUE(celix_bundleContext_startBundle(ctx, bndId));
    EXPECT_EQ(bndIdInAdd, bndId);
    bndIdInRemove = -1;
    celix_bundleContext_stopTracker(ctx, trkId1);
    celix_bundleContext_stopTracker(ctx, trkId2);
    EXPECT_EQ(bndIdInRemove, bndId);
}

TEST_F(CelixBundleContextServicesTestSuite, ServiceFactoryTest) {
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


TEST_F(CelixBundleContextServicesTestSuite, AsyncServiceFactoryTest) {
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

    long facId = celix_bundleContext_registerServiceFactoryAsync(ctx, &fac, name, nullptr);
    ASSERT_TRUE(facId >= 0);
    celix_bundleContext_waitForAsyncRegistration(ctx, facId);


    int result = -1;
    bool called = celix_bundleContext_useService(ctx, name, &result, [](void *handle, void* svc) {
        auto *r = (int *)(handle);
        auto *calc = (struct calc*)svc;
        *r = calc->calc(2);
    });
    ASSERT_TRUE(called);
    ASSERT_EQ(84, result);
    ASSERT_EQ(2, count); //expecting getService & unGetService to be called during the useService call.


    celix_bundleContext_unregisterServiceAsync(ctx, facId, nullptr, nullptr);
}

TEST_F(CelixBundleContextServicesTestSuite, FindServicesTest) {
    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x100, "example", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x100, "example", nullptr);
    long svcId3 = celix_bundleContext_registerService(ctx, (void*)0x100, "example", nullptr);
    long svcId4 = celix_bundleContext_registerService(ctx, (void*)0x100, "example", nullptr);


    long foundId = celix_bundleContext_findService(ctx, "non existing service name");
    ASSERT_EQ(-1L, foundId);

    foundId = celix_bundleContext_findService(ctx, "example");
    ASSERT_EQ(foundId, svcId1); //oldest should have highest ranking

    celix_array_list_t *list = celix_bundleContext_findServices(ctx, "non existing service name");
    ASSERT_EQ(0, celix_arrayList_size(list));
    celix_arrayList_destroy(list);

    list = celix_bundleContext_findServices(ctx, "example");
    ASSERT_EQ(4, celix_arrayList_size(list));
    celix_arrayList_destroy(list);

    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId3);
    celix_bundleContext_unregisterService(ctx, svcId4);

    celix_service_filter_options_t opts{};
    opts.serviceName = "example";
    foundId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
    ASSERT_EQ(foundId, svcId2); //only one left

    celix_bundleContext_unregisterService(ctx, svcId2);
}

TEST_F(CelixBundleContextServicesTestSuite, TrackServiceTrackerTest) {

    int count = 0;

    auto add = [](void *handle, const celix_service_tracker_info_t *info) {
        EXPECT_STRCASEEQ("example", info->serviceName);
        auto *c = static_cast<int*>(handle);
        *c += 1;
    };
    auto remove = [](void *handle, const celix_service_tracker_info_t *info) {
        EXPECT_STRCASEEQ("example", info->serviceName);
        auto *c = static_cast<int*>(handle);
        *c -= 1;
    };

    long trackerId = celix_bundleContext_trackServiceTrackers(ctx, "example", &count, add, remove);
    EXPECT_TRUE(trackerId >= 0);
    EXPECT_EQ(0, count);

    long tracker2 = celix_bundleContext_trackServices(ctx, "example");
    EXPECT_TRUE(tracker2 >= 0);
    EXPECT_EQ(1, count);

    long tracker3 = celix_bundleContext_trackServices(ctx, "example");
    EXPECT_TRUE(tracker3 >= 0);
    EXPECT_EQ(2, count);

    long tracker4 = celix_bundleContext_trackServices(ctx, "no-match");
    EXPECT_TRUE(tracker4 >= 0);
    EXPECT_EQ(2, count);

    celix_bundleContext_stopTracker(ctx, tracker2);
    EXPECT_EQ(1, count);
    celix_bundleContext_stopTracker(ctx, tracker3);
    EXPECT_EQ(0, count);

    celix_bundleContext_stopTracker(ctx, trackerId);
    celix_bundleContext_stopTracker(ctx, tracker4);
}

TEST_F(CelixBundleContextServicesTestSuite, FloodEventLoopTest) {
    struct callback_data {
        std::mutex mutex{};
        std::condition_variable cond{};
        bool ready{false};
    };
    callback_data data{};

    //test so that the framework needs to use dynamic allocated event on the event loop
    celix_service_registration_options_t opts{};
    opts.svc = (void*)0x42;
    opts.serviceName = "test";
    opts.asyncData = (void*)&data;
    opts.asyncCallback = [](void *d, long /*svcId*/) {
        auto *localData = static_cast<callback_data*>(d);
        std::unique_lock<std::mutex> lck{localData->mutex};
        localData->cond.wait_for(lck, std::chrono::seconds{30}, [&]{ return localData->ready; }); //wait til ready.
        EXPECT_TRUE(localData->ready);
    };
    long svcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    EXPECT_GE(svcId, 0);

    int nrOfAdditionalRegistrations = 300;
    std::vector<long> svcIds{};
    std::vector<long> trackerIds{};
    for (int i = 0; i < nrOfAdditionalRegistrations; ++i) {
        long id = celix_bundleContext_registerServiceAsync(ctx, (void*)0x42, "test", nullptr); //note cannot be completed because the first service registration in blocking in the event loop.
        EXPECT_GE(id, 0);
        svcIds.push_back(id);
        trackerIds.push_back(celix_bundleContext_trackServicesAsync(ctx, "test"));

        //CHECK if celix_bundleContext_isServiceRegistered work
        EXPECT_FALSE(celix_bundleContext_isServiceRegistered(ctx, id));
    }

    {
        //let the first service registration continue and as result all the following registrations.
        std::lock_guard<std::mutex> lck{data.mutex};
        data.ready = true;
        data.cond.notify_all();
    }

    celix_bundleContext_waitForAsyncRegistration(ctx, svcId);
    EXPECT_TRUE(celix_bundleContext_isServiceRegistered(ctx, svcId));
    long foundId = celix_bundleContext_findService(ctx, "test");
    EXPECT_GE(foundId, 0);

    celix_bundleContext_unregisterServiceAsync(ctx, svcId, nullptr, nullptr);
    for (auto id : svcIds) {
        celix_bundleContext_unregisterServiceAsync(ctx, id, nullptr, nullptr);
        celix_bundleContext_findService(ctx, "test"); //just to add some entropy
    }
    for (auto id : trackerIds) {
        celix_bundleContext_stopTrackerAsync(ctx, id, nullptr, nullptr);
    }
}


TEST_F(CelixBundleContextServicesTestSuite, ServiceOnDemandWithAsyncRegisterTest) {
    //NOTE that even though service are registered async, they should be found by a useService call.

    bool called = celix_bundleContext_useService(ctx, "test", nullptr, [](void*, void*){/*nop*/});
    EXPECT_FALSE(called); //service not available

    struct test_service {
        void* handle;
    };

    struct callback_data {
        celix_bundle_context_t* ctx;
        long svcId;
        test_service ts;
    };
    callback_data cbData{ctx, -1L, {nullptr}};

    long trkId = celix_bundleContext_trackServiceTrackers(ctx, "test", &cbData, [](void *voidData, const celix_service_tracker_info_t*) {
        auto* data = static_cast<callback_data*>(voidData);
        data->svcId = celix_bundleContext_registerServiceAsync(data->ctx, &data->ts, "test", nullptr);
    }, nullptr);

    called = celix_bundleContext_useService(ctx, "test", nullptr, nullptr);
    EXPECT_TRUE(called); //service created on demand.

    celix_bundleContext_unregisterService(ctx, cbData.svcId);
    celix_bundleContext_stopTracker(ctx, trkId);
}

TEST_F(CelixBundleContextServicesTestSuite, UseServiceOnDemandDirectlyWithAsyncRegisterTest) {
    //NOTE that even though service are registered async, they should be found by a useService call.

    bool called = celix_bundleContext_useService(ctx, "test", nullptr, [](void*, void*){/*nop*/});
    EXPECT_FALSE(called); //service not available

    struct test_service {
        void* handle;
    };

    struct callback_data {
        celix_bundle_context_t* ctx;
        long svcId;
        test_service ts;
    };
    callback_data cbData{ctx, -1L, {nullptr}};

    long trkId = celix_bundleContext_trackServiceTrackers(ctx, "test", &cbData, [](void *voidData, const celix_service_tracker_info_t*) {
        auto* data = static_cast<callback_data*>(voidData);
        data->svcId = celix_bundleContext_registerServiceAsync(data->ctx, &data->ts, "test", nullptr);
    }, nullptr);
    celix_service_use_options_t opts{};
    opts.filter.serviceName = "test";
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    EXPECT_TRUE(called); //service created on demand.

    celix_bundleContext_unregisterService(ctx, cbData.svcId);
    celix_bundleContext_stopTracker(ctx, trkId);
}

TEST_F(CelixBundleContextServicesTestSuite, UseServicesOnDemandDirectlyWithAsyncRegisterTest) {
    //NOTE that even though service are registered async, they should be found by a useService call.

    bool called = celix_bundleContext_useService(ctx, "test", nullptr, [](void*, void*){/*nop*/});
    EXPECT_FALSE(called); //service not available

    struct test_service {
        void* handle;
    };

    struct callback_data {
        celix_bundle_context_t* ctx;
        long svcId;
        test_service ts;
    };
    callback_data cbData{ctx, -1L, {nullptr}};

    long trkId = celix_bundleContext_trackServiceTrackers(ctx, "test", &cbData, [](void *voidData, const celix_service_tracker_info_t*) {
        auto* data = static_cast<callback_data*>(voidData);
        data->svcId = celix_bundleContext_registerServiceAsync(data->ctx, &data->ts, "test", nullptr);
    }, nullptr);

    callback_data cbData1{ctx, -1L, {nullptr}};
    long trkId1 = celix_bundleContext_trackServiceTrackers(ctx, "test", &cbData1, [](void *voidData, const celix_service_tracker_info_t*) {
        auto* data = static_cast<callback_data*>(voidData);
        data->svcId = celix_bundleContext_registerServiceAsync(data->ctx, &data->ts, "test", nullptr);
    }, nullptr);

    celix_service_use_options_t opts{};
    opts.filter.serviceName = "test";
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    size_t count = celix_bundleContext_useServicesWithOptions(ctx, &opts);
    EXPECT_EQ(2, count);

    celix_bundleContext_unregisterService(ctx, cbData.svcId);
    celix_bundleContext_unregisterService(ctx, cbData1.svcId);
    celix_bundleContext_stopTracker(ctx, trkId);
    celix_bundleContext_stopTracker(ctx, trkId1);
}

TEST_F(CelixBundleContextServicesTestSuite, StartStopServiceTrackerAsyncTest) {
    std::atomic<int> count{0};

    auto cb = [](void* data) {
        auto* c = static_cast<std::atomic<int>*>(data);
        (*c)++;
    };

    celix_service_tracking_options_t opts{};
    opts.trackerCreatedCallbackData = &count;
    opts.trackerCreatedCallback = cb;
    long trkId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    EXPECT_GE(trkId, 0);
    celix_bundleContext_waitForAsyncTracker(ctx, trkId);
    EXPECT_EQ(count.load(), 1); //1x tracker started

    celix_bundleContext_stopTrackerAsync(ctx, trkId, &count, cb);
    celix_bundleContext_waitForAsyncStopTracker(ctx, trkId);
    EXPECT_EQ(2, count.load()); //1x tracker started, 1x tracker stopped
}

TEST_F(CelixBundleContextServicesTestSuite, StartStopMetaServiceTrackerAsyncTest) {
    std::atomic<int> count{0};

    auto cb = [](void* data) {
        auto* c = static_cast<std::atomic<int>*>(data);
        (*c)++;
    };

    long trkId = celix_bundleContext_trackServiceTrackersAsync(ctx, "test", nullptr, nullptr, nullptr, &count, cb);
    EXPECT_GE(trkId, 0);
    celix_bundleContext_waitForAsyncTracker(ctx, trkId);
    EXPECT_EQ(count.load(), 1); //1x tracker started

    celix_bundleContext_stopTrackerAsync(ctx, trkId, &count, cb);
    celix_bundleContext_waitForAsyncStopTracker(ctx, trkId);
    EXPECT_EQ(2, count.load()); //1x tracker started, 1x tracker stopped
}

TEST_F(CelixBundleContextServicesTestSuite, OnlyCallAsyncCallbackWithAsyncApiTest) {
    celix_service_tracking_options_t opts{};
    opts.trackerCreatedCallback = [](void *) {
        FAIL();
    };
    long trkId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    EXPECT_GT(trkId, 0);
    celix_bundleContext_stopTracker(ctx, trkId);

    celix_service_registration_options_t opts2{};
    opts2.serviceName = "test";
    opts2.svc = (void*)0x42;
    opts2.asyncCallback = [](void*, long) {
        FAIL();
    };
    long svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts2);
    EXPECT_GT(svcId, 0);
    celix_bundleContext_waitForEvents(ctx);
    celix_bundleContext_unregisterService(ctx, trkId);

    celix_bundle_tracking_options_t opts3{};
    opts3.trackerCreatedCallback = [](void *) {
        FAIL();
    };
    trkId = celix_bundleContext_trackBundlesWithOptions(ctx, &opts3);
    EXPECT_GT(trkId, 0);
    celix_bundleContext_stopTracker(ctx, trkId);
}

TEST_F(CelixBundleContextServicesTestSuite, UnregisterSvcBeforeAsyncRegistrationTest) {
    struct callback_data {
        std::atomic<int> count{};
        celix_bundle_context_t* ctx{nullptr};
    };
    callback_data cbData{};
    cbData.ctx = ctx;

    celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "registerAsync",
            (void*)&cbData,
            [](void *data) {
                auto cbd = static_cast<struct callback_data*>(data);

                //note register async. So a event on the event queue, but because this is done on the event queue this cannot be completed
                long svcId = celix_bundleContext_registerServiceAsync(cbd->ctx, (void*)0x42, "test-service", nullptr);

                celix_bundleContext_unregisterService(cbd->ctx, svcId); //trying to unregister still queued svc registration -> should cancel event.
            },
            nullptr,
            nullptr);

    celix_bundleContext_waitForEvents(ctx);
    long svcId = celix_bundleContext_findService(ctx, "test-service");
    EXPECT_LT(svcId, 0);
    EXPECT_EQ(0, cbData.count.load()); //note create tracker canceled -> no callback
}

TEST_F(CelixBundleContextServicesTestSuite, StopSvcTrackerBeforeAsyncTrackerIsCreatedTest) {
    struct callback_data {
        std::atomic<int> count{};
        celix_bundle_context_t* ctx{nullptr};
    };
    callback_data cbData{};
    cbData.ctx = ctx;

    celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "create tracker async",
            (void*)&cbData,
            [](void *data) {
                auto cbd = static_cast<struct callback_data*>(data);

                celix_service_tracking_options_t opts{};
                opts.filter.serviceName = "test-service";
                opts.trackerCreatedCallbackData = data;
                opts.trackerCreatedCallback = [](void *data) {
                    auto* cbd = static_cast<struct callback_data*>(data);
                    cbd->count.fetch_add(1);
                };

                //note create async. So a event on the event queue, but because this is done on the event queue this cannot be completed
                long trkId = celix_bundleContext_trackServicesWithOptionsAsync(cbd->ctx, &opts);

                celix_bundleContext_stopTracker(cbd->ctx, trkId);
            },
            nullptr,
            nullptr);

    celix_bundleContext_waitForEvents(ctx);
    EXPECT_EQ(0, cbData.count.load()); //note create tracker canceled -> no callback
}

TEST_F(CelixBundleContextServicesTestSuite, WaitForTrackerOnLoop) {
    struct callback_data {
        std::atomic<int> count{};
        celix_bundle_context_t* ctx{nullptr};
    };
    callback_data cbData{};
    cbData.ctx = ctx;

    celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "create tracker async",
            (void*)&cbData,
            [](void *data) {
                auto cbd = static_cast<struct callback_data*>(data);

                celix_service_tracking_options_t opts{};
                opts.filter.serviceName = "test-service";
                opts.trackerCreatedCallbackData = data;
                opts.trackerCreatedCallback = [](void *data) {
                    auto* cbd = static_cast<struct callback_data*>(data);
                    cbd->count.fetch_add(1);
                };
                long trkId = celix_bundleContext_trackServicesWithOptions(cbd->ctx, &opts);
                celix_bundleContext_waitForAsyncTracker(cbd->ctx, trkId);
                celix_bundleContext_stopTracker(cbd->ctx, trkId);
            },
            nullptr,
            nullptr);

    celix_bundleContext_waitForEvents(ctx);
}

TEST_F(CelixBundleContextServicesTestSuite, WaitForNonexistingTracker) {
    celix_bundleContext_waitForAsyncTracker(ctx, 111L /* non-existing tracker */);
}

TEST_F(CelixBundleContextServicesTestSuite, StopBundleTrackerBeforeAsyncTrackerIsCreatedTest) {
    struct callback_data {
        std::atomic<int> count{};
        celix_bundle_context_t* ctx{nullptr};
    };
    callback_data cbData{};
    cbData.ctx = ctx;

    celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "create tracker async",
            (void*)&cbData,
            [](void *data) {
                auto cbd = static_cast<struct callback_data*>(data);

                celix_bundle_tracking_options_t opts{};
                opts.trackerCreatedCallbackData = data;
                opts.trackerCreatedCallback = [](void *data) {
                    auto* cbd = static_cast<struct callback_data*>(data);
                    cbd->count.fetch_add(1);
                };

                //note create async. So a event on the event queue, but because this is done on the event queue this cannot be completed
                long trkId = celix_bundleContext_trackBundlesWithOptionsAsync(cbd->ctx, &opts);

                celix_bundleContext_stopTracker(cbd->ctx, trkId);
            },
            nullptr,
            nullptr);

    celix_bundleContext_waitForEvents(ctx);
    EXPECT_EQ(0, cbData.count.load()); //note create tracker canceled -> no callback
}

TEST_F(CelixBundleContextServicesTestSuite, StopMetaTrackerBeforeAsyncTrackerIsCreatedTest) {
    struct callback_data {
        std::atomic<int> count{};
        celix_bundle_context_t* ctx{nullptr};
    };
    callback_data cbData{};
    cbData.ctx = ctx;

    celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "create tracker async",
            (void*)&cbData,
            [](void *data) {
                auto cbd = static_cast<struct callback_data*>(data);

                //note create async. So a event on the event queue, but because this is done on the event queue this cannot be completed
                long trkId = celix_bundleContext_trackServiceTrackersAsync(cbd->ctx, "test-service", nullptr, nullptr, nullptr, data, [](void *data) {
                    auto* cbd = static_cast<struct callback_data*>(data);
                    cbd->count.fetch_add(1);
                });

                celix_bundleContext_stopTracker(cbd->ctx, trkId);
            },
            nullptr,
            nullptr);

    celix_bundleContext_waitForEvents(ctx);
    EXPECT_EQ(0, cbData.count.load()); //note create tracker canceled -> no callback
}


TEST_F(CelixBundleContextServicesTestSuite, SetServicesWithTrackerWhenMultipleRegistrationAlreadyExistsTest) {
    void* dummySvc = (void*)0x42;
    long svcId1 = celix_bundleContext_registerService(ctx, &dummySvc, "TestService", nullptr);
    long svcId2 = celix_bundleContext_registerService(ctx, &dummySvc, "TestService", nullptr);
    long svcId3 = celix_bundleContext_registerService(ctx, &dummySvc, "TestService", nullptr);
    ASSERT_GE(svcId1, 0);
    ASSERT_GE(svcId2, 0);
    ASSERT_GE(svcId3, 0);

    std::atomic<int> count{0};
    celix_service_tracking_options_t opts{};
    opts.callbackHandle = &count;
    opts.set = [](void *handle, void* /*svc*/) {
        auto* c = static_cast<std::atomic<int>*>(handle);
        (*c)++;
    };
    long trkId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    ASSERT_GE(trkId, 0);
    EXPECT_EQ(1, count.load()); //NOTE ensure that the service tracker only calls set once for opening tracker even if 3 service exists.

    count = 0;
    celix_bundleContext_stopTracker(ctx, trkId);
    EXPECT_EQ(1, count.load());


    celix_bundleContext_unregisterService(ctx, svcId1);
    celix_bundleContext_unregisterService(ctx, svcId2);
    celix_bundleContext_unregisterService(ctx, svcId3);
}


TEST_F(CelixBundleContextServicesTestSuite, InvalidArgumentsForUseTrackedServicesTest) {
    EXPECT_FALSE(celix_bundleContext_useTrackedService(ctx, -1, nullptr, nullptr));
    EXPECT_FALSE(celix_bundleContext_useTrackedService(ctx, 1 /*non existing*/, nullptr, nullptr));

    EXPECT_EQ(0, celix_bundleContext_useTrackedServices(ctx, -1, nullptr, nullptr));
    EXPECT_EQ(0, celix_bundleContext_useTrackedServices(ctx, 1 /*non existing*/, nullptr, nullptr));

    celix_tracked_service_use_options_t useOpts{};
    EXPECT_FALSE(celix_bundleContext_useTrackedServiceWithOptions(ctx, -1, &useOpts));
    EXPECT_FALSE(celix_bundleContext_useTrackedServiceWithOptions(ctx, 1 /*non existing*/, &useOpts));

    EXPECT_EQ(0, celix_bundleContext_useTrackedServicesWithOptions(ctx, -1, &useOpts));
    EXPECT_EQ(0, celix_bundleContext_useTrackedServicesWithOptions(ctx, 1 /*non existing*/, &useOpts));

    EXPECT_EQ(0, celix_bundleContext_getTrackedServiceCount(ctx, -1));
    EXPECT_EQ(0, celix_bundleContext_getTrackedServiceCount(ctx, 1 /*non existing*/));

    EXPECT_EQ(nullptr, celix_bundleContext_getTrackedServiceName(ctx, -1));
    EXPECT_EQ(nullptr, celix_bundleContext_getTrackedServiceName(ctx, 1 /*non existing*/));

    EXPECT_EQ(nullptr, celix_bundleContext_getTrackedServiceFilter(ctx, -1));
    EXPECT_EQ(nullptr, celix_bundleContext_getTrackedServiceFilter(ctx, 1 /*non existing*/));

    EXPECT_FALSE(celix_bundleContext_isValidTrackerId(ctx, -1));
}

TEST_F(CelixBundleContextServicesTestSuite, IsValidTrackerIdTest) {
    long trkId = celix_bundleContext_trackServices(ctx, "test");
    EXPECT_TRUE(celix_bundleContext_isValidTrackerId(ctx, trkId));
    celix_bundleContext_stopTracker(ctx, trkId);
    EXPECT_FALSE(celix_bundleContext_isValidTrackerId(ctx, trkId));
}

TEST_F(CelixBundleContextServicesTestSuite, UseTrackedServiceTest) {
    // Given 3 foo services with different service properties
    celix_properties_t* props1 = celix_properties_create();
    celix_properties_set(props1, "key", "1");
    long svcId1 = celix_bundleContext_registerService(ctx, (void*)0x42, "test", props1);
    celix_auto(celix_service_registration_guard_t) guard1 = celix_serviceRegistrationGuard_init(ctx, svcId1);

    celix_properties_t* props2 = celix_properties_create();
    celix_properties_set(props2, "key", "2");
    long svcId2 = celix_bundleContext_registerService(ctx, (void*)0x42, "test", props2);
    celix_auto(celix_service_registration_guard_t) guard2 = celix_serviceRegistrationGuard_init(ctx, svcId2);

    celix_properties_t* props3 = celix_properties_create();
    celix_properties_set(props3, "key", "3");
    long svcId3 = celix_bundleContext_registerService(ctx, (void*)0x42, "test", props3);
    celix_auto(celix_service_registration_guard_t) guard3 = celix_serviceRegistrationGuard_init(ctx, svcId3);

    // When tracking services for a service name
    long trkId = celix_bundleContext_trackServices(ctx, "test");
    celix_auto(celix_tracker_guard_t) trkGuard = celix_trackerGuard_init(ctx, trkId);

    // Then the useTrackedService function should be called for each service
    struct use_data {
        long bndId{0};
        int count{0}; //note atomic not needed because the use function is called in the same thread.
    };
    use_data data{};
    data.bndId = celix_bundleContext_getBundleId(ctx);
    celix_tracked_service_use_options_t useOpts{};
    useOpts.callbackHandle = (void*)&data;
    useOpts.use = [](void* handle, void* svc) {
        EXPECT_EQ((void*)0x42, svc);
        auto *d = static_cast<use_data*>(handle);
        d->count++;
    };
    useOpts.useWithProperties = [](void* handle, void* svc, const celix_properties_t* props) {
        EXPECT_EQ((void*)0x42, svc);
        auto* val = celix_properties_get(props, "key", nullptr);
        EXPECT_TRUE(val != nullptr);
        auto *d = static_cast<use_data*>(handle);
        d->count++;
    };
    useOpts.useWithOwner = [](void* handle, void* svc, const celix_properties_t* props, const celix_bundle_t* owner) {
        EXPECT_EQ((void*)0x42, svc);
        auto* val = celix_properties_get(props, "key", nullptr);
        EXPECT_TRUE(val != nullptr);
        auto *d = static_cast<use_data*>(handle);
        d->count++;
        EXPECT_EQ(celix_bundle_getId(owner), d->bndId);
    };
    auto count = celix_bundleContext_useTrackedServicesWithOptions(ctx, trkId, &useOpts);
    EXPECT_EQ(3, count);
    EXPECT_EQ(9, data.count); // 3x use, 3x useWithProperties, 3x useWithOwner

    // And the useTrackedServiceWithOptions function should be called a single time
    data.count = 0;
    bool called = celix_bundleContext_useTrackedServiceWithOptions(ctx, trkId, &useOpts);
    EXPECT_TRUE(called);
    EXPECT_EQ(3, data.count); // 1x use, 1x useWithProperties, 1x useWithOwner

    // And the useTrackedServices function should be called 3 times
    data.count = 0;
    count = celix_bundleContext_useTrackedServices(ctx, trkId, useOpts.callbackHandle, useOpts.use);
    EXPECT_EQ(3, count);
    EXPECT_EQ(3, data.count); // 3x use

    // And the useTrackedService function should be called a single time
    data.count = 0;
    called = celix_bundleContext_useTrackedService(ctx, trkId, useOpts.callbackHandle, useOpts.use);
    EXPECT_TRUE(called);
    EXPECT_EQ(1, data.count); // 1x use

    // When tracking a service with a filter
    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "test";
    opts.filter.filter = "(key=1)";
    long trkId2 = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    celix_auto(celix_tracker_guard_t) trkGuard2 = celix_trackerGuard_init(ctx, trkId2);

    // Then the useTrackedServiceWithOption function should be called for the service with the matching filter
    useOpts.use = nullptr;
    useOpts.useWithOwner = nullptr;
    useOpts.useWithProperties = [](void* handle, void* svc, const celix_properties_t* props) {
        EXPECT_EQ((void*)0x42, svc);
        auto* val = celix_properties_get(props, "key", nullptr);
        EXPECT_TRUE(val != nullptr);
        EXPECT_STREQ("1", val);
        auto *d = static_cast<use_data*>(handle);
        d->count++;
    };
    data.count = 0;
    called = celix_bundleContext_useTrackedServiceWithOptions(ctx, trkId2, &useOpts);
    EXPECT_TRUE(called);
    EXPECT_EQ(1, data.count); // 1x useWithProperties

    // And the useTrackedServicesWithOption function should be called a single time
    data.count = 0;
    count = celix_bundleContext_useTrackedServicesWithOptions(ctx, trkId2, &useOpts);
    EXPECT_EQ(1, count);
    EXPECT_EQ(1, data.count); // 1x useWithProperties
}

TEST_F(CelixBundleContextServicesTestSuite, GetTrackedServicesInfoTest) {
    //When a service tracker for a specific service name and with a filter
    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = "test";
    opts.filter.filter = "(key=1)";
    long trkId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    celix_auto(celix_tracker_guard_t) trkGuard = celix_trackerGuard_init(ctx, trkId);

    // And a service is registered with a matching service name and filter
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, "key", "1");
    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "test", props);
    celix_auto(celix_service_registration_guard_t) svcGuard = celix_serviceRegistrationGuard_init(ctx, svcId);

    // Then the tracked services info should be available
    EXPECT_EQ(celix_bundleContext_getTrackedServiceCount(ctx, trkId), 1);
    EXPECT_STREQ(celix_bundleContext_getTrackedServiceName(ctx, trkId), "test");
    EXPECT_STREQ(celix_bundleContext_getTrackedServiceFilter(ctx, trkId), "(&(objectClass=test)(key=1))");


    // When a tracker for all services is created
    long trkId2 = celix_bundleContext_trackServices(ctx, nullptr);
    celix_auto(celix_tracker_guard_t) trkGuard2 = celix_trackerGuard_init(ctx, trkId2);

    // Then the tracked services info should be available
    EXPECT_EQ(celix_bundleContext_getTrackedServiceCount(ctx, trkId2), 1);
    EXPECT_STREQ(celix_bundleContext_getTrackedServiceName(ctx, trkId2), "*");
    EXPECT_TRUE(strstr(celix_bundleContext_getTrackedServiceFilter(ctx, trkId2), "(objectClass=*)") != nullptr);
}

TEST_F(CelixBundleContextServicesTestSuite, UseTrackedServiceDuringTrackerCreation) {
    // Given a registered service
    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "test", nullptr);
    celix_auto(celix_service_registration_guard_t) svcGuard = celix_serviceRegistrationGuard_init(ctx, svcId);

    // And a generic event on the event thread that will wait for promise before continuing
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    auto eventCallback = [](void *data) {
        auto* f = static_cast<std::future<void>*>(data);
        auto status = f->wait_for(std::chrono::seconds{5});
        EXPECT_EQ(std::future_status::ready, status);
    };
    celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "wait for promise",
            (void*)&future,
            eventCallback,
            nullptr,
            nullptr);

    // When a service tracker is created async (and cannot be completed because the event thread is waiting)
    long trkId = celix_bundleContext_trackServicesAsync(ctx, "test");
    celix_auto(celix_tracker_guard_t) trkGuard = celix_trackerGuard_init(ctx, trkId);

    // Then a call to useTrackedService will not deadlock and the tracked service is not found, because the
    // tracker is not yet created
    bool serviceFound = celix_bundleContext_useTrackedService(ctx, trkId, nullptr, nullptr);
    EXPECT_FALSE(serviceFound);

    // When the promise is set
    promise.set_value();

    // And the tracker is created
    celix_bundleContext_waitForAsyncTracker(ctx, trkId);

    // Then the use service function will return true, because the tracker is now created and tracking the service
    serviceFound = celix_bundleContext_useTrackedService(ctx, trkId, nullptr, nullptr);
    EXPECT_TRUE(serviceFound);
}

TEST_F(CelixBundleContextServicesTestSuite, UseTrackedServiceOnTheCelixEventThread) {
    //Given a registered service with a service registration guard
    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "test", nullptr);
    celix_auto(celix_service_registration_guard_t) svcGuard = celix_serviceRegistrationGuard_init(ctx, svcId);

    //And a  service tracker for the "test" service with a service tracker guard
    long trkId = celix_bundleContext_trackServices(ctx, "test");
    celix_auto(celix_tracker_guard_t) trkGuard = celix_trackerGuard_init(ctx, trkId);

    //When all events are processed
    celix_bundleContext_waitForEvents(ctx);

    //Then I can use the tracked service on the Celix event thread
    struct callback_data {
        celix_bundle_context_t* ctx;
        long trkId;
    };
    callback_data cbData{ctx, trkId};
    auto eventCallback = [](void *data) {
        auto d = static_cast<callback_data*>(data);
        bool called = celix_bundleContext_useTrackedService(d->ctx, d->trkId, nullptr, nullptr);
        EXPECT_TRUE(called);
    };

    long eventId = celix_framework_fireGenericEvent(
            fw,
            -1,
            celix_bundle_getId(celix_framework_getFrameworkBundle(fw)),
            "use tracked service",
            (void*)&cbData,
            eventCallback,
            nullptr,
            nullptr);
    celix_framework_waitForGenericEvent(fw, eventId);
}

TEST_F(CelixBundleContextServicesTestSuite, CreateServiceTrackedOnUseServiceTrackerCall) {
    //Given a registered service with a service registration guard
    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "test", nullptr);
    celix_auto(celix_service_registration_guard_t) svcGuard = celix_serviceRegistrationGuard_init(ctx, svcId);

    //And a  service tracker for the "test" service with a service tracker guard
    long trkId = celix_bundleContext_trackServices(ctx, "test");
    celix_auto(celix_tracker_guard_t) trkGuard = celix_trackerGuard_init(ctx, trkId);

    //When all events are processed
    celix_bundleContext_waitForEvents(ctx);

    //Then I can create and destroy an additional service tracker on the callback of the useTrackedService function
    auto useCallback = [](void *data, void* /*svc*/) {
        auto c = static_cast<celix_bundle_context_t *>(data);
        long additionalTrkId = celix_bundleContext_trackServices(c, "foo");
        EXPECT_GT(additionalTrkId, 0);
        celix_bundleContext_stopTracker(c, additionalTrkId);
    };
    bool called = celix_bundleContext_useTrackedService(ctx, trkId, ctx, useCallback);
    EXPECT_TRUE(called);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterServiceWithInvalidRankingAndVersionPropertyTypeTest) {
    //Given service properties with invalid type for ranking and version
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_RANKING, "10"); //string, not long type
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, "1.0.0"); //string, not celix_version_t* type

    //When registering service
    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "TestService", props);

    //Then the registration is successful
    EXPECT_GE(svcId, 0);

    //And the service properties types are corrected
    celix_service_use_options_t opts;
    opts.filter.serviceName = "TestService";
    opts.useWithProperties = [](void* /*handle*/, void* /*svc*/, const celix_properties_t* props) {
        auto propertyType = celix_properties_getType(props, CELIX_FRAMEWORK_SERVICE_RANKING);
        EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_LONG, propertyType);

        propertyType = celix_properties_getType(props, CELIX_FRAMEWORK_SERVICE_VERSION);
        EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_VERSION, propertyType);
    };
    auto count = celix_bundleContext_useServicesWithOptions(ctx, &opts);
    EXPECT_EQ(1, count);

    celix_bundleContext_unregisterService(ctx, svcId);
}

TEST_F(CelixBundleContextServicesTestSuite, RegisterServiceWithInvalidRankingAndVersionValueTest) {
    //Given service properties with invalid type for ranking and version
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_RANKING, "foo"); //string, not convertable to long
    celix_properties_set(props, CELIX_FRAMEWORK_SERVICE_VERSION, "bar"); //string, not convertable to version

    //When registering service
    long svcId = celix_bundleContext_registerService(ctx, (void*)0x42, "TestService", props);

    //Then the registration is successful
    EXPECT_GE(svcId, 0);

    //And the service properties typer are kept as-is.
    celix_service_use_options_t opts;
    opts.filter.serviceName = "TestService";
    opts.useWithProperties = [](void* /*handle*/, void* /*svc*/, const celix_properties_t* props) {
        auto propertyType = celix_properties_getType(props, CELIX_FRAMEWORK_SERVICE_RANKING);
        EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, propertyType);

        propertyType = celix_properties_getType(props, CELIX_FRAMEWORK_SERVICE_VERSION);
        EXPECT_EQ(CELIX_PROPERTIES_VALUE_TYPE_STRING, propertyType);
    };
    auto count = celix_bundleContext_useServicesWithOptions(ctx, &opts);
    EXPECT_EQ(1, count);

    celix_bundleContext_unregisterService(ctx, svcId);
}
