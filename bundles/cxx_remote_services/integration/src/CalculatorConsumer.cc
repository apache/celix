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

#include "celix/IShellCommand.h"
#include "celix/BundleActivator.h"
#include "celix/rsa/IConfiguredDiscoveryManager.h"
#include "ICalculator.h"

class CalculatorConsumer final : public celix::IShellCommand {
public:
    ~CalculatorConsumer() override = default;

    void setCalculator(const std::shared_ptr<ICalculator>& cal) {
        calculator = cal;
    }

    void executeCommand(const std::string &/*commandLine*/, const std::vector<std::string> &/*commandArgs*/, FILE *outStream,
                        FILE */*errorStream*/) override {
        constexpr double arg1 = 42;
        thread_local double counter = 1;
        fprintf(outStream, "Calling calc add(%f,%f)\n", arg1, counter);
        calculator->add(arg1, counter)
                .onSuccess([c = counter](double val) {
                    fprintf(stdout, "calc add(%f, %f) is %f\n", arg1, c, val);
                })
                .onFailure([c = counter](const auto& exp) {
                    fprintf(stderr, "error calling calc add(%f,%f). Exception: %s\n", arg1, c, exp.what());
                });
        counter++;
    }
private:
    std::shared_ptr<ICalculator> calculator{};
};

class CalculatorConsumerActivator {
public:
    explicit CalculatorConsumerActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto& cmp = ctx->getDependencyManager()->createComponent(std::make_shared<CalculatorConsumer>());
        cmp.createServiceDependency<ICalculator>()
                .setRequired(true)
                .setCallbacks(&CalculatorConsumer::setCalculator);
        cmp.createProvidedService<celix::IShellCommand>()
                .addProperty(celix::IShellCommand::COMMAND_NAME, "calc");
        cmp.build();

        //bootstrap own configured import discovery to the configured discovery manager
        auto path = ctx->getBundle().getEntry("META-INF/discovery/endpoint_discovery.json");
        ctx->useService<celix::rsa::IConfiguredDiscoveryManager>()
                .addUseCallback([&path](auto& disc) {
                    disc.addConfiguredDiscoveryFile(path);
                })
                .build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalculatorConsumerActivator)
