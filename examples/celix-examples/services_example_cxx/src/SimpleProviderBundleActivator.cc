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

#include "celix/BundleActivator.h"
#include "examples/ICalc.h"


/**
 * @brief Simple implementation of ICalc.
 */
class CalcImpl : public examples::ICalc {
public:
    explicit CalcImpl(int _seed) : seed{_seed} {}
    ~CalcImpl() override = default;

    /**
     * @brief "Calculates" a value based on the provided input and the class field seed.
     */
    int calc(int input) override {
        return seed * input;
    }
private:
    const int seed;
};

/**
 * @brief A bundle activator for a simple ICalc provider.
 */
class SimpleProviderBundleActivator {
public:
    explicit SimpleProviderBundleActivator(std::shared_ptr<celix::BundleContext> ctx) :
        registration{createCalcService(ctx)} {}

private:
    /**
     * @brief Creates and registers a single ICalc provider service.
     */
    static std::shared_ptr<celix::ServiceRegistration> createCalcService(std::shared_ptr<celix::BundleContext>& ctx) {
        int seed = 42;
        return ctx->registerService<examples::ICalc>(std::make_shared<CalcImpl>(seed))
                .addProperty("seed", seed)
                .build();
    }

    const std::shared_ptr<celix::ServiceRegistration> registration;
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(SimpleProviderBundleActivator)