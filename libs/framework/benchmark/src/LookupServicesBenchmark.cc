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
 * Benchmark to measure to lookup/track services in Celix framework already containing more
 * or less registered services.
 */
class LookupServicesBenchmark {
public:
    explicit LookupServicesBenchmark(int64_t _nrOfServiceRegistrations) : nrOfServiceRegistrations{_nrOfServiceRegistrations}, fw{createFw()} {
        auto ctx = fw->getFrameworkBundleContext();
        for (int i = 0; i < nrOfServiceRegistrations; ++i) {
            auto reg = ctx->registerService<IService>(std::make_shared<ServiceImpl>(), IService::NAME)
                    .addProperty("key", std::string{"value"} + std::to_string(i))
                    .build();
            registrations.emplace_back(std::move(reg));
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
};

static void findSingleService(benchmark::State& state, bool cTest, bool useFilter) {
    LookupServicesBenchmark benchmark{state.range(0)};
    auto ctx = benchmark.fw->getFrameworkBundleContext();
    auto* cCtx = ctx->getCBundleContext();

    auto index = benchmark.nrOfServiceRegistrations / 2;
    auto filter = std::string{"(key=value"} + std::to_string(index) + ")";

    if (cTest) {
        if (useFilter) {
            celix_service_filter_options_t opts{};
            opts.serviceName = IService::NAME;
            opts.filter = filter.c_str();
            for (auto _ : state) {
                // This code gets timed
                long svcId = celix_bundleContext_findServiceWithOptions(cCtx, &opts);
                if (svcId < 0) {
                    state.SkipWithError("invalid svc id");
                }
            }
        } else {
            for (auto _ : state) {
                // This code gets timed
                long svcId = celix_bundleContext_findService(cCtx, IService::NAME);
                if (svcId < 0) {
                    state.SkipWithError("invalid svc id");
                }
            }
        }
    } else {
        if (useFilter) {
            for (auto _ : state) {
                // This code gets timed
                long svcId = ctx->findServiceWithName(IService::NAME, filter);
                if (svcId < 0) {
                    state.SkipWithError("invalid svc id");
                }
            }
        } else {
            for (auto _ : state) {
                // This code gets timed
                long svcId = ctx->findServiceWithName(IService::NAME);
                if (svcId < 0) {
                    state.SkipWithError("invalid svc id");
                }
            }
        }

    }
    state.SetItemsProcessed(state.iterations());
}

static void createDestroyServiceTracker(benchmark::State& state, bool cTest) {
    LookupServicesBenchmark benchmark{state.range(0)};
    auto ctx = benchmark.fw->getFrameworkBundleContext();
    auto* cCtx = ctx->getCBundleContext();

    if (cTest) {
        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = IService::NAME;
        for (auto _ : state) {
            // This code gets timed
            long trkId = celix_bundleContext_trackServicesWithOptions(cCtx, &opts);
            celix_bundleContext_stopTracker(cCtx, trkId);
        }
    } else {
        for (auto _ : state) {
            // This code gets timed
            auto trk = ctx->trackServices<IService>(IService::NAME).build();
            trk->wait();
            trk->close();
            trk->wait();
        }
    }
    state.SetItemsProcessed(state.iterations());
}

static void LookupServicesBenchmark_cFindSingleService(benchmark::State& state) {
    findSingleService(state, true, false);
}

static void LookupServicesBenchmark_cxxFindSingleService(benchmark::State& state) {
    findSingleService(state, false, false);
}

static void LookupServicesBenchmark_cFindServiceWithFilter(benchmark::State& state) {
    findSingleService(state, true, true);
}

static void LookupServicesBenchmark_cxxFindServiceWithFilter(benchmark::State& state) {
    findSingleService(state, false, true);
}

static void LookupServicesBenchmark_cCreateDestroyTracker(benchmark::State& state) {
    createDestroyServiceTracker(state, true);
}

static void LookupServicesBenchmark_cxxCreateDestroyTracker(benchmark::State& state) {
    createDestroyServiceTracker(state, false);
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kMillisecond)

CELIX_BENCHMARK(LookupServicesBenchmark_cFindSingleService)->RangeMultiplier(10)->Range(1, 10000);
CELIX_BENCHMARK(LookupServicesBenchmark_cxxFindSingleService)->RangeMultiplier(10)->Range(1, 10000);

CELIX_BENCHMARK(LookupServicesBenchmark_cFindServiceWithFilter)->RangeMultiplier(10)->Range(1, 10000);
CELIX_BENCHMARK(LookupServicesBenchmark_cxxFindServiceWithFilter)->RangeMultiplier(10)->Range(1, 10000);

CELIX_BENCHMARK(LookupServicesBenchmark_cCreateDestroyTracker)->RangeMultiplier(10)->Range(1, 1000);
CELIX_BENCHMARK(LookupServicesBenchmark_cxxCreateDestroyTracker)->RangeMultiplier(10)->Range(1, 1000);
