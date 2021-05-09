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
#include "ICalculator.h"

class CalculatorConsumer final {
public:
    void start() {
        std::cout << "starting calc thread" << std::endl;
        active = true;
        calcThread = std::thread{[this]() {
            double secondArg = 1;
            while(active) {
                std::cout << "Calling calc" << std::endl;
                calculator->add(42, secondArg++)
                    .onSuccess([](double val) {
                        std::cout << "calc result is " << val << std::endl;
                    })
                    .onFailure([](const auto& exp) {
                        std::cerr << "error calling calc: " << exp.what() << std::endl;
                    });
                std::this_thread::sleep_for(std::chrono::seconds{5});
            }
        }};
    }

    void stop() {
        active = false;
        if (calcThread.joinable()) {
            calcThread.join();
        }
    }

    void setCalculator(const std::shared_ptr<ICalculator>& cal) {
        calculator = cal;
    }
private:
    std::atomic<bool> active{false};
    std::shared_ptr<ICalculator> calculator{};
    std::thread calcThread{};
};

class CalculatorProviderActivator {
public:
    explicit CalculatorProviderActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto& cmp = ctx->getDependencyManager()->createComponent(std::make_shared<CalculatorConsumer>());
        cmp.createServiceDependency<ICalculator>()
                .setRequired(true)
                .setCallbacks(&CalculatorConsumer::setCalculator);
        cmp.setCallbacks(nullptr, &CalculatorConsumer::start, &CalculatorConsumer::stop, nullptr);
        cmp.build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalculatorProviderActivator)
