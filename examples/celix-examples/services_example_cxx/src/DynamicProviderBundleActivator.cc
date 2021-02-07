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

#include <random>

#include "celix/IShellCommand.h"
#include "celix/BundleActivator.h"
#include "examples/ICalc.h"


class CalcImpl : public examples::ICalc {
public:
    explicit CalcImpl(int _seed) : seed{_seed} {}
    ~CalcImpl() override = default;

    int calc(int input) override {
        return seed * input;
    }
private:
    const int seed;
};

class DynamicProviderFactory : public celix::IShellCommand {
public:
    explicit DynamicProviderFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}

    void executeCommand(std::string /*commandLine*/, std::vector<std::string> /*commandArgs*/, FILE* /*outStream*/, FILE* /*errorStream*/) override {
        clearCacls();
        registerCalcs();
    }

    void registerCalcs() {
        std::lock_guard<std::mutex> lock{mutex};
        while (registrations.size() <= 100) {
            auto reg = createCalcService();
            std::cout << "Registered calc service with svc id " << reg->getServiceId() << " and ranking " << reg->getServiceRanking() << std::endl;
            registrations.emplace_back(std::move(reg));
        }
    }

    void clearCacls() {
        std::lock_guard<std::mutex> lock{mutex};
        std::cout << "Clearing all registered calc services" << std::endl;
        registrations.clear();
    }
private:
    std::shared_ptr<celix::ServiceRegistration> createCalcService() {
        int seed = distribution(generator);
        int rank = distribution(generator);
        return ctx->registerService<examples::ICalc>(std::make_shared<CalcImpl>(seed))
                .addProperty("seed", seed)
                .addProperty(celix::SERVICE_RANKING, rank)
                .build();
    }

    const std::shared_ptr<celix::BundleContext> ctx;
    std::default_random_engine generator{};
    std::uniform_int_distribution<int> distribution{-100,100};

    std::mutex mutex{}; //protects below
    std::vector<std::shared_ptr<celix::ServiceRegistration>> registrations{};
};

class DynamicProviderBundleActivator {
public:
    explicit DynamicProviderBundleActivator(std::shared_ptr<celix::BundleContext> ctx) : factory{std::make_shared<DynamicProviderFactory>(ctx)} {
        cmdReg = ctx->registerService<celix::IShellCommand>(factory)
                .addProperty(celix::IShellCommand::COMMAND_NAME, "providers_reset")
                .build();
        factory->registerCalcs();
    }
private:
    std::shared_ptr<DynamicProviderFactory> factory;
    std::shared_ptr<celix::ServiceRegistration> cmdReg{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(DynamicProviderBundleActivator)