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

#include <memory>

#include "celix/BundleActivator.h"
#include "celix/PromiseFactory.h"
#include "celix/PushStreamProvider.h"
#include "ICalculator.h"

class CalculatorImpl final : public ICalculator {
public:
    ~CalculatorImpl() noexcept override = default;

    celix::Promise<double> add(double a, double b) override {
        auto deferred = factory->deferred<double>();
        deferred.resolve(a+b);

        return deferred.getPromise();
    }

    void setPushStreamProvider(const std::shared_ptr<celix::PushStreamProvider>& provider) {
        psp = provider;
    }

    void setFactory(const std::shared_ptr<celix::PromiseFactory>& fac) {
        factory = fac;
    }

    std::shared_ptr<celix::PushStream<double>> result() override {
        return psp->createUnbufferedStream<double>(ses, factory);
    }

    int init() {
        return CELIX_SUCCESS;
    }

    int start() {
        ses = psp->template createSynchronousEventSource<double>(factory);

        t = std::make_unique<std::thread>([&]() {
            int counter = 0;
            stopThread = false;
            while(!stopThread) {
                ses->publish((double)counter);
                counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds{5000});
            }
        });
        return CELIX_SUCCESS;
    }

    int stop() {
        stopThread = true;
        t->join();
        ses->close();
        ses.reset();
        return CELIX_SUCCESS;
    }

    int deinit() {
        return CELIX_SUCCESS;
    }

private:
    std::unique_ptr<std::thread> t{};
    std::shared_ptr<celix::PromiseFactory> factory{};
    std::shared_ptr<celix::PushStreamProvider> psp {};
    std::shared_ptr<celix::SynchronousPushEventSource<double>> ses {};
    std::atomic<bool> stopThread{false};
};

class CalculatorProviderActivator {
public:
    explicit CalculatorProviderActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto& cmp = ctx->getDependencyManager()->createComponent(std::make_shared<CalculatorImpl>());
        cmp.createServiceDependency<celix::PromiseFactory>()
                .setRequired(true)
                .setCallbacks(&CalculatorImpl::setFactory);
        cmp.createServiceDependency<celix::PushStreamProvider>()
            .setRequired(true)
            .setCallbacks(&CalculatorImpl::setPushStreamProvider);
        cmp.createProvidedService<ICalculator>()
                .addProperty("service.exported.interfaces", celix::typeName<ICalculator>())
                .addProperty("endpoint.client.to.provider.channel.id", "1234")
                .addProperty("endpoint.provider.to.client.channel.id", "1235")
                .addProperty("service.exported.intents", "osgi.async");

        cmp.setCallbacks(&CalculatorImpl::init, &CalculatorImpl::start, &CalculatorImpl::stop, &CalculatorImpl::deinit);
        cmp.build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalculatorProviderActivator)
