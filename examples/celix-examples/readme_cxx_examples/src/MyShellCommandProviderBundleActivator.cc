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
#include "celix/IShellCommand.h"

class MyCommand : public celix::IShellCommand {
public:
    explicit MyCommand(std::string_view _name) : name{_name} {}

    ~MyCommand() noexcept override = default;

    void executeCommand(
            const std::string& commandLine,
            const std::vector<std::string>& /*commandArgs*/,
            FILE* outStream,
            FILE* /*errorStream*/) override {
        fprintf(outStream, "Hello from bundle %s with command line '%s'\n", name.c_str(), commandLine.c_str());
    }
private:
    const std::string name;
};

class MyShellCommandProviderBundleActivator {
public:
    explicit MyShellCommandProviderBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto svcObject = std::make_shared<MyCommand>(ctx->getBundle().getName());
        cmdShellRegistration = ctx->registerService<celix::IShellCommand>(std::move(svcObject))
                .addProperty(celix::IShellCommand::COMMAND_NAME, "MyCommand")
                .build();
    }

    ~MyShellCommandProviderBundleActivator() noexcept = default;
private:
    //NOTE when celix::ServiceRegistration goes out of scope the underlining service will be un-registered
    std::shared_ptr<celix::ServiceRegistration> cmdShellRegistration{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(MyShellCommandProviderBundleActivator)