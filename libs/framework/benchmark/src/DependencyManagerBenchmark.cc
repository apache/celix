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


class TestComponent : public IService {
};

/**
 * Benchmark to measure to time needed create, make active and destroy (simple) dependency manager components in
 * Celix framework where the framework already contains more or less registered services.
 */
class DependencyManagerBenchmark {
public:
    explicit DependencyManagerBenchmark(int64_t _nrOfServiceRegistrations) : nrOfServiceRegistrations{_nrOfServiceRegistrations}, fw{createFw()} {
        auto ctx = fw->getFrameworkBundleContext();
        registrations.reserve(nrOfServiceRegistrations);
        for (int64_t i = 0; i < nrOfServiceRegistrations; ++i) {
            registrations.emplace_back(
                    ctx->registerService<IService>(std::make_shared<ServiceImpl>(), IService::NAME).build());
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

static void createAndDestroyComponentTest(benchmark::State& state, bool cTest) {
    DependencyManagerBenchmark benchmark{state.range(0)};
    auto ctx = benchmark.fw->getFrameworkBundleContext();
    auto* cCtx = ctx->getCBundleContext();
    auto man = ctx->getDependencyManager();
    auto* cMan = man->cDependencyManager();

    if (cTest) {
        auto svc = std::make_shared<ServiceImpl>();
        for (auto _ : state) {
            // This code gets timed
            auto* cmp = celix_dmComponent_create(cCtx, "test");
            celix_dmComponent_addInterface(cmp, IService::NAME, nullptr, svc.get(), nullptr);

            auto* dep = celix_dmServiceDependency_create();
            celix_dmServiceDependency_setService(dep, IService::NAME, nullptr, nullptr);
            celix_dmServiceDependency_setRequired(dep, true);
            celix_dmComponent_addServiceDependency(cmp, dep);

            celix_dependencyManager_addAsync(cMan, cmp);
            celix_dependencyManager_wait(cMan);
            assert(celix_dmComponent_currentState(cmp) == CELIX_DM_CMP_STATE_TRACKING_OPTIONAL);
            celix_dependencyManager_removeAllComponents(cMan);
        }
    } else {
        for (auto _ : state) {
            // This code gets timed
            auto& cmp = man->createComponent<TestComponent>();
            cmp.createProvidedService<IService>(IService::NAME);
            cmp.createServiceDependency<IService>(IService::NAME).setRequired(true);
            man->buildAsync();
            man->wait();
            assert(cmp.getState() == ComponentState::TRACKING_OPTIONAL);
            man->clear();
        }
    }

    state.SetItemsProcessed(state.iterations());
}

static void DependencyManagerBenchmark_cCreateAndDestroyComponentTest(benchmark::State& state) {
    createAndDestroyComponentTest(state, true);
}

static void DependencyManagerBenchmark_cxxCreateAndDestroyComponentTest(benchmark::State& state) {
    createAndDestroyComponentTest(state, false);
}

#define CELIX_BENCHMARK(name) \
    BENCHMARK(name)->MeasureProcessCPUTime()->UseRealTime()->Unit(benchmark::kMillisecond)

CELIX_BENCHMARK(DependencyManagerBenchmark_cCreateAndDestroyComponentTest)->RangeMultiplier(10)->Range(1, 10000);
CELIX_BENCHMARK(DependencyManagerBenchmark_cxxCreateAndDestroyComponentTest)->RangeMultiplier(10)->Range(1, 10000);
