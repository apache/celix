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

#include <benchmark/benchmark.h>
#include "celix/FrameworkFactory.h"

//note using c++ service for both the C and C++ benchmark, because this should not impact the performance.
class IService {
public:
    static constexpr const char * const NAME = "IService";
    virtual ~IService() noexcept = default;
};

class ServiceImpl : public IService {
public:
    ~ServiceImpl() noexcept override = default;
};

/**
 * Benchmark to measure to time needed registering and unregister services in Celix framework already containing more
 * or less registered services.
 */
class RegisterServicesBenchmark {
public:
    explicit RegisterServicesBenchmark(int64_t _nrOfServiceRegistrations, int nrOfServiceTrackers = 0) : nrOfServiceRegistrations{_nrOfServiceRegistrations}, fw{createFw()} {
        auto ctx = fw->getFrameworkBundleContext();
        registrations.reserve(nrOfServiceRegistrations);
        for (int64_t i = 0; i < nrOfServiceRegistrations; ++i) {
            registrations.emplace_back(
                    ctx->registerService<IService>(std::make_shared<ServiceImpl>(), IService::NAME).build());
        }
        for (int i = 0; i < nrOfServiceTrackers; ++i) {
            trackers.emplace_back(
                    ctx->trackServices<IService>(IService::NAME).build()
            );
        }
        ctx->waitForEvents();
    }

    static std::shared_ptr<celix::Framework> createFw() {
        celix::Properties config{};
        config.set(celix::FRAMEWORK_STATIC_EVENT_QUEUE_SIZE, 1024*10);
        config.set("CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "error");
        return celix::createFramework(config);
    }

    const int64_t nrOfServiceRegistrations;
    const std::shared_ptr<celix::Framework> fw;

    std::vector<std::shared_ptr<celix::ServiceRegistration>> registrations{};
    std::vector<std::shared_ptr<celix::GenericServiceTracker>> trackers{};
};

static void registrationAndUnregistrationTest(benchmark::State& state, bool cTest, int nrOfTrackers) {
    RegisterServicesBenchmark benchmark{state.range(0), nrOfTrackers};
    auto ctx = benchmark.fw->getFrameworkBundleContext();
    auto* cCtx = ctx->getCBundleContext();
    auto svc = std::make_shared<ServiceImpl>();

    if (cTest) {
        for (auto _ : state) {
            // This code gets timed
            long svcId = celix_bundleContext_registerService(cCtx, svc.get(), IService::NAME, nullptr);
            celix_bundleContext_unregisterService(cCtx, svcId);
        }
    } else {
        for (auto _ : state) {
            // This code gets timed
            auto reg = ctx->registerService<IService>(svc, IService::NAME)
                    .setRegisterAsync(false)
                    .setUnregisterAsync(false)
                    .build();
            reg->unregister();
        }
    }

    state.SetItemsProcessed(state.iterations());
}

static void registrationTest(benchmark::State& state, bool cTest) {
    RegisterServicesBenchmark benchmark{state.range(0)};
    auto ctx = benchmark.fw->getFrameworkBundleContext();
    auto *cCtx = ctx->getCBundleContext();
    auto svc = std::make_shared<ServiceImpl>();

    if (cTest) {
        for (auto _ : state) {
            // This code gets timed
            long svcId = celix_bundleContext_registerService(cCtx, svc.get(), IService::NAME, nullptr);
            state.PauseTiming();
            celix_bundleContext_unregisterService(cCtx, svcId);
            state.ResumeTiming();
        }
    } else {
        for (auto _ : state) {
            // This code gets timed
            auto reg = ctx->registerService<IService>(svc, IService::NAME)
                    .setRegisterAsync(false)
                    .setUnregisterAsync(false)
                    .build();
            state.PauseTiming();
            reg->unregister();
            state.ResumeTiming();
        }
    }

    state.SetItemsProcessed(state.iterations());
}

static void RegisterServicesBenchmark_cRegistrationAndUnregistration(benchmark::State& state) {
    registrationAndUnregistrationTest(state, true, 0);
}


static void RegisterServicesBenchmark_cxxRegistrationAndUnregistration(benchmark::State& state) {
    registrationAndUnregistrationTest(state, false, 0);
}

static void RegisterServicesBenchmark_cRegistrationAndUnregistrationWith100Trackers(benchmark::State& state) {
    registrationAndUnregistrationTest(state, true, 100);
}

static void RegisterServicesBenchmark_cxxRegistrationAndUnregistrationWith100Trackers(benchmark::State& state) {
    registrationAndUnregistrationTest(state, false, 100);
}

static void RegisterServicesBenchmark_cRegistration(benchmark::State& state) {
    registrationTest(state, true);
}

static void RegisterServicesBenchmark_cxxRegistration(benchmark::State& state) {
    registrationTest(state, false);
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kMillisecond)

CELIX_BENCHMARK(RegisterServicesBenchmark_cRegistrationAndUnregistration)->RangeMultiplier(10)->Range(1, 10000);
CELIX_BENCHMARK(RegisterServicesBenchmark_cxxRegistrationAndUnregistration)->RangeMultiplier(10)->Range(1, 10000);

CELIX_BENCHMARK(RegisterServicesBenchmark_cRegistrationAndUnregistrationWith100Trackers)->RangeMultiplier(10)->Range(1, 1000);
CELIX_BENCHMARK(RegisterServicesBenchmark_cxxRegistrationAndUnregistrationWith100Trackers)->RangeMultiplier(10)->Range(1, 1000);

CELIX_BENCHMARK(RegisterServicesBenchmark_cRegistration)->RangeMultiplier(10)->Range(1, 1000);
CELIX_BENCHMARK(RegisterServicesBenchmark_cxxRegistration)->RangeMultiplier(10)->Range(1, 1000);